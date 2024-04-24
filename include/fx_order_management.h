// Copyright 2024, Andrew Drogalis
// GNU License

#ifndef FX_ORDER_MANAGEMENT_H
#define FX_ORDER_MANAGEMENT_H

#include <cstddef>      // for size_t
#include <expected>     // for expected
#include <string>       // for hash, string, allocator
#include <unordered_map>// for unordered_map
#include <vector>       // for vector

#include "gain_capital_api/gain_capital_client.h"// for GCapiClient
#include "json/json.hpp"                         // for json

#include "fx_exception.h"    // for FXException
#include "fx_market_time.h"  // for FXMarketTime
#include "fx_trading_model.h"// for FXTradingModel
#include "fx_utilities.h"    // for FXUtilities

namespace fxordermgmt
{

class FXOrderManagement
{
  public:
    FXOrderManagement() = default;

    FXOrderManagement(std::string const& account, bool place_trades, int max_retry_failures, int clear_system, bool file_logging,
                      std::string sys_path);

    ~FXOrderManagement() = default;

    // Move ONLY | No Copy Constructor
    FXOrderManagement(FXOrderManagement const& obj) = delete;

    FXOrderManagement& operator=(FXOrderManagement const& obj) = delete;

    FXOrderManagement(FXOrderManagement&& obj) noexcept = default;

    FXOrderManagement& operator=(FXOrderManagement&& obj) noexcept = default;

    // ==============================================================================================
    // Main Entry
    // ==============================================================================================

    [[nodiscard]] std::expected<bool, FXException> initialize_order_management();

    [[nodiscard]] std::expected<bool, FXException> run_order_management_system();

    // ==============================================================================================
    // Testing
    // ==============================================================================================

    void enable_testing(std::string const& url);

  private:
    // Passed Through Constructor
    std::string paper_or_live;
    bool place_trades;
    int max_retry_failures;
    int emergency_close;
    bool file_logging;
    std::string sys_path;

    // Load User Settings
    std::vector<std::string> fx_symbols_to_trade;
    std::string trading_account, forex_api_key, update_interval;
    int start_hr, end_hr, num_data_points, update_span, order_position_size;

    // For Gain Capital
    gaincapital::GCClient session;

    // For Trading Indicator
    std::unordered_map<std::string, FXTradingModel> trading_model_map;
    std::unordered_map<std::string, std::vector<float>> open_prices_map;
    std::unordered_map<std::string, std::vector<float>> high_prices_map;
    std::unordered_map<std::string, std::vector<float>> low_prices_map;
    std::unordered_map<std::string, std::vector<float>> close_prices_map;
    std::unordered_map<std::string, std::vector<float>> datetime_map;

    // Building Trades
    std::unordered_map<std::string, int> position_multiplier;
    std::vector<std::string> execute_list;

    // Getting Price History
    std::size_t last_bar_timestamp, next_bar_timestamp;
    std::unordered_map<std::string, int> price_update_failure_count;

    // Placing Trades
    int execution_loop_count;

    // Output Profit Report
    float initial_equity;

    // General Use
    int update_frequency_seconds, general_error_count;
    FXUtilities fx_utilities;
    FXMarketTime fx_market_time;

    // Testing
    bool fx_order_mgmt_testing = false;
    std::string gain_capital_testing_url;

    // ==============================================================================================
    // Forex Order Management
    // ==============================================================================================

    [[nodiscard]] std::expected<bool, FXException> trade_order_sequence();

    [[nodiscard]] std::expected<std::vector<nlohmann::json>, FXException> build_trades();

    void return_tick_history(std::vector<std::string> const& symbols_list);

    void return_price_history(std::vector<std::string> const& symbols_list);

    [[nodiscard]] std::expected<bool, FXException> pause_till_next_bar();

    [[nodiscard]] std::expected<bool, FXException> execute_signals(std::vector<nlohmann::json>& trade_dict);

    [[nodiscard]] std::expected<bool, FXException> monitor_active_orders();

    [[nodiscard]] std::expected<bool, FXException> verify_trades_opened(std::vector<nlohmann::json>& trade_dict);

    // ==============================================================================================
    // Gain Capital API
    // ==============================================================================================

    [[nodiscard]] std::expected<bool, FXException> gain_capital_session(std::string const& password);

    // ==============================================================================================
    // FX Trading Model
    // ==============================================================================================

    void initialize_trading_model(std::string const& symbol) noexcept;

    // ==============================================================================================
    // Forex File I/O
    // ==============================================================================================

    [[nodiscard]] std::expected<bool, FXException> load_user_settings();

    [[nodiscard]] std::expected<bool, FXException> build_filesystem_directory(std::string const& dir);

    [[nodiscard]] std::expected<bool, FXException> read_active_management_file();

    [[nodiscard]] std::expected<bool, FXException> output_profit_report();
};

}// namespace fxordermgmt

#endif
