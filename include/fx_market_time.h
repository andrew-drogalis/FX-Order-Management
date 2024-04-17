// Copyright 2024, Andrew Drogalis
// GNU License

#ifndef FX_MARKET_TIME_H
#define FX_MARKET_TIME_H

#include <cstddef>// for size_t
#include <string> // for hash, string, allocator

namespace fxordermgmt
{

class FXMarketTime
{

  public:
    FXMarketTime() noexcept = default;

    FXMarketTime(int start_hr, int end_hr, int update_frequency_seconds) noexcept;

    [[nodiscard]] bool forex_market_time_setup();

    void set_timezone_offset();

    void correct_start_and_end_hours(int offset, int& start_days_adjustment, int& end_days_adjustment) noexcept;

    void wait_till_market_is_open(int& start_days_adjustment, int& end_days_adjustment);

    [[nodiscard]] bool is_market_open_today(std::string const& todays_date, int start_days_adjustment, int end_days_adjustment, int day_of_week);

    void set_trading_time_bounds(int offset_seconds) noexcept;

    void set_testing_parameters() noexcept;

    void pause_till_market_open(int seconds_to_wait) const noexcept;

    [[nodiscard]] bool is_market_closed() const noexcept;

    [[nodiscard]] bool is_forex_market_close_only() const noexcept;

  private:
    std::size_t FX_market_start, FX_market_end, market_close_time;
    int start_hr, end_hr, update_frequency_seconds;
    int tz_offset, tz_offset_seconds;
    bool fx_testing = false;
};

}// namespace fxordermgmt

#endif
