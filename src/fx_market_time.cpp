// Copyright 2024, Andrew Drogalis
// GNU License

#include "fx_market_time.h"

#include <time.h>  // for localtime, time, strftime, tm, time_t
#include <unistd.h>// for sleep

#include <algorithm>      // for find
#include <array>          // for array
#include <chrono>         // for __time_zone_representation, zoned_time, cur...
#include <cmath>          // for round
#include <expected>       // for expected
#include <format>         // for format, format_string
#include <iostream>       // for cout
#include <ratio>          // for ratio
#include <source_location>// for current, function_name...
#include <string>         // for allocator, basic_string, char_traits, opera...
#include <string_view>    // for basic_string_view

#include "fx_exception.h"// for FXException

namespace
{
int get_day_of_week()
{
    time_t now = time(0);
    struct tm* now_local = localtime(&now);
    return now_local->tm_wday;
}

std::string get_todays_date()
{
    time_t now = time(0);
    struct tm* now_local = localtime(&now);
    char DATE_TODAY[50];
    strftime(DATE_TODAY, sizeof(DATE_TODAY), "%Y_%m_%d", now_local);
    return DATE_TODAY;
}
}// namespace

namespace fxordermgmt
{

namespace ch = std::chrono;

FXMarketTime::FXMarketTime(int start_hr, int end_hr, int update_frequency_seconds) noexcept
    : start_hr(start_hr), end_hr(end_hr), update_frequency_seconds(update_frequency_seconds)
{
}

std::expected<bool, FXException> FXMarketTime::initialize_forex_market_time()
{
    if (end_hr <= start_hr)
    {
        return std::expected<bool, FXException> {std::unexpect, std::source_location::current().function_name(), "End Time is less than Start Time."};
    }
    if (end_hr > 24 || end_hr < 0 || start_hr > 24 || start_hr < 0)
    {
        return std::expected<bool, FXException> {std::unexpect, std::source_location::current().function_name(), "Provide Value Between 0 and 24."};
    }
    // -------------------
    if (! fx_market_time_testing) { set_timezone_offset(); }

    int start_days_adjustment = 0, end_days_adjustment = 0;
    adjust_start_and_end_hours(tz_offset, start_days_adjustment, end_days_adjustment);

    int seconds_to_wait = seconds_till_market_is_open(start_days_adjustment, end_days_adjustment);
    if (seconds_to_wait > 0) { pause_for_set_time(seconds_to_wait); }

    wait_till_active_trading_day(seconds_to_wait, start_days_adjustment, end_days_adjustment);

    set_trading_time_bounds();
    // -------------------
    return std::expected<bool, FXException> {true};
}

void FXMarketTime::set_timezone_offset()
{
    ch::zoned_time const local_time = ch::zoned_time {ch::current_zone(), ch::system_clock::now()};
    ch::zoned_time const london_time = ch::zoned_time {"Europe/London", ch::system_clock::now()};
    // -------------------
    tz_offset = (std::stoi(std::format("{:%z}", local_time)) - std::stoi(std::format("{:%z}", london_time))) / 100;
}

void FXMarketTime::adjust_start_and_end_hours(int adjustment, int& start_days_adjustment, int& end_days_adjustment) noexcept
{
    start_hr += adjustment % 24;
    end_hr += adjustment % 24;
    // -------------------
    if (start_hr < 0)
    {
        start_hr += 24;
        start_days_adjustment = -1;
    }
    if (end_hr < 0)
    {
        end_hr += 24;
        end_days_adjustment = -1;
    }
    if (end_hr >= 24)
    {
        end_hr -= 24;
        end_days_adjustment = 1;
    }
    if (start_hr >= 24)
    {
        start_hr -= 24;
        start_days_adjustment = 1;
    }
}

int FXMarketTime::seconds_till_market_is_open(int start_days_adjustment, int end_days_adjustment)
{
    time_t now = time(0);
    struct tm* now_local = localtime(&now);
    double const local_time_hr = now_local->tm_hour + now_local->tm_min / 60.0 + now_local->tm_sec / 3600.0;
    // -------------------
    // Market is inside the Start Hr & End Hr. Negative seconds will only be used if the trading day is closed (Weekend, Holiday).
    if (start_days_adjustment == end_days_adjustment && local_time_hr < end_hr && start_hr <= local_time_hr)
    {
        return (start_hr - local_time_hr) * 3600;
    }
    else if (start_days_adjustment != end_days_adjustment && (local_time_hr < end_hr || start_hr <= local_time_hr))
    {
        return (local_time_hr < start_hr) ? (-24 + start_hr - local_time_hr) * 3600 : (start_hr - local_time_hr) * 3600;
    }
    // Market is Closed.
    else { return (local_time_hr < start_hr) ? (start_hr - local_time_hr) * 3600 : (start_hr + 24 - local_time_hr) * 3600; }
}

void FXMarketTime::wait_till_active_trading_day(int adjustment_seconds, int& start_days_adjustment, int& end_days_adjustment)
{
    bool market_is_open = is_market_open_today(get_todays_date(), start_days_adjustment, end_days_adjustment, get_day_of_week());
    // -------------------
    while (! market_is_open)
    {
        // Check if TX Offset changes due to DST
        int const temp_tz_offset = tz_offset;
        set_timezone_offset();
        int const DST_adjustment = tz_offset - temp_tz_offset;
        if (DST_adjustment) { adjust_start_and_end_hours(DST_adjustment, start_days_adjustment, end_days_adjustment); }

        int HOURS_24_TO_SECONDS = 86400;
        HOURS_24_TO_SECONDS -= DST_adjustment * 3600;
        if (adjustment_seconds < 0)
        {
            HOURS_24_TO_SECONDS += adjustment_seconds;
            adjustment_seconds = 0;
        }
        pause_for_set_time(HOURS_24_TO_SECONDS);
        // -------------------
        market_is_open = is_market_open_today(get_todays_date(), start_days_adjustment, end_days_adjustment, get_day_of_week());
    }
}

void FXMarketTime::set_trading_time_bounds() noexcept
{
    /* Set the Official Start & End Times
       FX Trading will stop (2) minutes before official end time */
    ch::time_point time_now = ch::system_clock::now();
    int const trade_hours_duration = end_hr - start_hr;
    int const tz_offset_seconds = tz_offset * 3600;

    FX_market_start = ch::floor<ch::seconds>(time_now).time_since_epoch().count() - tz_offset_seconds;
    FX_market_end = ((ch::floor<ch::seconds>(time_now) + ch::hours {trade_hours_duration - 1} + ch::minutes {58}).time_since_epoch()).count() * 60 -
                    tz_offset_seconds;
    market_close_time = ((ch::floor<ch::seconds>(time_now) + ch::hours {trade_hours_duration}).time_since_epoch()).count() * 3600 - tz_offset_seconds;
}

void FXMarketTime::pause_for_set_time(std::size_t seconds_to_wait) const noexcept
{
    std::cout << "Market Closed; Will check if market is open in " << round(seconds_to_wait / 36) / 100 << " Hours...\n";
    sleep(seconds_to_wait);
}

bool FXMarketTime::is_market_open_today(std::string todays_date, int start_days_adjustment, int end_days_adjustment, int day_of_week)
{
    // Holidays Must Be Updated as Required
    std::array<std::string, 12> const holidays = {"2024_01_01", "2024_01_15", "2024_02_19", "2024_03_29", "2024_05_27", "2024_06_19",
                                                  "2024_07_04", "2024_09_02", "2024_11_28", "2024_11_29", "2024_12_24", "2024_12_25"};
    std::array<int, 5> days_of_week_to_trade_start = {0, 1, 2, 3, 4};
    std::array<int, 5> days_of_week_to_trade_end = {0, 1, 2, 3, 4};
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

bool FXMarketTime::is_market_closed() const noexcept
{
    std::size_t const time_now = (ch::system_clock::now().time_since_epoch()).count() * ch::system_clock::period::num / ch::system_clock::period::den;
    // -------------------
    if (market_close_time <= time_now) { return true; }
    else { return false; }
}

bool FXMarketTime::is_forex_market_close_only() const noexcept
{
    std::size_t const time_now = (ch::system_clock::now().time_since_epoch()).count() * ch::system_clock::period::num / ch::system_clock::period::den;
    // -------------------
    if (FX_market_start <= time_now && time_now <= FX_market_end) { return false; }
    else { return true; }
}

// ==============================================================================================
// Testing
// ==============================================================================================

void FXMarketTime::enable_testing() noexcept
{
    fx_market_time_testing = true;
    std::size_t const time_now = (ch::system_clock::now().time_since_epoch()).count() * ch::system_clock::period::num / ch::system_clock::period::den;
    FX_market_start = time_now;
    // Allows for (2) Updates with a 10 second buffer.
    FX_market_end = market_close_time = time_now + (2 * update_frequency_seconds) + 10;
}

}// namespace fxordermgmt
