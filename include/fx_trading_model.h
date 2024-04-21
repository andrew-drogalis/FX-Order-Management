// Copyright 2024, Andrew Drogalis
// GNU License

#ifndef FX_TRADING_MODEL_H
#define FX_TRADING_MODEL_H

#include <vector>// for vector

namespace fxordermgmt
{

class FXTradingModel
{
  public:
    FXTradingModel() = delete;

    FXTradingModel(std::vector<float> const& open_prices, std::vector<float> const& high_prices, std::vector<float> const& low_prices,
                   std::vector<float> const& close_prices, std::vector<float> const& date_time);

    [[nodiscard]] int send_trading_signal();

  private:
    std::vector<float> const& open_prices;
    std::vector<float> const& high_prices;
    std::vector<float> const& low_prices;
    std::vector<float> const& close_prices;
    std::vector<float> const& date_time;
};

}// namespace fxordermgmt

#endif
