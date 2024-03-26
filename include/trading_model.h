// Copyright 2024, Andrew Drogalis
// GNU License

#ifndef TRADING_MODEL_H
#define TRADING_MODEL_H

#include <unordered_map>
#include <vector>
#include <string>

namespace fxordermgmt {

class TradingModel {

    public:

        TradingModel();

        ~TradingModel();

        TradingModel(std::unordered_map<std::string, std::vector<float>> historical_data);

        int send_trading_signal();

        void receive_latest_market_data(std::unordered_map<std::string, std::vector<float>> historical_data);

};

} // namespace fxordermgmt

#endif