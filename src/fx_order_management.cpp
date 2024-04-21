// Copyright 2024, Andrew Drogalis
// GNU License

#include "fx_order_management.h"

#include <algorithm>       // for remove, find
#include <chrono>          // for system_clock
#include <cmath>           // for round
#include <ctime>           // for size_t, ctime
#include <expected>        // for expected
#include <fstream>         // for basic_ostream
#include <initializer_list>// for initializer_list
#include <iostream>        // for cerr, cout
#include <string>          // for operator==, hash
#include <unistd.h>        // for sleep, NULL
#include <unordered_map>   // for unordered_map
#include <utility>         // for pair
#include <vector>          // for vector

#include "boost/log/trivial.hpp"                 // for BOOST_LOG_TRIVIAL
#include "gain_capital_api/gain_capital_client.h"// for GCapiClient
#include "json/json.hpp"                         // for json_ref, basi...

#include "fx_exception.h"    // for FXException
#include "fx_market_time.h"  // for FXMarketTime
#include "fx_trading_model.h"// for FXTradingModel
#include "fx_utilities.h"    // for FXUtilities

namespace fxordermgmt
{

FXOrderManagement::FXOrderManagement(std::string const& paper_or_live, bool place_trades, int max_retry_failures, int emergency_close,
                                     std::string working_directory)
    : paper_or_live(paper_or_live), place_trades(place_trades), max_retry_failures(max_retry_failures), emergency_close(emergency_close),
      sys_path(working_directory)
{
}

// ==============================================================================================
// Main Entry
// ==============================================================================================

std::expected<bool, FXException> FXOrderManagement::initialize_order_management()
{
    auto logging_response = fx_utilities.initialize_logging(sys_path);
    if (! logging_response) { return logging_response; }
    // -------------------
    auto user_settings_response = load_user_settings();
    if (! user_settings_response) { return user_settings_response; }
    // -------------------
    auto validation_response = fx_utilities.validate_user_settings(update_interval, update_span, update_frequency_seconds);
    if (! validation_response) { return validation_response; }
    // -------------------
    auto password_response = fx_utilities.keyring_unlock_get_password(paper_or_live, trading_account);
    if (! password_response) { return std::expected<bool, FXException> {std::unexpect, std::move(password_response.error())}; }
    std::string password = password_response.value();
    // -------------------
    fx_market_time = FXMarketTime(start_hr, end_hr, update_frequency_seconds);
    if (! fx_market_time.initialize_forex_market_time()) { return false; }
    // -------------
    for (std::string const& symbol : fx_symbols_to_trade)
    {
        position_multiplier[symbol] = 1;
        open_prices_map[symbol].resize(num_data_points);
        high_prices_map[symbol].resize(num_data_points);
        low_prices_map[symbol].resize(num_data_points);
        close_prices_map[symbol].resize(num_data_points);
        datetime_map[symbol].resize(num_data_points);
        initialize_trading_model(symbol);
    }
    // -------------
    if (! gain_capital_session(password)) { return false; }
    // -------------
    if (emergency_close) { emergency_position_close(); }
    // -------------
    if (! output_profit_report()) { return false; }
    if (! read_active_management_file()) { return false; }
    // No Errors -> return true;
    return std::expected<bool, FXException> {true};
}

std::expected<bool, FXException> FXOrderManagement::run_order_management_system()
{
    if (! emergency_close)
    {
        BOOST_LOG_TRIVIAL(info) << "FX Order Management - Currently Running";
        // Initialize Price History & Trading Model
        return_price_history(fx_symbols_to_trade);
        for (std::string const& symbol : execute_list) { initialize_trading_model(symbol); }
        // ----------------------------
        while (! fx_market_time.is_market_closed())
        {
            if (! pause_till_next_bar()) { return false; }
            if (! read_active_management_file()) { return false; }
            build_trades_map();
            output_profit_report();
            BOOST_LOG_TRIVIAL(info) << "FX Order Management - Loop Completed";
        }
        BOOST_LOG_TRIVIAL(info) << "FX Order Management - Market Closed";
    }
    // -------------
    return true;
}

// ==============================================================================================
// Forex Order Management
// ==============================================================================================

std::expected<bool, FXException> FXOrderManagement::build_trades_map()
{
    nlohmann::json trades_map = {};
    auto open_positions_response = session.list_open_positions();
    if (! open_positions_response) { std::expected<bool, FXException> {std::unexpect, open_positions_response.error().where(), open_positions_response.error().what()}; }

    nlohmann::json open_positions = open_positions_response.value()["OpenPositions"];
    // ---------------------------
    if (! fx_market_time.is_forex_market_close_only())
    {
        for (auto position : open_positions)
        {
            std::string symbol = position["MarketName"];
            // ------------
            if (find(execute_list.begin(), execute_list.end(), symbol) != execute_list.end())
            {
                execute_list.erase(remove(execute_list.begin(), execute_list.end(), symbol), execute_list.end());

                int const base_quantity = (position_multiplier.count(symbol)) ? round(position_multiplier[symbol] * order_position_size / 1000) * 1000
                                                                              : order_position_size;
                int const signal = trading_model_map[symbol].send_trading_signal();
                int const live_quantity = position["Quantity"];
                std::string const direction = position["Direction"];
                std::string const new_direction = (direction == "buy") ? "sell" : "buy";
                // ---------------------------
                if (direction == "buy")
                {
                    if (signal == 1 && live_quantity < base_quantity)
                    {
                        trades_map[symbol] = {
                            {"Quantity", base_quantity - live_quantity}, {"Direction", direction}, {"Final Quantity", base_quantity}};
                    }
                    else if (signal == 0) { trades_map[symbol] = {{"Quantity", live_quantity}, {"Direction", new_direction}, {"Final Quantity", 0}}; }
                    else if (signal == -1)
                    {
                        trades_map[symbol] = {
                            {"Quantity", base_quantity + live_quantity}, {"Direction", new_direction}, {"Final Quantity", base_quantity}};
                    }
                }
                else if (direction == "sell")
                {
                    if (signal == -1 && live_quantity < base_quantity)
                    {
                        trades_map[symbol] = {
                            {"Quantity", base_quantity - live_quantity}, {"Direction", direction}, {"Final Quantity", base_quantity}};
                    }
                    else if (signal == 0) { trades_map[symbol] = {{"Quantity", live_quantity}, {"Direction", new_direction}, {"Final Quantity", 0}}; }
                    else if (signal == 1)
                    {
                        trades_map[symbol] = {
                            {"Quantity", base_quantity + live_quantity}, {"Direction", new_direction}, {"Final Quantity", base_quantity}};
                    }
                }
            }
        }
        // ---------------------------
        // Open New Positions
        if (! execute_list.empty())
        {
            for (auto const& symbol : execute_list)
            {
                int const position_signal = trading_model_map[symbol].send_trading_signal();
                int const base_quantity = (position_multiplier.count(symbol)) ? round(position_multiplier[symbol] * order_position_size / 1000) * 1000
                                                                              : order_position_size;
                if (position_signal && base_quantity)
                {
                    std::string const direction = (position_signal == 1) ? "buy" : "sell";
                    trades_map[symbol] = {{"Quantity", base_quantity}, {"Direction", direction}, {"Final Quantity", base_quantity}};
                }
            }
        }
    }
    // ------------------------------------
    // Exit Only Positions
    else
    {
        for (auto position : open_positions)
        {
            std::string const symbol = position["MarketName"];
            std::string const new_direction = (position["Direction"] == "buy") ? "sell" : "buy";
            trades_map[symbol] = {{"Quantity", static_cast<int>(position["Quantity"])}, {"Direction", new_direction}, {"Final Quantity", 0}};
        }
    }
    // Place Trades
    if (! trades_map.empty())
    {
        execution_loop_count = 0;
        execute_signals(trades_map);
    }
}

std::expected<bool, FXException> FXOrderManagement::emergency_position_close()
{
    nlohmann::json trades_map = {};
    auto open_positions_response = session.list_open_positions();
    if (! open_positions_response) { std::expected<bool, FXException> {std::unexpect, open_positions_response.error().where(), open_positions_response.error().what()}; }
    
    nlohmann::json open_positions = open_positions_response.value()["OpenPositions"];
    // ---------------------------
    for (auto position : open_positions)
    {
        std::string const symbol = position["MarketName"];
        int const quantity = position["Quantity"];
        std::string const direction = position["Direction"];
        std::string const new_direction = (direction == "buy") ? "sell" : "buy";
        trades_map[symbol] = {{"Quantity", quantity}, {"Direction", new_direction}, {"Final Quantity", 0}};
    }
    // Place Trades
    if (! trades_map.empty())
    {
        execution_loop_count = 0;
        execute_signals(trades_map);
    }
}

/*  * This Function is Not Called
 *  * User Has Option to Replace OHLC w/ Tick Data */
std::expected<bool, FXException> FXOrderManagement::return_tick_history(std::vector<std::string> const& symbols_list)
{
    for (auto const& symbol: symbols_list)
    {
    auto prices_response = session.get_prices(symbol, num_data_points);
    if (! prices_response) { return std::expected<bool, FXException> {std::unexpect, prices_response.error().where(), prices_response.error().what()}; }
    nlohmann::json prices = prices_response.value();
    /*
        Do something with values
    */
}
}

std::expected<bool, FXException> FXOrderManagement::return_price_history(std::vector<std::string> const& symbols_list)
{
    BOOST_LOG_TRIVIAL(info) << "FX Order Management - Attempting to Fetch Price History";
    int const WAIT_DURATION = 10;
    std::size_t start = (std::chrono::system_clock::now().time_since_epoch()).count() * std::chrono::system_clock::period::num /
                        std::chrono::system_clock::period::den;
    price_data_update_datetime = {};
    price_data_update_failure = {};
    std::unordered_map<std::string, nlohmann::json> data;
    std::vector<std::string> loop_list = symbols_list;
    std::vector<std::string> completed_list = {};

    while (true)
    {
        data = session.get_ohlc(loop_list, update_interval, num_data_points, update_span);
        // ---------------------------
        for (std::string symbol : loop_list)
        {
            try
            {
                std::string last_bar_datetime = data[symbol].back()["BarDate"];
                std::size_t last_timestamp = std::stoi(last_bar_datetime.substr(6, 10));
                if (last_timestamp >= next_bar_timestamp)
                {
                    completed_list.push_back(symbol);
                    price_data_update_datetime[symbol] = last_timestamp;
                }
            }
            catch (...)
            {
                price_data_update_failure.push_back(symbol);
            }
        }
        // ------------
        loop_list = price_data_update_failure;
        std::size_t timestamp_now = (std::chrono::system_clock::now().time_since_epoch()).count() * std::chrono::system_clock::period::num /
                                    std::chrono::system_clock::period::den;
        if (loop_list.empty() || (timestamp_now - start) > WAIT_DURATION) { break; }
        else
        {
            price_data_update_failure = {};
            sleep(2);
        }
    }
    // Confirm Latest Data Provided
    // --------------------------------
    std::vector<std::size_t> symbol_update_datetimes = {};
    for (auto it = price_data_update_datetime.begin(); it != price_data_update_datetime.end(); ++it)
    {
        symbol_update_datetimes.push_back(it->second);
    }
    last_bar_timestamp =
        (! symbol_update_datetimes.empty()) ? *max_element(symbol_update_datetimes.begin(), symbol_update_datetimes.end()) : last_bar_timestamp;

    for (auto it = price_data_update_datetime.begin(); it != price_data_update_datetime.end(); ++it)
    {
        if (it->second != last_bar_timestamp)
        {
            price_data_update_failure.push_back(it->first);
            completed_list.erase(remove(completed_list.begin(), completed_list.end(), it->first), completed_list.end());
        }
    }

    for (std::string symbol : price_data_update_failure)
    {
        BOOST_LOG_TRIVIAL(error) << "FX Order Management - Price Data Update Failure " << symbol << "; Will Retry After Trades Placed";
    }
    // ------------------------------------
    // Separate Data From OHLC into Individual Vectors
    execute_list = {};
    for (std::string symbol : completed_list)
    {
        nlohmann::json ohlc_data = data[symbol];
        int const data_length = ohlc_data.size();
        if (data_length >= num_data_points)
        {
            execute_list.push_back(symbol);
            // ------------
            for (int x = 0; x < num_data_points; x++)
            {
                int const x1 = data_length - num_data_points + x;
                open_prices_map[symbol][x] = ohlc_data[x1]["Open"];
                high_prices_map[symbol][x] = ohlc_data[x1]["High"];
                low_prices_map[symbol][x] = ohlc_data[x1]["Low"];
                close_prices_map[symbol][x] = ohlc_data[x1]["Close"];
                datetime_map[symbol][x] = std::stof(static_cast<std::string>(ohlc_data[x1]["BarDate"]).substr(6, 10));
            }
        }
        else
        {
            data_error_list.push_back(symbol);
            BOOST_LOG_TRIVIAL(warning) << "Data Acquisition Error; Length Too Short: " << symbol << " - " << data_length;
        }
    }
}

std::expected<bool, FXException> FXOrderManagement::pause_till_next_bar()
{
    next_bar_timestamp = last_bar_timestamp + update_frequency_seconds;
    std::size_t const time_next_bar_will_be_ready = next_bar_timestamp + update_frequency_seconds;
    // -----------------------------------
    // Execute Trading Model & Confirm All Data Acquired Properly
    for (int x = 0; x < 5; x++)
    {
        std::size_t const timestamp_now = (std::chrono::system_clock::now().time_since_epoch()).count() * std::chrono::system_clock::period::num /
                                          std::chrono::system_clock::period::den;
        if (! price_data_update_failure.empty())
        {
            if (timestamp_now <= time_next_bar_will_be_ready - 15)
            {
                return_price_history(price_data_update_failure);
                if (! execute_list.empty())
                {
                    if (! read_active_management_file()) { return false; }
                    build_trades_map();
                    output_profit_report();
                }
            }
            else { break; }
        }
        else
        {
            price_data_update_failure_count = {};
            break;
        }
    }
    // -----------------------------------
    for (std::string const& symbol : price_data_update_failure)
    {
        price_data_update_failure_count[symbol] += 1;
        BOOST_LOG_TRIVIAL(warning) << "Updates Failed " << symbol << "; Number of Failed Loops: " << price_data_update_failure_count[symbol]
                                   << std::endl;
    }
    // -----------------------------------
    // Notify Data Error
    if (! data_error_list.empty())
    {
        for (std::string const& symbol : data_error_list)
        {
            fx_symbols_to_trade.erase(remove(fx_symbols_to_trade.begin(), fx_symbols_to_trade.end(), symbol), fx_symbols_to_trade.end());
            if (price_data_update_datetime.contains(symbol)) { price_data_update_datetime.erase(symbol); }
            BOOST_LOG_TRIVIAL(error) << "Data Acquisition Error: " << symbol << std::endl;
            position_multiplier[symbol] = 0;
        }
        data_error_list = {};
    }
    // -----------------------------------
    // Wait Till New Bar Ready & Fetch OHLC
    std::size_t const timestamp_now = (std::chrono::system_clock::now().time_since_epoch()).count() * std::chrono::system_clock::period::num /
                                      std::chrono::system_clock::period::den;
    float time_to_wait_now = time_next_bar_will_be_ready - timestamp_now;
    if (time_to_wait_now > 0)
    {
        BOOST_LOG_TRIVIAL(info) << "FX Order Management - Waiting For OHLC Bar Update";
        sleep(time_to_wait_now);
    }
    return_price_history(fx_symbols_to_trade);
    // ------------
    return true;
}

std::expected<bool, FXException> FXOrderManagement::execute_signals(nlohmann::json& trades_map)
{
    if (place_trades || emergency_close)
    {
        auto error_list = session.trade_order(trades_map, "MARKET");
        ++execution_loop_count;
        // ------------
        // Notify if any errors
        if (! error_list.empty())
        {
            for (auto const& elem : error_list) { BOOST_LOG_TRIVIAL(warning) << "Trading Error for: " << elem; }
        }
        // ------------
        // Check On Active Orders
        int const ITERATIONS = 3;
        for (int x = 0; x < ITERATIONS; x++)
        {
            // Pause For Each Loop
            sleep(1);
            int pending_order_count = 0;
            nlohmann::json active_orders = session.list_active_orders();
            for (auto& order : active_orders)
            {
                int const status = order["TradeOrder"]["StatusId"];
                // ---------------------------
                if (status == 1)
                {
                    ++pending_order_count;
                    if (x == ITERATIONS - 1)
                    {
                        nlohmann::json resp = session.cancel_order(order["TradeOrder"]["OrderId"]);
                        BOOST_LOG_TRIVIAL(warning) << "Canceled Order: " << resp;
                    }
                }
                // ---------------------------
                std::vector<int> MAJOR_STATUS_ERROR_CODES = {6, 8, 10};
                if (std::find(MAJOR_STATUS_ERROR_CODES.begin(), MAJOR_STATUS_ERROR_CODES.end(), status) != MAJOR_STATUS_ERROR_CODES.end())
                {
                    // Pending Order Count Doesn't Increase Because Order Will Never Finish
                    BOOST_LOG_TRIVIAL(error) << "Major Order Status Error: " << status;
                }
            }
            if (! pending_order_count) { break; }
        }
        // ------------
        // Recursive Trade Confirmation Can Only Happen A Maximum of (3) Times
        if (execution_loop_count <= 3) { verify_trades_opened(trades_map); }
        else
        {
            for (std::string const& symbol : error_list)
            {
                BOOST_LOG_TRIVIAL(warning) << "Trading Error, Symbol Removed: " << symbol;
                position_multiplier[symbol] = 0;
            }
        }
    }
}

std::expected<bool, FXException> FXOrderManagement::verify_trades_opened(nlohmann::json& trades_map)
{
    nlohmann::json NEW_trades_map = {};
    std::vector<std::string> keys_list = {};

    for (nlohmann::json::iterator it = trades_map.begin(); it != trades_map.end(); ++it) { keys_list.push_back(it.key()); }
    // ------------
    // Check Open Positions
    auto open_positions_response = session.list_open_positions();
    if (! open_positions_response) { std::expected<bool, FXException> {std::unexpect, open_positions_response.error().where(), open_positions_response.error().what()}; }

    nlohmann::json open_positions = open_positions_response.value()["OpenPositions"];
    for (auto& position : open_positions)
    {
        std::string const symbol = position["MarketName"];
        float const existing_quantity = position["Quantity"];
        std::string const existing_direction = position["Direction"];
        // ------------
        if (find(keys_list.begin(), keys_list.end(), symbol) != keys_list.end())
        {
            keys_list.erase(remove(keys_list.begin(), keys_list.end(), symbol), keys_list.end());
            std::string const direction = trades_map[symbol]["Direction"];
            int const final_quantity = trades_map[symbol]["Final Quantity"];
            // ------------
            if (direction != existing_direction)
            {
                NEW_trades_map[symbol] = {
                    {"Quantity", existing_quantity + final_quantity}, {"Direction", direction}, {"Final Quantity", final_quantity}};
            }
            else if (direction == existing_direction && existing_quantity < final_quantity)
            {
                NEW_trades_map[symbol] = {
                    {"Quantity", final_quantity - existing_quantity}, {"Direction", direction}, {"Final Quantity", final_quantity}};
            }
        }
    }
    // ---------------------------
    // If position not opened, add to new trades map
    for (std::string const& symbol : keys_list)
    {
        if (static_cast<int>(trades_map[symbol]["Final Quantity"]) != 0) { NEW_trades_map[symbol] = trades_map[symbol]; }
    }
    // ---------------------------
    // Execute Trades
    if (! NEW_trades_map.empty()) { execute_signals(trades_map); }
}

// ==============================================================================================
// Gain Capital API
// ==============================================================================================
std::expected<bool, FXException> FXOrderManagement::gain_capital_session(std::string const& password)
{
    session = gaincapital::GCClient(trading_account, password, forex_api_key);
    // ---------------------------
    if (fx_order_mgmt_testing) { session.set_testing_rest_urls(gain_capital_testing_url); }
    auto authenticate_session_response = session.authenticate_session();
    if (! authenticate_session_response)
    {
        return std::expected<bool, FXException> {std::unexpect, authenticate_session_response.error().where(),
                                                 authenticate_session_response.error().what()};
    }

    BOOST_LOG_TRIVIAL(info) << "FX Order Management - New Gain Capital Session Initiated";

    for (auto const& symbol : fx_symbols_to_trade)
    {
        auto market_id_response = session.get_market_id(symbol);
        if (! market_id_response)
        {
            return std::expected<bool, FXException> {std::unexpect, market_id_response.error().where(), market_id_response.error().what()};
        }
    }

    return std::expected<bool, FXException> {true};
}

// ==============================================================================================
// FX Trading Model
// ==============================================================================================
void FXOrderManagement::initialize_trading_model(std::string const& symbol) noexcept
{
    trading_model_map.emplace(symbol, FXTradingModel {open_prices_map[symbol], high_prices_map[symbol], low_prices_map[symbol],
                                                      close_prices_map[symbol], datetime_map[symbol]});

    BOOST_LOG_TRIVIAL(info) << "FX Order Management - Trading Model Initialized for " << symbol;
}

// ==============================================================================================
// Forex File I/O
// ==============================================================================================

std::expected<bool, FXException> FXOrderManagement::load_user_settings() 
{
    std::string const file_name = sys_path + "/interface_files/user_settings.json";
    nlohmann::json data;

    std::ifstream in(file_name);
    if (in.is_open())
    {
        data = nlohmann::json::parse(in);
        in.close();
    }
    else
    {

    }
}

std::expected<bool, FXException> FXOrderManagement::read_active_management_file()
{
    std::string const file_name = sys_path + "/interface_files/active_order_management.json";
    std::vector<std::string> double_check = fx_symbols_to_trade;
    nlohmann::json data;

    std::ifstream in(file_name);
    if (in.is_open())
    {
        data = nlohmann::json::parse(in);
        in.close();

        for (nlohmann::json::iterator it = data.begin(); it != data.end(); ++it)
        {
            nlohmann::json json_value = it.value();
            if (json_value.contains("Close Immediately") && json_value["Close Immediately"])
            {
                // Fix This
                position_multiplier[it.key()] = 0;
            }
            if (json_value.contains("Close On Trade Signal Change") && json_value["Close On Trade Signal Change"])
            {
                position_multiplier[it.key()] = 0;
            }
            double_check.erase(remove(double_check.begin(), double_check.end(), it.key()), double_check.end());
        }
    }

    for (std::string const& symbol : double_check) { data[symbol] = {{"Close Immediately", false}, {"Close On Trade Signal Change", false}}; }

    std::ofstream out(file_name);
    bool success = false;
    if (out.is_open())
    {
        out << data.dump(4);
        success = out.good();
        out.close();
    }
    return success;
}

std::expected<bool, FXException> FXOrderManagement::output_profit_report()
{
    nlohmann::json current_performance = {}, current_positions = {};

    auto margin_info_reponse = session.get_margin_info();
    float equity_total = (! margin_info.empty()) ? static_cast<float>(margin_info["netEquity"]) : 0.0;
    float margin_total = (! margin_info.empty()) ? static_cast<float>(margin_info["margin"]) : 0.0;

    auto open_positions_response = session.list_open_positions();
    if (! open_positions_response) { std::expected<bool, FXException> {std::unexpect, open_positions_response.error().where(), open_positions_response.error().what()}; }

    nlohmann::json open_positions = open_positions_response.value()["OpenPositions"];
    for (auto& position : open_positions)
    {
        std::string const market_name = position["MarketName"];
        int const direction = (position["Direction"] == "buy") ? 1 : -1;
        float const entry_price = position["Price"];
        int const quantity = position["Quantity"];

        auto prices_response = session.get_prices({market_name});
        float const current_price = (! response.empty()) ? static_cast<float>(response[market_name][0]["Price"]) : 0.0;

        float profit = round((current_price - entry_price) * 100'000) / 100'000 * direction;
        float profit_percent = (entry_price != 0) ? round(profit * 10'000 / entry_price) / 100 : 0;

        current_positions[market_name] = {{"Direction", position["Direction"]},
                                          {"Quantity", position["Quantity"]},
                                          {"Entry Price", position["Price"]},
                                          {"Current Price", current_price},
                                          {"Profit", profit},
                                          {"Profit Percent", profit_percent}};
    }
    // ---------------------------
    if (initial_equity == 0) { initial_equity = equity_total; }
    float const total_profit = round((equity_total - initial_equity) * 100) / 100;
    float const total_profit_percent = (initial_equity != 0) ? round(total_profit * 10'000 / initial_equity) / 100 : 0;

    current_performance["Performance Information"] = {{"Initial Funds", initial_equity},
                                                      {"Current Funds", equity_total},
                                                      {"Margin Utilized", margin_total},
                                                      {"Profit Cumulative", total_profit},
                                                      {"Profit Percent Cumulative", total_profit_percent}};
    current_performance["Position Information"] = current_positions;
    time_t time_now = time(NULL);
    current_performance["Last Updated"] = ctime(&time_now);

    std::string const file_name = sys_path + "/" + fx_utilities.get_todays_date() + "_FX_Order_Information.json";

    std::ofstream out(file_name);
    bool success = false;
    if (out.is_open())
    {
        out << current_performance.dump(4);
        success = out.good();
        out.close();
    }
    return success;
}

// ==============================================================================================
// Testing
// ==============================================================================================

std::expected<std::string, FXException> FXOrderManagement::enable_testing(std::string const& url)
{
    fx_order_mgmt_testing = true;
    gain_capital_testing_url = url;
}

}// namespace fxordermgmt
