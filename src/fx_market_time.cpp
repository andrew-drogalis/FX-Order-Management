// Copyright 2024, Andrew Drogalis
// GNU License

#include "fx_market_time.h"

#include <algorithm>    // for find
#include <bits/chrono.h>// for operator-, duration, operator+, system_clock
#include <chrono>       // for __time_zone_representation, zoned_time, cur...
#include <cmath>        // for round
#include <format>       // for format, format_string
#include <ratio>        // for ratio
#include <string>       // for allocator, basic_string, char_traits, opera...
#include <string_view>  // for basic_string_view
#include <time.h>       // for localtime, time, strftime, tm, time_t
#include <unistd.h>     // for sleep
#include <vector>       // for vector

#include "boost/log/trivial.hpp"              // for BOOST_LOG_TRIVIAL

namespace fxordermgmt
{

namespace ch = std::chrono;

FXMarketTime::FXMarketTime(int start_hr, int end_hr, int update_frequency_seconds) noexcept
    : start_hr(start_hr), end_hr(end_hr), update_frequency_seconds(update_frequency_seconds)
{
}

bool FXMarketTime::forex_market_time_setup()
{
    if (end_hr <= start_hr)
    {
        BOOST_LOG_TRIVIAL(error) << "Fatal Error: End Time is less than Start Time.";
        return false;
    }
    if (end_hr > 24 || end_hr < 0 || start_hr > 24 || start_hr < 0)
    {
        BOOST_LOG_TRIVIAL(error) << "Fatal Error: Provide Value Between 0 and 24.";
        return false;
    }
    if (fx_testing) { return true; }
    // --------
    set_timezone_offset();
    int start_days_adjustment = 0, end_days_adjustment = 0;
    correct_start_and_end_hours(tz_offset, start_days_adjustment, end_days_adjustment);
    // --------
    wait_till_market_is_open(start_days_adjustment, end_days_adjustment);
    set_trading_time_bounds(tz_offset_seconds);
    // No Errors -> return true;
    return true;
}

void FXMarketTime::set_timezone_offset()
{
    ch::zoned_time const local_time = ch::zoned_time {ch::current_zone(), ch::system_clock::now()};
    ch::zoned_time const london_time = ch::zoned_time {"Europe/London", ch::system_clock::now()};
    // --------
    tz_offset = (std::stoi(std::format("{:%z}", local_time)) - std::stoi(std::format("{:%z}", london_time))) / 100;
    tz_offset_seconds = -tz_offset * 3600;
}

void FXMarketTime::correct_start_and_end_hours(int offset, int& start_days_adjustment, int& end_days_adjustment) noexcept
{
    start_hr += offset % 24;
    end_hr += offset % 24;
    // --------
    if (start_hr < 0)
    {
        start_hr += 24;
        start_days_adjustment = -1;
        if (end_hr < 0)
        {
            end_hr += 24;
            end_days_adjustment = -1;
        }
    }
    if (end_hr >= 24)
    {
        end_hr -= 24;
        end_days_adjustment = 1;
        if (start_hr >= 24)
        {
            start_hr -= 24;
            start_days_adjustment = 1;
        }
    }
}

void FXMarketTime::wait_till_market_is_open(int& start_days_adjustment, int& end_days_adjustment)
{
    time_t now = time(0);
    struct tm* now_local = localtime(&now);
    int const local_hour = now_local->tm_hour;
    // --------
    if ((start_days_adjustment == end_days_adjustment && local_hour < end_hr && start_hr <= local_hour) ||
        (start_days_adjustment != end_days_adjustment && (local_hour < end_hr || start_hr <= local_hour)))
    {
        // Pass - Market is Currently Open
    }
    else
    {
        // Wait till market starts
        int const seconds_to_wait = (local_hour < start_hr) ? (start_hr - local_hour) * 3600 : (start_hr + 24 - local_hour) * 3600;
        pause_till_market_open(seconds_to_wait);
    }
    // --------
    bool market_is_open = true;
    do {
        // Pause for 24 Hours
        if (! market_is_open) { pause_till_market_open(86400); }
        // Keep this information accurate by re-calculating
        int const temp_tz_offset = tz_offset;
        set_timezone_offset();
        if (temp_tz_offset != tz_offset)
        {
            int difference = tz_offset - temp_tz_offset;
            correct_start_and_end_hours(difference, start_days_adjustment, end_days_adjustment);
        }
        now = time(0);
        now_local = localtime(&now);
        int const day_of_week = now_local->tm_wday;
        char DATE_TODAY[50];
        strftime(DATE_TODAY, sizeof(DATE_TODAY), "%Y_%m_%d", now_local);
        market_is_open = is_market_open_today(DATE_TODAY, start_days_adjustment, end_days_adjustment, day_of_week);
    } while (! market_is_open);
}

bool FXMarketTime::is_market_open_today(std::string const& todays_date, int start_days_adjustment, int end_days_adjustment, int day_of_week)
{
    // Holidays Must Be Updated as Required
    std::vector<std::string> holidays = {"2024_01_01", "2024_01_15", "2024_02_19", "2024_03_29", "2024_05_27", "2024_06_19",
                                         "2024_07_04", "2024_09_02", "2024_11_28", "2024_11_29", "2024_12_24", "2024_12_25"};
    std::vector<int> days_of_week_to_trade_start = {0, 1, 2, 3, 4};
    std::vector<int> days_of_week_to_trade_end = {0, 1, 2, 3, 4};
    for (auto& item : days_of_week_to_trade_start) { item += start_days_adjustment; }
    for (auto& item : days_of_week_to_trade_end) { item += end_days_adjustment; }

    if ((std::find(days_of_week_to_trade_start.begin(), days_of_week_to_trade_start.end(), day_of_week) != days_of_week_to_trade_start.end() ||
         std::find(days_of_week_to_trade_end.begin(), days_of_week_to_trade_end.end(), day_of_week) != days_of_week_to_trade_end.end()) &&
        std::find(holidays.begin(), holidays.end(), todays_date) == holidays.end())
    {
        return true;
    }
    else { return false; }
}

void FXMarketTime::set_trading_time_bounds(int offset_seconds) noexcept
{
    /* Set the Official Start & End Times
       FX Trading will stop (2) minutes before official end time */
    ch::time_point time_now = ch::system_clock::now();
    int const trade_hours_duration = end_hr - start_hr;
    // --------
    FX_market_start = ch::floor<ch::seconds>(time_now).time_since_epoch().count() + offset_seconds;
    FX_market_end = ((ch::floor<ch::seconds>(time_now) + ch::hours {trade_hours_duration - 1} + ch::minutes {58}).time_since_epoch()).count() * 60 +
                    offset_seconds;
    market_close_time = ((ch::floor<ch::seconds>(time_now) + ch::hours {trade_hours_duration}).time_since_epoch()).count() * 3600 + offset_seconds;
}

void FXMarketTime::set_testing_parameters() noexcept
{
    fx_testing = true;
    std::size_t const time_now = (ch::system_clock::now().time_since_epoch()).count() * ch::system_clock::period::num / ch::system_clock::period::den;
    FX_market_start = time_now;
    FX_market_end = market_close_time = time_now + (2 * update_frequency_seconds) + 10;
}

void FXMarketTime::pause_till_market_open(int seconds_to_wait) const noexcept
{
    BOOST_LOG_TRIVIAL(info) << "Market Closed; Waiting for Market Open; Will be waiting for " << roundf(seconds_to_wait / 36) / 100 << " Hours...\n";
    if (seconds_to_wait > 0) { sleep(seconds_to_wait); }
}

bool FXMarketTime::is_market_closed() const noexcept
{
    std::size_t const time_now = (ch::system_clock::now().time_since_epoch()).count() * ch::system_clock::period::num / ch::system_clock::period::den;
    if (market_close_time <= time_now) { return true; }
    else { return false; }
}

bool FXMarketTime::is_forex_market_close_only() const noexcept
{
    std::size_t const time_now = (ch::system_clock::now().time_since_epoch()).count() * ch::system_clock::period::num / ch::system_clock::period::den;
    if (FX_market_start <= time_now && time_now <= FX_market_end) { return false; }
    else { return true; }
}

}// namespace fxordermgmt
