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

    FXMarketTime(int start_hr, int end_hr, int update_frequency_seconds);

    [[nodiscard]] bool forex_market_time_setup();

    void set_timezone_offset();

    void correct_start_and_end_offset(int offset, int& start_days_adjustment, int& end_days_adjustment);

    void is_local_time_within_market_hours(int& start_days_adjustment, int& end_days_adjustment);

    void set_trading_time_bounds(int offset_seconds);

    [[nodiscard]] bool is_market_open_today(const std::string& todays_date, int start_days_adjustment, int end_days_adjustment, int day_of_week);

    void set_testing_parameters() noexcept;

    void pause_till_market_open(float seconds_to_wait) const noexcept;

    [[nodiscard]] bool is_market_closed() const noexcept;

    [[nodiscard]] bool is_forex_market_close_only() const noexcept;

  private:
    std::size_t FX_market_start = 0;
    std::size_t FX_market_end = 0;
    std::size_t market_close_time = 0;
    int start_hr = 0;
    int end_hr = 0;
    int tz_offset = 0;
    int tz_offset_seconds = 0;
    int update_frequency_seconds = 0;
    bool fx_testing = false;
};

}// namespace fxordermgmt

#endif
