// Copyright 2024, Andrew Drogalis
// GNU License

#ifndef TRADING_MODEL_H
#define TRADING_MODEL_H

#include <string>
#include <unordered_map>
#include <vector>

namespace fxordermgmt
{

class TradingModel
{

  public:
    TradingModel();

    TradingModel(std::unordered_map<std::string, std::vector<float>> historical_data);

    int send_trading_signal();

    void receive_latest_market_data(std::unordered_map<std::string, std::vector<float>> historical_data);

  private:
    std::vector<float> open_data;
    std::vector<float> high_data;
    std::vector<float> low_data;
    std::vector<float> close_data;
    std::vector<float> datetime_data;
};

}// namespace fxordermgmt

#endif
