// Copyright 2024, Andrew Drogalis
// GNU License

#include "fx_trading_model.h"

#include <stdlib.h>// for rand, RAND_MAX
#include <vector>  // for vector

namespace fxordermgmt
{

FXTradingModel::FXTradingModel(std::vector<float> const& open_prices, std::vector<float> const& high_prices, std::vector<float> const& low_prices,
                               std::vector<float> const& close_prices, std::vector<float> const& date_time)
    : open_prices(open_prices), high_prices(high_prices), low_prices(low_prices), close_prices(close_prices), date_time(date_time)
{
}

int FXTradingModel::send_trading_signal()
{
    // User's code to Buy / Sell
    int const signal = rand() % 2;
    return (signal) ? 1 : -1;
}

}// namespace fxordermgmt
