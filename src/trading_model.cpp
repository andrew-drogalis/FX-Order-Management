
#include <trading_model.h>
#include <map>
#include <vector>
#include <string>

using namespace std;

TradingModel::TradingModel() { }

TradingModel::TradingModel(map<string, vector<float>> historical_data){

    vector<float> open_data = historical_data["Open"];
    vector<float> high_data = historical_data["High"];
    vector<float> low_data = historical_data["Low"];
    vector<float> close_data = historical_data["Close"];
    vector<float> datetime_data = historical_data["Datetime"];

}

void TradingModel::receive_latest_market_data(map<string, vector<float>> historical_data) {

}

int TradingModel::send_trading_signal(){

    float signal = ((double) rand() / (RAND_MAX));

    signal = (signal > 0.5) ? 1 : -1;

    return signal;
}

