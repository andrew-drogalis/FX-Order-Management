// Copyright 2024, Andrew Drogalis
// GNU License

#ifndef FX_ORDER_MANAGEMENT_H
#define FX_ORDER_MANAGEMENT_H

#include "fx_market_time.h"
#include "fx_utilities.h"
#include "gain_capital_api/gain_capital_api.h"
#include "trading_model.h"

#include "json/json.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace fxordermgmt
{

class FXOrderManagement
{

  public:
    FXOrderManagement();

    ~FXOrderManagement();

    FXOrderManagement(std::string account, bool place_trades, int clear_system, std::string sys_path);

    bool initalize_order_management();

    bool run_order_management_system();

  private:
    // Passed Through Constructor
    std::string paper_or_live = "";
    bool place_trades = false;
    int emergency_close = 0;
    std::string sys_path = "";

    // Gain Capital Parameters
    std::string trading_account = "", service_id = "", package = "";
    gaincapital::GCapiClient session;

    // For Trading Indicator
    std::unordered_map<std::string, int> main_signals;
    std::unordered_map<std::string, TradingModel> trading_model_map;
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<float>>> historical_data_map;

    // Build Trades Map
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> trades_map;
    std::unordered_map<std::string, int> position_multiplier;
    std::vector<std::string> execute_list;
    nlohmann::json open_positions;

    // Getting Price History
    long unsigned int last_bar_timestamp = 0, next_bar_timestamp = 0;
    std::vector<std::string> price_data_update_failure, data_error_list, live_symbols_list;
    std::unordered_map<std::string, int> price_data_update_failure_count;
    std::unordered_map<std::string, long unsigned int> price_data_update_datetime;

    // Placing Trades
    int execution_loop_count = 0;

    // Output Order Information
    float margin_total = 0;
    float equity_total = 0;
    float init_equity = 0;

    // General Use
    int update_frequency_seconds = 0;
    FXUtilities fx_utilities;
    FXMarketTime fx_market_time;

    // ==============================================================================================
    // Gain Capital API
    // ==============================================================================================

    bool gain_capital_session();

    // ==============================================================================================
    // Trading Model
    // ==============================================================================================

    void initalize_trading_model(std::string symbol);

    void get_trading_model_signal();

    // ==============================================================================================
    // Forex Order Management
    // ==============================================================================================

    void build_trades_map();

    void emergency_position_close();

    void return_tick_history(std::vector<std::string> symbols_list);

    void return_price_history(std::vector<std::string> symbols_list);

    bool pause_next_bar();

    void execute_signals(nlohmann::json trade_dict);

    void verify_trades_opened(nlohmann::json trade_dict);

    // ==============================================================================================
    // Forex Order File I/O
    // ==============================================================================================

    bool read_input_information();

    bool output_order_information();
};

}// namespace fxordermgmt

#endif