// Copyright 2024, Andrew Drogalis
// GNU License

#ifndef FX_ORDER_MANAGEMENT_H
#define FX_ORDER_MANAGEMENT_H

#include <cstddef>      // for size_t
#include <string>       // for hash, string, allocator
#include <unordered_map>// for unordered_map
#include <vector>       // for vector

#include "gain_capital_api/gain_capital_api.h"// for GCapiClient
#include "json/json.hpp"                      // for json

#include "fx_market_time.h"// for FXMarketTime
#include "fx_utilities.h"  // for FXUtilities
#include "trading_model.h" // for TradingModel

namespace fxordermgmt
{

class FXOrderManagement
{
  public:
    FXOrderManagement() = default;

    FXOrderManagement(std::string const& account, bool place_trades, int clear_system, std::string const& sys_path);

    ~FXOrderManagement() = default;

    FXOrderManagement(FXOrderManagement const& obj) = delete;

    FXOrderManagement& operator=(FXOrderManagement const& obj) = delete;

    FXOrderManagement(FXOrderManagement&& obj) = default;

    FXOrderManagement& operator=(FXOrderManagement&& obj) = default;

    // ==============================================================================================
    // Main Entry
    // ==============================================================================================

    [[nodiscard]] bool initialize_order_management();

    [[nodiscard]] bool run_order_management_system();

    // ==============================================================================================
    // Testing
    // ==============================================================================================

    void enable_testing(std::string const& url, std::string const& function_to_test);

  private:
    // Passed Through Constructor
    std::string paper_or_live, sys_path;
    bool place_trades;
    int emergency_close;

    // Gain Capital Parameters
    std::string trading_account, service_id, package;
    gaincapital::GCapiClient session;

    // For Trading Indicator
    std::unordered_map<std::string, TradingModel> trading_model_map;
    std::unordered_map<std::string, std::vector<float>> open_prices_map;
    std::unordered_map<std::string, std::vector<float>> high_prices_map;
    std::unordered_map<std::string, std::vector<float>> low_prices_map;
    std::unordered_map<std::string, std::vector<float>> close_prices_map;
    std::unordered_map<std::string, std::vector<float>> datetime_map;

    // Building Trades
    std::unordered_map<std::string, int> position_multiplier;
    std::vector<std::string> execute_list;
    nlohmann::json open_positions;

    // Getting Price History
    std::size_t last_bar_timestamp, next_bar_timestamp;
    std::vector<std::string> price_data_update_failure, data_error_list;
    std::unordered_map<std::string, int> price_data_update_failure_count;
    std::unordered_map<std::string, std::size_t> price_data_update_datetime;

    // Placing Trades
    int execution_loop_count;

    // Output Order Information
    float margin_total, equity_total, init_equity;

    // General Use
    int update_frequency_seconds;
    FXUtilities fx_utilities;
    FXMarketTime fx_market_time;

    // Testing
    bool fx_order_mgmt_testing = false;
    std::string gain_capital_testing_url;

    // ==============================================================================================
    // Forex Order Management
    // ==============================================================================================

    bool run_order_processing_loop();

    void build_trades_map();

    void emergency_position_close();

    void send_trade_and_update_profit(nlohmann::json& trade_map);

    void return_tick_history(std::vector<std::string> const& symbols_list);

    void return_price_history(std::vector<std::string> const& symbols_list);

    [[nodiscard]] bool pause_till_next_bar();

    void execute_signals(nlohmann::json& trade_dict);

    void verify_trades_opened(nlohmann::json& trade_dict);

    // ==============================================================================================
    // Gain Capital API
    // ==============================================================================================

    [[nodiscard]] bool gain_capital_session();

    // ==============================================================================================
    // Trading Model
    // ==============================================================================================

    void initialize_trading_model(std::string const& symbol);

    void get_trading_model_signal();

    // ==============================================================================================
    // Forex Order File I/O
    // ==============================================================================================

    [[nodiscard]] bool read_input_information();

    [[nodiscard]] bool output_order_information();
};

}// namespace fxordermgmt

#endif
