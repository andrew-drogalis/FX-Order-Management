// Copyright 2024, Andrew Drogalis
// GNU License

#include "fx_market_time.h"

#include "fx_utilities.h"

#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

namespace fxordermgmt
{

namespace ch = std::chrono;

FXMarketTime::FXMarketTime() {}

FXMarketTime::FXMarketTime(int update_frequency_seconds, FXUtilities fx_utilities)
    : update_frequency_seconds(update_frequency_seconds), fx_utilities(fx_utilities)
{
}

FXMarketTime::~FXMarketTime() {}

void FXMarketTime::forex_market_time_setup()
{
    // 24 HR Start Trading Time & End Trading Time
    // In London Time 8 = 8:00 AM London Time
    int start_hr = 8;
    int end_hr = 20;
    // ----------------------------------------
    time_t now = time(0);
    struct tm* now_local = localtime(&now);
    std::string date = fx_utilities.get_todays_date();
    int day_of_week = now_local->tm_wday;

    ch::zoned_time local_time = ch::zoned_time {ch::current_zone(), ch::system_clock::now()};
    ch::zoned_time london_time = ch::zoned_time {"Europe/London", ch::system_clock::now()};
    // --------
    int offset = (std::stoi(std::format("{:%z}", local_time)) - std::stoi(std::format("{:%z}", london_time))) / 100;
    int offset_seconds = -offset * 3600;
    start_hr += offset;
    end_hr += offset;
    // ----------------------------------------
    // Holidays Must Be Updated as Required
    std::vector<std::string> holidays = {"2024_01_01", "2024_01_15", "2024_02_19", "2024_03_29", "2024_05_27", "2024_06_19",
                                         "2024_07_04", "2024_09_02", "2024_11_28", "2024_11_29", "2024_12_24", "2024_12_25"};
    std::vector<int> days_of_week_to_trade = {6, 0, 1, 2, 3, 4};

    // Regular Market Hours
    if (std::find(days_of_week_to_trade.begin(), days_of_week_to_trade.end(), day_of_week) != days_of_week_to_trade.end() &&
        std::find(holidays.begin(), holidays.end(), date) == holidays.end())
    {
        will_market_be_open_tomorrow = true;
        ch::time_point time_now = ch::system_clock::now();

        if ((std::stoi(std::format("{:%H}", local_time)) < end_hr || day_of_week == 4) && day_of_week != 6)
        {
            FX_market_start = ((ch::floor<ch::days>(time_now) + ch::hours {start_hr}).time_since_epoch()).count() * 3600 + offset_seconds;
            FX_market_end =
                ((ch::floor<ch::days>(time_now) + ch::hours {end_hr - 1} + ch::minutes {58}).time_since_epoch()).count() * 60 + offset_seconds;
            market_close_time = ((ch::floor<ch::days>(time_now) + ch::hours {end_hr}).time_since_epoch()).count() * 3600 + offset_seconds;
        }
        else
        {
            time_now += ch::days {1};
            FX_market_start = ((ch::floor<ch::days>(time_now) + ch::hours {start_hr}).time_since_epoch()).count() * 3600 + offset_seconds;
            FX_market_end =
                ((ch::floor<ch::days>(time_now) + ch::hours {end_hr - 1} + ch::minutes {58}).time_since_epoch()).count() * 60 + offset_seconds;
            market_close_time = ((ch::floor<ch::days>(time_now) + ch::hours {end_hr}).time_since_epoch()).count() * 3600 + offset_seconds;
        }
    }
    // Market Closed
    else
    {
        will_market_be_open_tomorrow = false;
        FX_market_start = 0;
        FX_market_end = (ch::system_clock::now().time_since_epoch()).count() * ch::system_clock::period::num / ch::system_clock::period::den - 1;
        market_close_time = (ch::system_clock::now().time_since_epoch()).count() * ch::system_clock::period::num / ch::system_clock::period::den - 1;
    }
}

bool FXMarketTime::market_closed()
{
    unsigned int time_now = (ch::system_clock::now().time_since_epoch()).count() * ch::system_clock::period::num / ch::system_clock::period::den;
    if (market_close_time - 1 < time_now && time_now < market_close_time + update_frequency_seconds + 60) { return true; }
    else { return false; }
}

bool FXMarketTime::forex_market_exit_only()
{
    unsigned int time_now = (ch::system_clock::now().time_since_epoch()).count() * ch::system_clock::period::num / ch::system_clock::period::den;
    if (FX_market_start <= time_now && time_now <= FX_market_end) { return false; }
    else { return true; }
}

bool FXMarketTime::pause_till_market_open()
{
    bool market_is_closed = market_closed();
    bool forex_is_exit_only = forex_market_exit_only();

    if (! market_is_closed && forex_is_exit_only)
    {
        float time_to_wait_now =
            FX_market_start - (ch::system_clock::now().time_since_epoch()).count() * ch::system_clock::period::num / ch::system_clock::period::den;
        // Outpuit Status Message
        std::cout << "Market Closed Today; Waiting for Tomorrow; Will be waiting for " << round(time_to_wait_now / 36) / 100 << " Hours..."
                  << std::endl;
        // Pause Until Market Open
        if (time_to_wait_now > 0) { sleep(time_to_wait_now); }
        else { will_market_be_open_tomorrow = false; }
    }
    if (! will_market_be_open_tomorrow)
    {
        std::cout << "Market is Closed Today; Weekend Approaching; Terminating Program" << std::endl;
        return false;
    }
    return true;
}

}// namespace fxordermgmt