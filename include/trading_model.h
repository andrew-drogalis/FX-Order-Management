#ifndef TRADING_MODEL_H
#define TRADING_MODEL_H

#include <map>
#include <vector>
#include <string>

namespace std {

class TradingModel {

    public:

        TradingModel();

        TradingModel(std::map<std::string, std::vector<float>> historical_data);

        ~TradingModel();

        int send_trading_signal();

        void receive_latest_market_data(std::map<std::string, std::vector<float>> historical_data);

};

}

#endif