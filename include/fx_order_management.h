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
    FXOrderManagement() noexcept = default;

    FXOrderManagement(const std::string& account, bool place_trades, int clear_system, const std::string& sys_path);

    ~FXOrderManagement() noexcept = default;

    FXOrderManagement(const FXOrderManagement& obj) = delete;

    FXOrderManagement& operator=(const FXOrderManagement& obj) = delete;

    FXOrderManagement(FXOrderManagement&& obj) noexcept = default;

    FXOrderManagement& operator=(FXOrderManagement&& obj) noexcept = default;

    [[nodiscard]] bool initialize_order_management();

    [[nodiscard]] bool run_order_management_system();

    void set_testing_parameters(std::string url);

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
    std::unordered_map<std::string, TradingModel> trading_model_map;
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<float>>> historical_data_map;

    // Build Trades Map
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> trades_map;
    std::unordered_map<std::string, int> position_multiplier;
    std::vector<std::string> execute_list;
    nlohmann::json open_positions;

    // Getting Price History
    std::size_t last_bar_timestamp = 0, next_bar_timestamp = 0;
    std::vector<std::string> price_data_update_failure, data_error_list, live_symbols_list;
    std::unordered_map<std::string, int> price_data_update_failure_count;
    std::unordered_map<std::string, std::size_t> price_data_update_datetime;

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

    // Testing
    bool global_testing = false;
    std::string testing_url = "";

    // ==============================================================================================
    // Gain Capital API
    // ==============================================================================================

    [[nodiscard]] bool gain_capital_session();

    // ==============================================================================================
    // Trading Model
    // ==============================================================================================

    void initialize_trading_model(const std::string& symbol);

    void get_trading_model_signal();

    // ==============================================================================================
    // Forex Order Management
    // ==============================================================================================

    void build_trades_map();

    void emergency_position_close();

    void return_tick_history(const std::vector<std::string>& symbols_list);

    void return_price_history(const std::vector<std::string>& symbols_list) noexcept;

    [[nodiscard]] bool pause_next_bar();

    void execute_signals(nlohmann::json trade_dict);

    void verify_trades_opened(nlohmann::json trade_dict);

    // ==============================================================================================
    // Forex Order File I/O
    // ==============================================================================================

    [[nodiscard]] bool read_input_information();

    [[nodiscard]] bool output_order_information();
};

}// namespace fxordermgmt

#endif
