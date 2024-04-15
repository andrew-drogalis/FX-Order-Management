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

    TradingModel(const std::vector<float>& openPrices, const std::vector<float>& highPrices, const std::vector<float>& lowPrices,
                 const std::vector<float>& closePrices, const std::vector<float>& dateTime);

    void receive_latest_market_data(const std::vector<float>& openPrices, const std::vector<float>& highPrices, const std::vector<float>& lowPrices,
                                    const std::vector<float>& closePrices, const std::vector<float>& dateTime);

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
