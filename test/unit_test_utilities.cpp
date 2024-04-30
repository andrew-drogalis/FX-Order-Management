// Copyright 2024, Andrew Drogalis
// GNU License

#include <ctime>
#include <filesystem>
#include <string>
#include <typeinfo>

#include "gtest/gtest.h"

#include "fx_exception.h"
#include "fx_main_utilities.hpp"
#include "fx_utilities.h"

namespace
{

std::string get_todays_date()
{
    time_t t = std::time(0);
    char DATE_TODAY[50];
    struct tm* tmp = localtime(&t);
    strftime(DATE_TODAY, sizeof(DATE_TODAY), "%Y_%m_%d", tmp);
    return DATE_TODAY;
}

TEST(ForexUtilitiesTests, Validate_User_Settings_Correct_1_Test)
{
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
}

TEST(ForexUtilitiesTests, Validate_User_Settings_Correct_2_Test)
{
    fxordermgmt::FXUtilities fx_utils;

    std::string interval = "Minute";
    int span = 10;
    int update_frequency {};

    auto response = fx_utils.validate_user_settings(interval, span, update_frequency);

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
}

TEST(ForexUtilitiesTests, Validate_User_Settings_Correct_3_Test)
{
    fxordermgmt::FXUtilities fx_utils;

    std::string interval = "Hour";
    int span = 1;
    int update_frequency {};

    auto response = fx_utils.validate_user_settings(interval, span, update_frequency);

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
}

TEST(ForexUtilitiesTests, Validate_User_Settings_Correct_4_Test)
{
    fxordermgmt::FXUtilities fx_utils;

    std::string interval = "Hour";
    int span = 8;
    int update_frequency {};

    auto response = fx_utils.validate_user_settings(interval, span, update_frequency);

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

TEST(ForexUtilitiesTests, Validate_User_Settings_Negative_1_Test)
{
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
}

TEST(ForexUtilitiesTests, Validate_User_Settings_Negative_2_Test)
{
    fxordermgmt::FXUtilities fx_utils;

    std::string interval = "minute";
    int span = 100;
    int update_frequency {};

    auto response = fx_utils.validate_user_settings(interval, span, update_frequency);

    if (! response)
    {
        EXPECT_EQ(typeid(response.error()), typeid(fxordermgmt::FXException));
        EXPECT_EQ(std::string(response.error().what()), "Span Minute Error - Provide one of the following spans: 1, 2, 3, 5, 10, 15, 30");
    }
    else
    {
        FAIL();
    }
}

TEST(ForexUtilitiesTests, Validate_User_Settings_Negative_3_Test)
{
    fxordermgmt::FXUtilities fx_utils;

    std::string interval = "hour";
    int span = 10;
    int update_frequency {};

    auto response = fx_utils.validate_user_settings(interval, span, update_frequency);

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

TEST(ForexUtilitiesTests, Initialize_Logging_Correct)
{
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

TEST(ForexUtilitiesTests, Get_Todays_Date_Correct)
{
    fxordermgmt::FXUtilities fx_utils;

    EXPECT_EQ(fx_utils.get_todays_date(), get_todays_date());
}

TEST(ForexUtilitiesTests, Get_Todays_Date_Negative)
{
    fxordermgmt::FXUtilities fx_utils;

    EXPECT_FALSE(fx_utils.get_todays_date() == "");
}

TEST(ForexMainUtilitiesTests, Validate_Main_Parameters_Correct_1_Test)
{
    std::string ACCOUNT = "PAPER";
    bool PLACE_TRADES = true, EMERGENCY_CLOSE = false, FILE_LOGGING = true;
    int MAX_RETRY_FAILURES = 3;

    int argc = 1;
    char prog_name[] = "Program_Name";
    char* argv[] = {prog_name};

    EXPECT_TRUE(fxordermgmt::validateMainParameters(argc, argv, ACCOUNT, MAX_RETRY_FAILURES, PLACE_TRADES, EMERGENCY_CLOSE, FILE_LOGGING));
}

TEST(ForexMainUtilitiesTests, Validate_Main_Parameters_Correct_2_Test)
{
    std::string ACCOUNT = "PAPER";
    bool PLACE_TRADES = true, EMERGENCY_CLOSE = false, FILE_LOGGING = true;
    int MAX_RETRY_FAILURES = 3;

    int argc = 1;
    char prog_name[] = "Program_Name";

    char paper_or_live[] = "PAPER";
    char account_flag[] = "-a";
    char* argv[] = {prog_name, account_flag, paper_or_live};

    EXPECT_TRUE(fxordermgmt::validateMainParameters(argc, argv, ACCOUNT, MAX_RETRY_FAILURES, PLACE_TRADES, EMERGENCY_CLOSE, FILE_LOGGING));
    //EXPECT_EQ(account, "PAPER");
}

TEST(ForexMainUtilitiesTests, Validate_Main_Parameters_Correct_3_Test)
{
    std::string ACCOUNT = "PAPER";
    bool PLACE_TRADES = true, EMERGENCY_CLOSE = false, FILE_LOGGING = true;
    int MAX_RETRY_FAILURES = 3;

    int argc = 5;
    char prog_name[] = "Program_Name";
    char paper_or_live[] = "PAPER";
    char account_flag[] = "-a";
    char place_trade_flag[] = "-p";
    char place_trade_str[] = "trUE";
    char* argv[] = {prog_name, account_flag, paper_or_live, place_trade_flag, place_trade_str};

    EXPECT_TRUE(fxordermgmt::validateMainParameters(argc, argv, ACCOUNT, MAX_RETRY_FAILURES, PLACE_TRADES, EMERGENCY_CLOSE, FILE_LOGGING));
    //EXPECT_EQ(account, "PAPER");
    //EXPECT_TRUE(place_trades);
}

TEST(ForexMainUtilitiesTests, Validate_Main_Parameters_Correct_4_Test)
{
    std::string ACCOUNT = "PAPER";
    bool PLACE_TRADES = true, EMERGENCY_CLOSE = false, FILE_LOGGING = true;
    int MAX_RETRY_FAILURES = 3;

    int argc = 5;
    char prog_name[] = "Program_Name";
    char paper_or_live[] = "PAPER";
    char account_flag[] = "-a";
    char place_trade_flag[] = "-p";
    char place_trade_str2[] = "1";
    char* argv[] = {prog_name, account_flag, paper_or_live, place_trade_flag, place_trade_str2};

    EXPECT_TRUE(fxordermgmt::validateMainParameters(argc, argv, ACCOUNT, MAX_RETRY_FAILURES, PLACE_TRADES, EMERGENCY_CLOSE, FILE_LOGGING));
   // EXPECT_EQ(account, "PAPER");
    //EXPECT_TRUE(place_trades);
}

TEST(ForexMainUtilitiesTests, Validate_Main_Parameters_Correct_5_Test)
{
    std::string ACCOUNT = "PAPER";
    bool PLACE_TRADES = true, EMERGENCY_CLOSE = false, FILE_LOGGING = true;
    int MAX_RETRY_FAILURES = 3;

    int argc = 7;
    char prog_name[] = "Program_Name";
    char paper_or_live[] = "PAPER";
    char account_flag[] = "-a";
    char place_trade_flag[] = "-p";
    char place_trade_str2[] = "1";
    char max_retrys_flag[] = "-m";
    char max_retrys_str[] = "5";
    char* argv[] = {prog_name, account_flag, paper_or_live, place_trade_flag, place_trade_str2, max_retrys_flag, max_retrys_str};

    EXPECT_TRUE(fxordermgmt::validateMainParameters(argc, argv, ACCOUNT, MAX_RETRY_FAILURES, PLACE_TRADES, EMERGENCY_CLOSE, FILE_LOGGING));
    //EXPECT_EQ(account, "PAPER");
    //EXPECT_TRUE(place_trades);
    //EXPECT_EQ(max_retry_failures, 5);
}

TEST(ForexMainUtilitiesTests, Validate_Main_Parameters_Negative_1_Test)
{
    std::string ACCOUNT = "PAPER";
    bool PLACE_TRADES = true, EMERGENCY_CLOSE = false, FILE_LOGGING = true;
    int MAX_RETRY_FAILURES = 3;

    // Too Many Arguments
    int argc = 10;
    char prog_name[] = "Program_Name";
    char* argv[] = {prog_name};

    EXPECT_FALSE(fxordermgmt::validateMainParameters(argc, argv, ACCOUNT, MAX_RETRY_FAILURES, PLACE_TRADES, EMERGENCY_CLOSE, FILE_LOGGING));
}

TEST(ForexMainUtilitiesTests, Validate_Main_Parameters_Negative_2_Test)
{
    std::string ACCOUNT = "PAPER";
    bool PLACE_TRADES = true, EMERGENCY_CLOSE = false, FILE_LOGGING = true;
    int MAX_RETRY_FAILURES = 3;

    // Wrong Flag Type
    int argc = 3;
    char prog_name[] = "Program_Name";
    char paper_or_live[] = "PAPER";
    char account_flag_wrong[] = "-t";
    char* argv[] = {prog_name, account_flag_wrong, paper_or_live};

    EXPECT_FALSE(fxordermgmt::validateMainParameters(argc, argv, ACCOUNT, MAX_RETRY_FAILURES, PLACE_TRADES, EMERGENCY_CLOSE, FILE_LOGGING));
}

TEST(ForexMainUtilitiesTests, Validate_Main_Parameters_Negative_3_Test)
{
    std::string ACCOUNT = "PAPER";
    bool PLACE_TRADES = true, EMERGENCY_CLOSE = false, FILE_LOGGING = true;
    int MAX_RETRY_FAILURES = 3;

    // Wrong Place Trades Value
    int argc = 3;
    char prog_name[] = "Program_Name";
    char place_trade_flag[] = "-p";
    char place_trade_str_wrong[] = "X";
    char* argv[] = {prog_name, place_trade_flag, place_trade_str_wrong};

    EXPECT_FALSE(fxordermgmt::validateMainParameters(argc, argv, ACCOUNT, MAX_RETRY_FAILURES, PLACE_TRADES, EMERGENCY_CLOSE, FILE_LOGGING));
}

TEST(ForexMainUtilitiesTests, Validate_Main_Parameters_Negative_4_Test)
{
    std::string ACCOUNT = "PAPER";
    bool PLACE_TRADES = true, EMERGENCY_CLOSE = false, FILE_LOGGING = true;
    int MAX_RETRY_FAILURES = 3;

    // Wrong Max Retry Failures Value
    int argc = 3;
    char prog_name[] = "Program_Name";
    char max_retrys_flag[] = "-m";
    char max_retrys_str_wrong[] = "ABCD";
    char* argv[] = {prog_name, max_retrys_flag, max_retrys_str_wrong};

    EXPECT_FALSE(fxordermgmt::validateMainParameters(argc, argv, ACCOUNT, MAX_RETRY_FAILURES, PLACE_TRADES, EMERGENCY_CLOSE, FILE_LOGGING));
}

}// namespace