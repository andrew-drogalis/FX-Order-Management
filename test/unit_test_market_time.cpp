// Copyright 2024, Andrew Drogalis
// GNU License

#include <string>
#include <typeinfo>

#include "gtest/gtest.h"

#include "fx_market_time.h"
#include "fx_exception.h"

namespace {


TEST(ForexMarketTimeTests, Initialize_Forex_Market_Time_Correct) {
    // Flipped Start_Hr & End_Hr
    int const update_seconds_interval = 60, start_hr = 18, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime{start_hr, end_hr, update_seconds_interval};

    fxTime.enable_testing();
    
    auto response = fxTime.initialize_forex_market_time();

    if (response)
    {
        // EXPECT_EQ(typeid(response.error()), typeid(fxordermgmt::FXException));
        // EXPECT_EQ(std::string(response.error().what()), "End Time is less than Start Time");
    }
    else
    {
        FAIL();
    }   
}


TEST(ForexMarketTimeTests, Initialize_Forex_Market_Time_1Fail_Early) {
    // Flipped Start_Hr & End_Hr
    int const update_seconds_interval = 60, start_hr = 18, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime{start_hr, end_hr, update_seconds_interval};

    fxTime.enable_testing();
    
    auto response = fxTime.initialize_forex_market_time();

    if (! response)
    {
        EXPECT_EQ(typeid(response.error()), typeid(fxordermgmt::FXException));
        EXPECT_EQ(std::string(response.error().what()), "End Time is less than Start Time");
    }
    else
    {
        FAIL();
    }   
}


TEST(ForexMarketTimeTests, Initialize_Forex_Market_Time_2Fail_Early) {
    // Negative Start Hr
    int const update_seconds_interval = 60, start_hr = -18, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime{start_hr, end_hr, update_seconds_interval};

    fxTime.enable_testing();
    
    auto response = fxTime.initialize_forex_market_time();

    if (! response)
    {
        EXPECT_EQ(typeid(response.error()), typeid(fxordermgmt::FXException));
        EXPECT_EQ(std::string(response.error().what()), "Provide Value Between 0 and 24.");
    }
    else
    {
        FAIL();
    }   
}


TEST(ForexMarketTimeTests, Initialize_Forex_Market_Time_3Fail_Early) {
    // End Hr Over 24
    int const update_seconds_interval = 60, start_hr = 18, end_hr = 120;
    fxordermgmt::FXMarketTime fxTime{start_hr, end_hr, update_seconds_interval};

    fxTime.enable_testing();
    
    auto response = fxTime.initialize_forex_market_time();

    if (! response)
    {
        EXPECT_EQ(typeid(response.error()), typeid(fxordermgmt::FXException));
        EXPECT_EQ(std::string(response.error().what()), "Provide Value Between 0 and 24.");
    }
    else
    {
        FAIL();
    }   
}



} // namespace