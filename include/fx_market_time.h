// Copyright 2024, Andrew Drogalis
// GNU License

#ifndef FX_MARKET_TIME_H
#define FX_MARKET_TIME_H

#include "fx_utilities.h"

namespace fxordermgmt {

class FXMarketTime {

    public:

        FXMarketTime();

        FXMarketTime(int update_frequency_seconds);

        ~FXMarketTime();

        void forex_market_time_setup();

        bool market_closed();

        bool forex_market_exit_only();
        
        void pause_till_market_open();

    private:
        // Forex Time 
        long unsigned int FX_market_start;
        long unsigned int FX_market_end;
        long unsigned int market_close_time;
        int update_frequency_seconds;
        bool will_market_be_open_tomorrow;

        FXUtilities fx_utilities = FXUtilities();

};

}

#endif