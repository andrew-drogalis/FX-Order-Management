// Copyright 2024, Andrew Drogalis
// GNU License

#include "trading_model.h"

#include <stdlib.h>// for rand, RAND_MAX
#include <vector>  // for vector

namespace fxordermgmt
{

TradingModel::TradingModel(const std::vector<float>& openPrices, const std::vector<float>& highPrices, const std::vector<float>& lowPrices,
                           const std::vector<float>& closePrices, const std::vector<float>& dateTime)
    : openPrices(openPrices), highPrices(highPrices), lowPrices(lowPrices), closePrices(closePrices), dateTime(dateTime)
{
}

void TradingModel::receive_latest_market_data(const std::vector<float>& openPrices, const std::vector<float>& highPrices,
                                              const std::vector<float>& lowPrices, const std::vector<float>& closePrices,
                                              const std::vector<float>& dateTime)
{
    // Update the market data with each update interval
    this->openPrices = openPrices;
    this->highPrices = highPrices;
    this->lowPrices = lowPrices;
    this->closePrices = closePrices;
    this->dateTime = dateTime;
}

int TradingModel::send_trading_signal()
{
    // User's code to Buy / Sell
    double const signal = static_cast<double>(rand()) / RAND_MAX;
    return (signal > 0.5) ? 1 : -1;
}

}// namespace fxordermgmt
