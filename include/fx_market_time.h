// Copyright 2024, Andrew Drogalis
// GNU License

#ifndef FX_MARKET_TIME_H
#define FX_MARKET_TIME_H

#include <cstddef> // for size_t
#include <expected>// for expected
#include <string>  // for hash, string, allocator

#include "fx_exception.h"// for FXException

namespace fxordermgmt
{

class FXMarketTime
{

  public:
    int tz_offset, FX_market_start, FX_market_end, market_close_time;

    FXMarketTime() noexcept = default;

    FXMarketTime(int start_hr, int end_hr, int update_frequency_seconds) noexcept;

    [[nodiscard]] std::expected<bool, FXException> initialize_forex_market_time();

    [[nodiscard]] int seconds_till_market_is_open(int start_days_adjustment, int end_days_adjustment);

    [[nodiscard]] bool is_market_open_today(std::string todays_date, int start_days_adjustment, int end_days_adjustment, int day_of_week);

    [[nodiscard]] bool is_market_closed() const noexcept;

    [[nodiscard]] bool is_forex_market_close_only() const noexcept;

    // ==============================================================================================
    // Testing
    // ==============================================================================================

    void enable_testing() noexcept;

  private:
    int start_hr, end_hr, update_frequency_seconds;
    bool fx_market_time_testing = false;

    void set_timezone_offset();

    void adjust_start_and_end_hours(int adjustment, int& start_days_adjustment, int& end_days_adjustment) noexcept;

    void wait_till_active_trading_day(int adjustment_seconds, int& start_days_adjustment, int& end_days_adjustment);

    void set_trading_time_bounds() noexcept;

    void pause_for_set_time(std::size_t seconds_to_wait) const noexcept;
};

}// namespace fxordermgmt

#endif
