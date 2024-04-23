// Copyright 2024, Andrew Drogalis
// GNU License

#include "fx_utilities.h"

#include <ctype.h>// for toupper

#include <algorithm>      // for find, tran...
#include <array>          // for array
#include <ctime>          // for time, loca...
#include <expected>       // for expected
#include <filesystem>     // for is_directory, create_directories...
#include <iostream>       // for basic_ostream
#include <source_location>// for current, function_name...
#include <string>         // for basic_string

#include "boost/log/trivial.hpp"                        // for severity_l...
#include "boost/log/utility/setup/common_attributes.hpp"// for add_common...
#include "boost/log/utility/setup/file.hpp"             // for add_file_log
#include "keychain/keychain.h"                          // for Error, set...
#include <boost/log/utility/setup/console.hpp>          // for add_consule_log

#include "fx_exception.h"// for FXException

namespace fxordermgmt
{

std::expected<std::string, FXException> FXUtilities::keyring_unlock_get_password(std::string const& account_type, std::string const& username)
{
    // Required to prompt for first keyring unlock
    keychain::Error error = keychain::Error {};
    keychain::setPassword("Forex_Keychain_Unlocker", "", "", "", error);
    if (error)
    {
        return std::expected<std::string, FXException> {std::unexpect, std::source_location::current().function_name(),
                                                        "Cannot Unlock Keyring - " + error.message};
    }
    // -------------------
    std::string password, service_id, package;

    if (account_type == "LIVE") { service_id = "Live_Account", package = "com.gain_capital_forex.live_account"; }
    else { service_id = "Test_Account", package = "com.gain_capital_forex.test_account"; }
    // -------------------
    // Checks to confirm password exists and sets if it doesn't
    error = keychain::Error {};
    password = keychain::getPassword(package, service_id, username, error);
    if (error.type == keychain::ErrorType::NotFound)
    {
        std::cout << account_type << " Account password not found. Please input password: ";
        std::cin >> password;
        error = keychain::Error {};
        keychain::setPassword(package, service_id, username, password, error);
        if (error)
        {
            return std::expected<std::string, FXException> {std::unexpect, std::source_location::current().function_name(),
                                                            "Failed to Set Password - " + error.message};
        }
    }
    else if (error)
    {
        return std::expected<std::string, FXException> {std::unexpect, std::source_location::current().function_name(), error.message};
    }
    // -------------------
    return std::expected<std::string, FXException> {password};
}

std::expected<bool, FXException> FXUtilities::validate_user_settings(std::string& update_interval, int update_span, int& update_frequency_seconds)
{
    std::array<int, 7> const SPAN_M = {1, 2, 3, 5, 10, 15, 30};// Span intervals for minutes
    std::array<int, 4> const SPAN_H = {1, 2, 4, 8};            // Span intervals for hours
    // -------------------
    transform(update_interval.begin(), update_interval.end(), update_interval.begin(), ::toupper);

    if (update_interval != "MINUTE" && update_interval != "HOUR")
    {
        return std::expected<bool, FXException> {std::unexpect, std::source_location::current().function_name(),
                                                 "Interval Error - Provide one of the following intervals: 'HOUR', 'MINUTE'"};
    }
    // -------------------
    if (update_interval == "HOUR")
    {
        if (std::find(SPAN_H.begin(), SPAN_H.end(), update_span) == SPAN_H.end())
        {
            return std::expected<bool, FXException> {std::unexpect, std::source_location::current().function_name(),
                                                     "Span Hour Error - Provide one of the following spans: 1, 2, 4, 8"};
        }
        update_frequency_seconds = 3600 * update_span;
    }
    else
    {
        if (std::find(SPAN_M.begin(), SPAN_M.end(), update_span) == SPAN_M.end())
        {
            return std::expected<bool, FXException> {std::unexpect, std::source_location::current().function_name(),
                                                     "Span Minute Error - Provide one of the following spans: 1, 2, 3, 5, 10, 15, 30"};
        }
        update_frequency_seconds = 60 * update_span;
    }
    // -------------------
    return std::expected<bool, FXException> {true};
}

std::expected<bool, FXException> FXUtilities::initialize_logging_file(std::string const& working_directory)
{
    FXUtilities fxUtils;
    std::string const dir = working_directory + "/interface_files/logs";
    std::string const file_name = dir + "/" + fxUtils.get_todays_date() + "_FX_Order_Management.log";
    // -------------------
    try
    {
        bool valid = std::filesystem::is_directory(dir);
        if (! valid) { valid = std::filesystem::create_directories(dir); }
        if (! valid) { throw std::filesystem::filesystem_error {"Could not make directory", std::error_code {}}; }
    }
    catch (std::filesystem::filesystem_error const& e)
    {
        return std::expected<bool, FXException> {std::unexpect, std::source_location::current().function_name(), e.what()};
    }
    // -------------------
    static auto file_sink =
        boost::log::add_file_log(boost::log::keywords::file_name = file_name, boost::log::keywords::format = "[%TimeStamp%]: %Message%",
                                 boost::log::keywords::auto_flush = true);

    boost::log::add_common_attributes();

    boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug);
    // -------------------
    return std::expected<bool, FXException> {true};
}

void log_to_std_ouput() { static auto console_sink = boost::log::add_console_log(std::cout, boost::log::keywords::format = ">> %Message%"); }

std::string FXUtilities::get_todays_date() noexcept
{
    time_t t = std::time(0);
    char DATE_TODAY[50];
    struct tm* tmp = localtime(&t);
    strftime(DATE_TODAY, sizeof(DATE_TODAY), "%Y_%m_%d", tmp);
    // -------------------
    return DATE_TODAY;
}

}// namespace fxordermgmt
