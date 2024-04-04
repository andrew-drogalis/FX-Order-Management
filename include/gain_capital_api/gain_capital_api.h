// Copyright 2024, Andrew Drogalis
// GNU License

#ifndef GAIN_CAPITAL_API_H
#define GAIN_CAPITAL_API_H

#include "cpr/cpr.h"

#include "json/json.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace gaincapital
{

class GCapiClient
{

  public:
    // User can access these.
    std::string trading_account_id = "";
    std::string client_account_id = "";
    std::unordered_map<std::string, int> market_id_map;

    GCapiClient();

    ~GCapiClient();

    GCapiClient(std::string username, std::string password, std::string appkey);

    // =================================================================================================================
    // UTILITIES
    // =================================================================================================================

    bool authenticate_session();

    bool validate_session();

    bool validate_session_header();

    bool validate_auth_payload();

    bool validate_account_ids();

    void set_testing_rest_urls(std::string url);

    // =================================================================================================================
    // API CALLS
    // =================================================================================================================

    nlohmann::json get_account_info(std::string param = "");

    nlohmann::json get_margin_info(std::string param = "");

    std::unordered_map<std::string, int> get_market_ids(std::vector<std::string> positions);

    std::unordered_map<std::string, std::string> get_market_info(std::vector<std::string> market_name_list, std::string param = "");

    std::unordered_map<std::string, nlohmann::json> get_prices(std::vector<std::string> market_name_list, unsigned int num_ticks = 1,
                                                               long unsigned int from_ts = 0, long unsigned int to_ts = 0,
                                                               std::string price_type = "MID");

    std::unordered_map<std::string, nlohmann::json> get_ohlc(std::vector<std::string> market_name_list, std::string interval,
                                                             unsigned int num_ticks = 1, unsigned int span = 1, long unsigned int from_ts = 0,
                                                             long unsigned int to_ts = 0);

    std::vector<std::string> trade_order(nlohmann::json trade_map, std::string type, std::string tr_account_id = "");

    nlohmann::json list_open_positions(std::string tr_account_id = "");

    nlohmann::json list_active_orders(std::string tr_account_id = "");

    nlohmann::json cancel_order(std::string order_id, std::string tr_account_id = "");

  private:
    std::string rest_url_v2 = "https://ciapi.cityindex.com/v2";
    std::string rest_url = "https://ciapi.cityindex.com/TradingAPI";
    cpr::Header session_header;
    nlohmann::json auth_payload;
    nlohmann::json session_payload;

    bool set_trading_account_id();
};

}// namespace gaincapital

#endif
