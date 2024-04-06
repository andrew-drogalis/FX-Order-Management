// Copyright 2024, Andrew Drogalis
// GNU License

#ifndef FX_MARKET_TIME_H
#define FX_MARKET_TIME_H

#include "fx_utilities.h"

namespace fxordermgmt
{

class FXMarketTime
{

  public:
    FXMarketTime();

    FXMarketTime(int update_frequency_seconds, FXUtilities fx_utilities);

    void forex_market_time_setup();

    bool market_closed();

    bool forex_market_exit_only();

    bool pause_till_market_open();

  private:
    // Forex Time
    long unsigned int FX_market_start = 0;
    long unsigned int FX_market_end = 0;
    long unsigned int market_close_time = 0;
    int update_frequency_seconds = 0;
    bool will_market_be_open_tomorrow = false;
    FXUtilities fx_utilities;
};

}// namespace fxordermgmt

#endif
