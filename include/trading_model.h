// Copyright 2024, Andrew Drogalis
// GNU License

#ifndef TRADING_MODEL_H
#define TRADING_MODEL_H

#include <vector>// for vector

namespace fxordermgmt
{

class TradingModel
{
  public:
    TradingModel() = default;

    TradingModel(std::vector<float> const& openPrices, std::vector<float> const& highPrices, std::vector<float> const& lowPrices,
                 std::vector<float> const& closePrices, std::vector<float> const& dateTime);

    void receive_latest_market_data(std::vector<float> const& openPrices, std::vector<float> const& highPrices, std::vector<float> const& lowPrices,
                                    std::vector<float> const& closePrices, std::vector<float> const& dateTime);

    [[nodiscard]] int send_trading_signal();

  private:
    std::vector<float> openPrices;
    std::vector<float> highPrices;
    std::vector<float> lowPrices;
    std::vector<float> closePrices;
    std::vector<float> dateTime;
};

}// namespace fxordermgmt

#endif
