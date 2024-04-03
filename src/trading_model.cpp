// Copyright 2024, Andrew Drogalis
// GNU License

#include "trading_model.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace fxordermgmt
{

TradingModel::TradingModel() {}

TradingModel::~TradingModel() {}

TradingModel::TradingModel(std::unordered_map<std::string, std::vector<float>> historical_data)
{
    open_data = historical_data["Open"];
    high_data = historical_data["High"];
    low_data = historical_data["Low"];
    close_data = historical_data["Close"];
    datetime_data = historical_data["Datetime"];
}

void TradingModel::receive_latest_market_data(std::unordered_map<std::string, std::vector<float>> historical_data)
{
    // User defined trading model
    open_data = historical_data["Open"];
    high_data = historical_data["High"];
    low_data = historical_data["Low"];
    close_data = historical_data["Close"];
    datetime_data = historical_data["Datetime"];
}

int TradingModel::send_trading_signal()
{
    float signal = ((double)rand() / (RAND_MAX));
    signal = (signal > 0.5) ? 1 : -1;
    return signal;
}

}// namespace fxordermgmt
