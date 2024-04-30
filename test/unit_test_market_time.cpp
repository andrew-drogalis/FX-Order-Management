// Copyright 2024, Andrew Drogalis
// GNU License

#include <time.h>// for localtime, time, strftime, tm, time_t

#include <chrono>
#include <string>
#include <typeinfo>

#include "gtest/gtest.h"

#include "fx_exception.h"
#include "fx_market_time.h"

namespace
{

TEST(ForexMarketTimeTests, Initialize_Forex_Market_Time_1Correct)
{
    int const update_seconds_interval = 60, start_hr = 10, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};

    fxTime.enable_testing();
    fxTime.tz_offset = -5;

    auto response = fxTime.wait_till_forex_market_is_open();

    if (response)
    {
        fxTime.FX_market_end;
        fxTime.FX_market_start;
        fxTime.market_close_time;
        // EXPECT_EQ(typeid(response.error()), typeid(fxordermgmt::FXException));
        // EXPECT_EQ(std::string(response.error().what()), "End Time is less than Start Time");
    }
    else
    {
        FAIL();
    }
}

TEST(ForexMarketTimeTests, Initialize_Forex_Market_Time_2Correct)
{
    int const update_seconds_interval = 60, start_hr = 8, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};

    fxTime.enable_testing();
    fxTime.tz_offset = 12;

    auto response = fxTime.wait_till_forex_market_is_open();

    if (response)
    {
        fxTime.FX_market_end;
        fxTime.FX_market_start;
        fxTime.market_close_time;
        // EXPECT_EQ(typeid(response.error()), typeid(fxordermgmt::FXException));
        // EXPECT_EQ(std::string(response.error().what()), "End Time is less than Start Time");
    }
    else
    {
        FAIL();
    }
}

TEST(ForexMarketTimeTests, Initialize_Forex_Market_Time_3Correct)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};

    fxTime.enable_testing();
    fxTime.tz_offset = -12;

    auto response = fxTime.wait_till_forex_market_is_open();

    if (response)
    {
        fxTime.FX_market_end;
        fxTime.FX_market_start;
        fxTime.market_close_time;
        // EXPECT_EQ(typeid(response.error()), typeid(fxordermgmt::FXException));
        // EXPECT_EQ(std::string(response.error().what()), "End Time is less than Start Time");
    }
    else
    {
        FAIL();
    }
}

TEST(ForexMarketTimeTests, Initialize_Forex_Market_Time_1Fail_Early)
{
    // Flipped Start_Hr & End_Hr
    int const update_seconds_interval = 60, start_hr = 18, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};

    fxTime.enable_testing();

    auto response = fxTime.wait_till_forex_market_is_open();

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

TEST(ForexMarketTimeTests, Initialize_Forex_Market_Time_2Fail_Early)
{
    // Negative Start Hr
    int const update_seconds_interval = 60, start_hr = -18, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};

    fxTime.enable_testing();

    auto response = fxTime.wait_till_forex_market_is_open();

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

TEST(ForexMarketTimeTests, Initialize_Forex_Market_Time_3Fail_Early)
{
    // End Hr Over 24
    int const update_seconds_interval = 60, start_hr = 18, end_hr = 120;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};

    fxTime.enable_testing();

    auto response = fxTime.wait_till_forex_market_is_open();

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

// ===================================================================================================================

TEST(ForexMarketTimeTests, Seconds_Till_Market_Is_Open_1Correct)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};

    int start_days_adjustment = 0, end_days_adjustment = 0;
    int seconds_till_open = fxTime.seconds_till_market_is_open(start_days_adjustment, end_days_adjustment);
}

TEST(ForexMarketTimeTests, Seconds_Till_Market_Is_Open_2Correct)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};

    int start_days_adjustment = 0, end_days_adjustment = 0;
    int seconds_till_open = fxTime.seconds_till_market_is_open(start_days_adjustment, end_days_adjustment);
}

TEST(ForexMarketTimeTests, Seconds_Till_Market_Is_Open_3Correct)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};

    int start_days_adjustment = 0, end_days_adjustment = 0;
    int seconds_till_open = fxTime.seconds_till_market_is_open(start_days_adjustment, end_days_adjustment);
}

