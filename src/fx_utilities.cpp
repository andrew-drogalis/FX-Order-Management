// Copyright 2024, Andrew Drogalis
// GNU License

#include "fx_utilities.h"

#include <algorithm>// for find, tran...
#include <ctime>    // for time, loca...
#include <ctype.h>  // for toupper
#include <iostream> // for basic_ostream
#include <string>   // for basic_string
#include <vector>   // for vector

#include "boost/log/trivial.hpp"
#include "boost/log/utility/setup/common_attributes.hpp"
#include "boost/log/utility/setup/file.hpp"
#include "keychain/keychain.h"

namespace fxordermgmt
{

bool FXUtilities::setup_password_first_time(const std::string& account_type, const std::string& username)
{
    std::string const service_id_test = "Test_Account", service_id_live = "Live_Account", package_test = "com.gain_capital_forex.test_account",
                      package_live = "com.gain_capital_forex.live_account";
    std::string test_account_password, account_password, password;

    // Required to prompt for first keyring unlock
    keychain::Error error = keychain::Error {};
    keychain::setPassword("Forex_Keychain_Unlocker", "", "", "", error);
    if (error)
    {
        std::cerr << error.message << std::endl;
        return false;
    }
    // -------------------------------
    if (account_type == "PAPER")
    {
        error = keychain::Error {};
        password = keychain::getPassword(package_test, service_id_test, username, error);
        if (error.type == keychain::ErrorType::NotFound)
        {
            std::cout << "Test Account password not found. Please input password: ";
            std::cin >> test_account_password;
            keychain::setPassword(package_test, service_id_test, username, test_account_password, error);
            if (error)
            {
                std::cerr << "Test Account " << error.message << '\n';
                return false;
            }
        }
        else if (error)
        {
            std::cerr << error.message << '\n';
            return false;
        }
    }
    // -------------------------------
    if (account_type == "LIVE")
    {
        error = keychain::Error {};
        password = keychain::getPassword(package_live, service_id_live, username, error);
        if (error.type == keychain::ErrorType::NotFound)
        {
            std::cout << "Live Account password not found. Please input password: ";
            std::cin >> account_password;
            keychain::setPassword(package_live, service_id_live, username, account_password, error);
            if (error)
            {
                std::cerr << "Live Account " << error.message << '\n';
                return false;
            }
        }
        else if (error)
        {
            std::cerr << error.message << '\n';
            return false;
        }
    }
    // No Errors -> return true;
    return true;
}

void FXUtilities::init_logging(const std::string& working_directory)
{
    FXUtilities fxUtils;
    std::string const file_name = working_directory + "/" + fxUtils.get_todays_date() + "_FX_Order_Management.log";
    static auto file_sink =
        boost::log::add_file_log(boost::log::keywords::file_name = file_name, boost::log::keywords::format = "[%TimeStamp%]: %Message%",
                                 boost::log::keywords::auto_flush = true);
    boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug);
    boost::log::add_common_attributes();
}

std::string FXUtilities::get_todays_date() noexcept
{
    time_t t = std::time(0);
    char DATE_TODAY[50];
    struct tm* tmp = localtime(&t);
    strftime(DATE_TODAY, sizeof(DATE_TODAY), "%Y_%m_%d", tmp);
    return DATE_TODAY;
}

bool FXUtilities::validate_user_interval(std::string update_interval, int update_span, int& update_frequency_seconds)
{
    transform(update_interval.begin(), update_interval.end(), update_interval.begin(), ::toupper);

    std::vector<int> const SPAN_M = {1, 2, 3, 5, 10, 15, 30};// Span intervals for minutes
    std::vector<int> const SPAN_H = {1, 2, 4, 8};            // Span intervals for hours
    std::vector<std::string> const INTERVAL = {"HOUR", "MINUTE"};

    if (std::find(INTERVAL.begin(), INTERVAL.end(), update_interval) == INTERVAL.end())
    {
        std::cerr << "Interval Error - Provide one of the following intervals: 'HOUR', 'MINUTE'" << std::endl;
        return false;
    }
    if (update_interval == "HOUR")
    {
        if (std::find(SPAN_H.begin(), SPAN_H.end(), update_span) == SPAN_H.end())
        {
            std::cerr << "Span Hour Error - Provide one of the following spans: 1, 2, 4, 8" << std::endl;
            return false;
        }
        update_frequency_seconds = 3600 * update_span;
    }
    else if (update_interval == "MINUTE")
    {
        if (std::find(SPAN_M.begin(), SPAN_M.end(), update_span) == SPAN_M.end())
        {
            std::cerr << "Span Minute Error - Provide one of the following spans: 1, 2, 3, 5, 10, 15, 30" << std::endl;
            return false;
        }
        update_frequency_seconds = 60 * update_span;
    }
    // No Errors -> return true;
    return true;
}

}// namespace fxordermgmt
