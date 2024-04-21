// Copyright 2024, Andrew Drogalis
// GNU License

#include <string>
#include <ctime>  
#include <filesystem>
#include <typeinfo>

#include "gtest/gtest.h"

#include "fx_main_utilities.hpp"
#include "fx_utilities.h"
#include "fx_exception.h"

namespace {

std::string get_todays_date()
{
    time_t t = std::time(0);
    char DATE_TODAY[50];
    struct tm* tmp = localtime(&t);
    strftime(DATE_TODAY, sizeof(DATE_TODAY), "%Y_%m_%d", tmp);
    return DATE_TODAY;
}


TEST(ForexUtilitiesTests, Validate_User_Settings_Correct) {
    fxordermgmt::FXUtilities fx_utils;

    std::string interval = "Minute";
    int span = 1;
    int update_frequency {};

    auto response = fx_utils.validate_user_settings(interval, span, update_frequency);

    if (response)
    {
        EXPECT_EQ(response.value(), true);
        EXPECT_EQ(interval, "MINUTE");
        EXPECT_EQ(update_frequency, 60);
    }
    else
    {
        FAIL() << response.error().what();
    }   

    span = 10;
    response = fx_utils.validate_user_settings(interval, span, update_frequency);

    if (response)
    {
        EXPECT_EQ(response.value(), true);
        EXPECT_EQ(interval, "MINUTE");
        EXPECT_EQ(update_frequency, 600);
    }
    else
    {
        FAIL() << response.error().what();
    }   

    interval = "Hour";
    span = 1;
    update_frequency = 0;

    response = fx_utils.validate_user_settings(interval, span, update_frequency);

    if (response)
    {
        EXPECT_EQ(response.value(), true);
        EXPECT_EQ(interval, "HOUR");
        EXPECT_EQ(update_frequency, 3600);
    }
    else
    {
        FAIL() << response.error().what();
    }   

    span = 8;
    response = fx_utils.validate_user_settings(interval, span, update_frequency);

    if (response)
    {
        EXPECT_EQ(response.value(), true);
        EXPECT_EQ(interval, "HOUR");
        EXPECT_EQ(update_frequency, 28'800);
    }
    else
    {
        FAIL() << response.error().what();
    }   
}


TEST(ForexUtilitiesTests, Validate_User_Settings_Incorrect) {
    fxordermgmt::FXUtilities fx_utils;

    std::string interval = "Day";
    int span = 1;
    int update_frequency {};

    auto response = fx_utils.validate_user_settings(interval, span, update_frequency);

    if (! response)
    {
        EXPECT_EQ(typeid(response.error()), typeid(fxordermgmt::FXException));
        EXPECT_EQ(response.error().what(), "Interval Error - Provide one of the following intervals: 'HOUR', 'MINUTE'");
    }
    else
    {
        FAIL();
    }   

    interval = "minute";
    span = 100;
    response = fx_utils.validate_user_settings(interval, span, update_frequency);

    if (! response)
    {
        EXPECT_EQ(typeid(response.error()), typeid(fxordermgmt::FXException));
        EXPECT_EQ(response.error().what(), "Span Minute Error - Provide one of the following spans: 1, 2, 3, 5, 10, 15, 30");
    }
    else
    {
        FAIL();
    }   

    interval = "Hour";
    span = 10;
    response = fx_utils.validate_user_settings(interval, span, update_frequency);

    if (! response)
    {
        EXPECT_EQ(typeid(response.error()), typeid(fxordermgmt::FXException));
        EXPECT_EQ(response.error().what(), "Span Hour Error - Provide one of the following spans: 1, 2, 4, 8");
    }
    else
    {
        FAIL();
    }   
}


TEST(ForexUtilitiesTests, Initialize_Logging_Correct) {
    fxordermgmt::FXUtilities fx_utils;

    std::string parent_dir = std::filesystem::current_path().parent_path();

    auto response = fx_utils.initialize_logging(parent_dir);

    if (response)
    {
        std::string logging_dir = parent_dir + "/interface_files/logs";
        std::string logging_file = logging_dir + "/" + get_todays_date() + "_FX_Order_Management.log";

        EXPECT_EQ(std::filesystem::is_directory(logging_dir), true);
        EXPECT_EQ(std::filesystem::is_regular_file(logging_file), true);
    }
    else
    {
        FAIL() << response.error().what();
    }
}


TEST(ForexUtilitiesTests, Get_Todays_Date_Correct) {
    fxordermgmt::FXUtilities fx_utils;

    EXPECT_EQ(fx_utils.get_todays_date(), get_todays_date());
}


TEST(ForexUtilitiesTests, Get_Todays_Date_Incorrect) {
    fxordermgmt::FXUtilities fx_utils;

    EXPECT_EQ(fx_utils.get_todays_date(), "");
}


TEST(ForexMainUtilitiesTests, Validate_Main_Parameters_Correct) {
    std::string account = "";
    bool place_trades = false;
    int max_retry_failures = 3;

    int argc = 1;
    char *argv[] = {"Program_Name"};
   
    bool response = fxordermgmt::validateMainParameters(argc, argv, account, place_trades, max_retry_failures);
    EXPECT_EQ(response, true);
    EXPECT_EQ(account, "");
    EXPECT_EQ(place_trades, false);
    EXPECT_EQ(max_retry_failures, 3);

    argc = 2;
    char *argv2[] = {"Program_Name", "PAPER"};
   
    bool response = fxordermgmt::validateMainParameters(argc, argv2, account, place_trades, max_retry_failures);
    EXPECT_EQ(response, true);
    EXPECT_EQ(account, "PAPER");

    argc = 3;
    char *argv3[] = {"Program_Name", "PAPER", "trUE"};
   
    bool response = fxordermgmt::validateMainParameters(argc, argv3, account, place_trades, max_retry_failures);
    EXPECT_EQ(response, true);
    EXPECT_EQ(account, "PAPER");
    EXPECT_EQ(place_trades, true);

    argc = 3;
    char *argv3_1[] = {"Program_Name", "PAPER", "1"};
   
    bool response = fxordermgmt::validateMainParameters(argc, argv3_1, account, place_trades, max_retry_failures);
    EXPECT_EQ(response, true);
    EXPECT_EQ(account, "PAPER");
    EXPECT_EQ(place_trades, true);

    argc = 4;
    char *argv4[] = {"Program_Name", "PAPER", "1", "5"};
   
    bool response = fxordermgmt::validateMainParameters(argc, argv4, account, place_trades, max_retry_failures);
    EXPECT_EQ(response, true);
    EXPECT_EQ(account, "PAPER");
    EXPECT_EQ(place_trades, true);
    EXPECT_EQ(max_retry_failures, 5);   
}


TEST(ForexMainUtilitiesTests, Validate_Main_Parameters_Incorrect) {
    std::string account = "";
    bool place_trades = false;
    int max_retry_failures = 3;

    int argc = 10;
    char *argv[] = {"Program_Name"};
   
    bool response = fxordermgmt::validateMainParameters(argc, argv, account, place_trades, max_retry_failures);
    EXPECT_EQ(response, false);

    argc = 3;
    char *argv3[] = {"Program_Name", "PAPER", "X"};
   
    bool response = fxordermgmt::validateMainParameters(argc, argv3, account, place_trades, max_retry_failures);
    EXPECT_EQ(response, false);
    EXPECT_EQ(account, "PAPER");
    EXPECT_EQ(place_trades, false);

    argc = 4;
    char *argv4[] = {"Program_Name", "PAPER", "1", "ABCD"};
   
    bool response = fxordermgmt::validateMainParameters(argc, argv4, account, place_trades, max_retry_failures);
    EXPECT_EQ(response, false);
    EXPECT_EQ(account, "PAPER");
    EXPECT_EQ(place_trades, true);
    EXPECT_EQ(max_retry_failures, 3);   
}


TEST(ForexMainUtilitiesTests, Validate_Account_Type_Correct) {
    std::string account = "Live";
   
    EXPECT_EQ(fxordermgmt::validateAccountType(account), true);

    account = "LivE";

    EXPECT_EQ(fxordermgmt::validateAccountType(account), true);

    account = "paper";

    EXPECT_EQ(fxordermgmt::validateAccountType(account), true);

    account = "PaPer";

    EXPECT_EQ(fxordermgmt::validateAccountType(account), true);
}


TEST(ForexMainUtilitiesTests, Validate_Account_Type_Incorrect) {
    std::string account = "Liv1";
   
    EXPECT_EQ(fxordermgmt::validateAccountType(account), false);

    account = "X";

    EXPECT_EQ(fxordermgmt::validateAccountType(account), false);

    account = "paaper";

    EXPECT_EQ(fxordermgmt::validateAccountType(account), false);

    account = "PaPer!";

    EXPECT_EQ(fxordermgmt::validateAccountType(account), false);
}


} // namespace