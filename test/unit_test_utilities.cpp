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
        EXPECT_TRUE(response.value());
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
        EXPECT_TRUE(response.value());
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
        EXPECT_TRUE(response.value());
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
        EXPECT_TRUE(response.value());
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
        EXPECT_EQ(std::string(response.error().what()), "Interval Error - Provide one of the following intervals: 'HOUR', 'MINUTE'");
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
        EXPECT_EQ(std::string(response.error().what()), "Span Minute Error - Provide one of the following spans: 1, 2, 3, 5, 10, 15, 30");
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
        EXPECT_EQ(std::string(response.error().what()), "Span Hour Error - Provide one of the following spans: 1, 2, 4, 8");
    }
    else
    {
        FAIL();
    }   
}


TEST(ForexUtilitiesTests, Initialize_Logging_Correct) {
    fxordermgmt::FXUtilities fx_utils;

    std::string parent_dir = std::filesystem::current_path().parent_path();

    auto response = fx_utils.initialize_logging_file(parent_dir);

    if (response)
    {
        std::string logging_dir = parent_dir + "/interface_files/logs";
        std::string logging_file = logging_dir + "/" + get_todays_date() + "_FX_Order_Management.log";

        EXPECT_TRUE(std::filesystem::is_directory(logging_dir));
        EXPECT_TRUE(std::filesystem::is_regular_file(logging_file));
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

    EXPECT_FALSE(fx_utils.get_todays_date() == "");
}


TEST(ForexMainUtilitiesTests, Validate_Main_Parameters_1Correct) {
    std::string account = "";
    bool place_trades = false;
    int max_retry_failures = 3;

    int argc = 1;
    char prog_name[] = "Program_Name";
    char *argv[] = {prog_name};
   
    bool response = fxordermgmt::validateMainParameters(argc, argv, account, place_trades, max_retry_failures);
    EXPECT_TRUE(response);
    EXPECT_EQ(account, "");
    EXPECT_FALSE(place_trades);
    EXPECT_EQ(max_retry_failures, 3);
}


TEST(ForexMainUtilitiesTests, Validate_Main_Parameters_2Correct) {
    std::string account = "";
    bool place_trades = false;
    int max_retry_failures = 3;

    int argc = 3;
    char prog_name[] = "Program_Name";
    char paper_or_live[] = "PAPER";
    char account_flag[] = "-a";
    char *argv2[] = {prog_name, account_flag, paper_or_live};
   
    bool response = fxordermgmt::validateMainParameters(argc, argv2, account, place_trades, max_retry_failures);
    EXPECT_TRUE(response);
    EXPECT_EQ(account, "PAPER");
}


TEST(ForexMainUtilitiesTests, Validate_Main_Parameters_3Correct) {
    std::string account = "";
    bool place_trades = false;
    int max_retry_failures = 3;

    int argc = 5;
    char prog_name[] = "Program_Name";
    char paper_or_live[] = "PAPER";
    char account_flag[] = "-a";
    char place_trade_flag[] = "-p";
    char place_trade_str[] = "trUE";
    char *argv3[] = {prog_name, account_flag, paper_or_live, place_trade_flag, place_trade_str};
   
    bool response = fxordermgmt::validateMainParameters(argc, argv3, account, place_trades, max_retry_failures);
    EXPECT_TRUE(response);
    EXPECT_EQ(account, "PAPER");
    EXPECT_TRUE(place_trades);
}


TEST(ForexMainUtilitiesTests, Validate_Main_Parameters_4Correct) {
    std::string account = "";
    bool place_trades = false;
    int max_retry_failures = 3;

    int argc = 5;
    char prog_name[] = "Program_Name";
    char paper_or_live[] = "PAPER";
    char account_flag[] = "-a";
    char place_trade_flag[] = "-p";
    char place_trade_str2[] = "1";
    char *argv3_1[] = {prog_name, account_flag, paper_or_live, place_trade_flag, place_trade_str2};
   
    bool response = fxordermgmt::validateMainParameters(argc, argv3_1, account, place_trades, max_retry_failures);
    EXPECT_TRUE(response);
    EXPECT_EQ(account, "PAPER");
    EXPECT_TRUE(place_trades);
}


TEST(ForexMainUtilitiesTests, Validate_Main_Parameters_5Correct) {
    std::string account = "";
    bool place_trades = false;
    int max_retry_failures = 3;

    int argc = 7;
    char prog_name[] = "Program_Name";
    char paper_or_live[] = "PAPER";
    char account_flag[] = "-a";
    char place_trade_flag[] = "-p";
    char place_trade_str2[] = "1";
    char max_retrys_flag[] = "-m";
    char max_retrys_str[] = "5";
    char *argv4[] = {prog_name, account_flag, paper_or_live, place_trade_flag, place_trade_str2, max_retrys_flag, max_retrys_str};
   
    bool response = fxordermgmt::validateMainParameters(argc, argv4, account, place_trades, max_retry_failures);
    EXPECT_TRUE(response);
    EXPECT_EQ(account, "PAPER");
    EXPECT_TRUE(place_trades);
    EXPECT_EQ(max_retry_failures, 5);   
}


TEST(ForexMainUtilitiesTests, Validate_Main_Parameters_1Incorrect) {
    std::string account = "";
    bool place_trades = false;
    int max_retry_failures = 3;

    // Too Many Arguments 
    int argc = 10;
    char prog_name[] = "Program_Name";
    char *argv[] = {prog_name};
   
    bool response = fxordermgmt::validateMainParameters(argc, argv, account, place_trades, max_retry_failures);
    EXPECT_FALSE(response);
}

TEST(ForexMainUtilitiesTests, Validate_Main_Parameters_2Incorrect) {
    std::string account = "";
    bool place_trades = false;
    int max_retry_failures = 3;

    // Wrong Flag Type
    int argc = 3;
    char prog_name[] = "Program_Name";
    char paper_or_live[] = "PAPER";
    char account_flag_wrong[] = "-t";
    char *argv2[] = {prog_name, account_flag_wrong, paper_or_live};
   
    bool response = fxordermgmt::validateMainParameters(argc, argv2, account, place_trades, max_retry_failures);
    EXPECT_FALSE(response);
    EXPECT_EQ(account, "");
}

TEST(ForexMainUtilitiesTests, Validate_Main_Parameters_3Incorrect) {
    std::string account = "";
    bool place_trades = false;
    int max_retry_failures = 3;

    // Wrong Place Trades Value
    int argc = 3;
    char prog_name[] = "Program_Name";
    char place_trade_flag[] = "-p";
    char place_trade_str_wrong[] = "X";
    char *argv3[] = {prog_name, place_trade_flag, place_trade_str_wrong};
   
    bool response = fxordermgmt::validateMainParameters(argc, argv3, account, place_trades, max_retry_failures);
    EXPECT_FALSE(response);
    EXPECT_FALSE(place_trades);
}

TEST(ForexMainUtilitiesTests, Validate_Main_Parameters_4Incorrect) {
    std::string account = "";
    bool place_trades = false;
    int max_retry_failures = 3;

    // Wrong Max Retry Failures Value
    int argc = 3;
    char prog_name[] = "Program_Name";
    char max_retrys_flag[] = "-m";
    char max_retrys_str_wrong[] = "ABCD";
    char *argv4[] = {prog_name, max_retrys_flag, max_retrys_str_wrong};
   
    bool response = fxordermgmt::validateMainParameters(argc, argv4, account, place_trades, max_retry_failures);
    EXPECT_FALSE(response);
    EXPECT_EQ(max_retry_failures, 3);   
}


TEST(ForexMainUtilitiesTests, Validate_Account_Type_Correct) {
    std::string account = "Live";
   
    EXPECT_TRUE(fxordermgmt::validateAccountType(account));

    account = "LivE";

    EXPECT_TRUE(fxordermgmt::validateAccountType(account));

    account = "paper";

    EXPECT_TRUE(fxordermgmt::validateAccountType(account));

    account = "PaPer";

    EXPECT_TRUE(fxordermgmt::validateAccountType(account));
}


TEST(ForexMainUtilitiesTests, Validate_Account_Type_Incorrect) {
    std::string account = "Liv1";
   
    EXPECT_FALSE(fxordermgmt::validateAccountType(account));

    account = "X";

    EXPECT_FALSE(fxordermgmt::validateAccountType(account));

    account = "paaper";

    EXPECT_FALSE(fxordermgmt::validateAccountType(account));

    account = "PaPer!";

    EXPECT_FALSE(fxordermgmt::validateAccountType(account));
}


} // namespace