TEST(ForexMarketTimeTests, Seconds_Till_Market_Is_Open_4Correct)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};

    int start_days_adjustment = 0, end_days_adjustment = 0;
    int seconds_till_open = fxTime.seconds_till_market_is_open(start_days_adjustment, end_days_adjustment);
}

TEST(ForexMarketTimeTests, Seconds_Till_Market_Is_Open_5Correct)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};

    int start_days_adjustment = 0, end_days_adjustment = 0;
    int seconds_till_open = fxTime.seconds_till_market_is_open(start_days_adjustment, end_days_adjustment);
}

// ===================================================================================================================

TEST(ForexMarketTimeTests, Seconds_Till_Market_Open_Tomorrow_1Correct)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};

    int adjustment_seconds = 0, start_days_adjustment = 0, end_days_adjustment = 0;
    int seconds_till_open_tmrw = fxTime.seconds_till_market_open_tomorrow(adjustment_seconds, start_days_adjustment, end_days_adjustment);
}

TEST(ForexMarketTimeTests, Seconds_Till_Market_Open_Tomorrow_2Correct)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};

    int adjustment_seconds = 0, start_days_adjustment = 0, end_days_adjustment = 0;
    int seconds_till_open_tmrw = fxTime.seconds_till_market_open_tomorrow(adjustment_seconds, start_days_adjustment, end_days_adjustment);
}

TEST(ForexMarketTimeTests, Seconds_Till_Market_Open_Tomorrow_3Correct)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};

    int adjustment_seconds = 0, start_days_adjustment = 0, end_days_adjustment = 0;
    int seconds_till_open_tmrw = fxTime.seconds_till_market_open_tomorrow(adjustment_seconds, start_days_adjustment, end_days_adjustment);
}

TEST(ForexMarketTimeTests, Seconds_Till_Market_Open_Tomorrow_4Correct)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};

    int adjustment_seconds = 0, start_days_adjustment = 0, end_days_adjustment = 0;
    int seconds_till_open_tmrw = fxTime.seconds_till_market_open_tomorrow(adjustment_seconds, start_days_adjustment, end_days_adjustment);
}

// ===================================================================================================================

TEST(ForexMarketTimeTests, Is_Market_Open_Today_1Correct)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};
}

TEST(ForexMarketTimeTests, Is_Market_Open_Today_2Correct)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};
}

TEST(ForexMarketTimeTests, Is_Market_Open_Today_3Correct)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};
}

TEST(ForexMarketTimeTests, Is_Market_Open_Today_4Correct)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};
}

TEST(ForexMarketTimeTests, Is_Market_Open_Today_5Correct)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};
}

TEST(ForexMarketTimeTests, Is_Market_Open_Today_1Incorrect)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};
}

TEST(ForexMarketTimeTests, Is_Market_Open_Today_2Incorrect)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};
}

TEST(ForexMarketTimeTests, Is_Market_Open_Today_3Incorrect)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};
}

TEST(ForexMarketTimeTests, Is_Market_Open_Today_4Incorrect)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};
}

TEST(ForexMarketTimeTests, Is_Market_Open_Today_5Incorrect)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};
}

// ===================================================================================================================

TEST(ForexMarketTimeTests, Is_Market_Closed_1True)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};
}

TEST(ForexMarketTimeTests, Is_Market_Closed_2True)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};
}

TEST(ForexMarketTimeTests, Is_Market_Closed_1False)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};
}

TEST(ForexMarketTimeTests, Is_Market_Closed_2False)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};
}

TEST(ForexMarketTimeTests, Is_Market_Closed_3False)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};
}

TEST(ForexMarketTimeTests, Is_Market_Closed_4False)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};
}

// ===================================================================================================================

TEST(ForexMarketTimeTests, Is_Close_Only_1True)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};
}

TEST(ForexMarketTimeTests, Is_Close_Only_2True)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};
}

TEST(ForexMarketTimeTests, Is_Close_Only_3True)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};
}

TEST(ForexMarketTimeTests, Is_Close_Only_1False)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};
}

TEST(ForexMarketTimeTests, Is_Close_Only_2False)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};
}

TEST(ForexMarketTimeTests, Is_Close_Only_3False)
{
    int const update_seconds_interval = 60, start_hr = 5, end_hr = 12;
    fxordermgmt::FXMarketTime fxTime {start_hr, end_hr, update_seconds_interval};
}

}// namespace