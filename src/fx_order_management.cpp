// Copyright 2024, Andrew Drogalis
// GNU License

#include "fx_order_management.h"

#include <algorithm>       // for remove, find
#include <chrono>          // for system_clock
#include <cmath>           // for round
#include <ctime>           // for size_t, ctime
#include <expected>        // for expected
#include <filesystem>      // for is_directory, create_directories...
#include <fstream>         // for basic_ostream
#include <initializer_list>// for initializer_list
#include <iostream>        // for cerr, cout
#include <source_location> // for current, function_name...
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
                                     bool file_logging, std::string working_directory)
    : paper_or_live(paper_or_live), place_trades(place_trades), max_retry_failures(max_retry_failures), emergency_close(emergency_close),
      file_logging(file_logging), sys_path(working_directory)
{
}

// ==============================================================================================
// Main Entry
// ==============================================================================================

std::expected<bool, FXException> FXOrderManagement::initialize_order_management()
{
    auto user_settings_response = load_user_settings();
    if (! user_settings_response) { return user_settings_response; }

    auto validation_response = fx_utilities.validate_user_settings(update_interval, update_span, update_frequency_seconds);
    if (! validation_response) { return validation_response; }

    auto password_response = fx_utilities.keyring_unlock_get_password(paper_or_live, trading_account);
    if (! password_response) { return std::expected<bool, FXException> {std::unexpect, std::move(password_response.error())}; }
    std::string password = password_response.value();

    fx_market_time = FXMarketTime(start_hr, end_hr, update_frequency_seconds);
    if (! fx_market_time.initialize_forex_market_time()) { return false; }

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
    if (! gain_capital_session(password)) { return false; }

    if (file_logging)
    {
        auto logging_response = fx_utilities.initialize_logging_file(sys_path);
        if (! logging_response) { return logging_response; }
    }
    else { fx_utilities.log_to_std_output(); }
    // -------------------
    return std::expected<bool, FXException> {true};
}

std::expected<bool, FXException> FXOrderManagement::run_order_management_system()
{
    BOOST_LOG_TRIVIAL(info) << "FX Order Management - Currently Running";
    if (! emergency_close)
    {
        for (std::string const& symbol : execute_list) { initialize_trading_model(symbol); }
    }
    while (! fx_market_time.is_market_closed())
    {
        auto trade_order_response = trade_order_sequence();
        auto pause_next_bar_response = pause_till_next_bar();

        if (! trade_order_response)
        {
            ++general_error_count;
            if (general_error_count > max_retry_failures) { return trade_order_response; }
        }
        else if (! pause_next_bar_response)
        {
            ++general_error_count;
            if (general_error_count > max_retry_failures) { return pause_next_bar_response; }
        }
        else { general_error_count = 0; }
    }

    BOOST_LOG_TRIVIAL(info) << "FX Order Management - Loop Completed";
    // -------------------
    return std::expected<bool, FXException> {true};
}

// ==============================================================================================
// Forex Order Management
// ==============================================================================================

std::expected<bool, FXException> FXOrderManagement::trade_order_sequence()
{
    auto trade_map_response = build_trades();
    if (! trade_map_response) { return std::expected<bool, FXException> {std::unexpect, std::move(trade_map_response.error())}; }

    std::vector<nlohmann::json> trade_map = trade_map_response.value();

    // Place Trades
    if (! trade_map.empty())
    {
        execution_loop_count = 0;
        execute_signals(trade_map);
    }

    auto profit_report_response = output_profit_report();
    if (! profit_report_response) { return profit_report_response; }

    auto read_active_mgmt_response = read_active_management_file();
    if (! read_active_mgmt_response) { return read_active_mgmt_response; }
    // -------------------
    return std::expected<bool, FXException> {true};
}

std::expected<std::vector<nlohmann::json>, FXException> FXOrderManagement::build_trades()
{
    std::vector<nlohmann::json> trades_vect;
    auto open_positions_response = session.list_open_positions();
    if (! open_positions_response)
    {
        std::expected<std::vector<nlohmann::json>, FXException> {std::unexpect, open_positions_response.error().where(),
                                                                 open_positions_response.error().what()};
    }
    nlohmann::json open_positions = open_positions_response.value()["OpenPositions"];
    // -------------------
    if (! fx_market_time.is_forex_market_close_only() && ! emergency_close)
    {
        for (auto position : open_positions)
        {
            std::string symbol = position["MarketName"];
            // ------------
            if (find(execute_list.begin(), execute_list.end(), symbol) != execute_list.end())
            {
                nlohmann::json trades_map = {};
                execute_list.erase(remove(execute_list.begin(), execute_list.end(), symbol), execute_list.end());

                int const base_quantity = (position_multiplier.count(symbol)) ? round(position_multiplier[symbol] * order_position_size / 1000) * 1000
                                                                              : order_position_size;
                int const signal = trading_model_map[symbol].send_trading_signal();
                int const live_quantity = position["Quantity"];
                std::string const direction = position["Direction"];
                std::string const new_direction = (direction == "buy") ? "sell" : "buy";
                // -------------------
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
                trades_vect.emplace_back(trades_map);
            }
        }
        // -------------------
        // Open New Positions
        if (! execute_list.empty())
        {
            for (auto const& symbol : execute_list)
            {
                nlohmann::json trades_map = {};
                int const position_signal = trading_model_map[symbol].send_trading_signal();
                int const base_quantity = (position_multiplier.count(symbol)) ? round(position_multiplier[symbol] * order_position_size / 1000) * 1000
                                                                              : order_position_size;
                if (position_signal && base_quantity)
                {
                    std::string const direction = (position_signal == 1) ? "buy" : "sell";
                    trades_map[symbol] = {{"Quantity", base_quantity}, {"Direction", direction}, {"Final Quantity", base_quantity}};
                    trades_vect.emplace_back(trades_map);
                }
            }
        }
    }
    // -------------------
    // Exit Only Positions
    else
    {
        for (auto position : open_positions)
        {
            nlohmann::json trades_map = {};
            std::string const symbol = position["MarketName"];
            std::string const new_direction = (position["Direction"] == "buy") ? "sell" : "buy";
            trades_map[symbol] = {{"Quantity", static_cast<int>(position["Quantity"])}, {"Direction", new_direction}, {"Final Quantity", 0}};
            trades_vect.emplace_back(trades_map);
        }
    }
    // -------------------
    return std::expected<std::vector<nlohmann::json>, FXException> {trades_vect};
}

/*  * This Function is Not Called
 *  * User Has Option to Replace OHLC w/ Tick Data */
void FXOrderManagement::return_tick_history(std::vector<std::string> const& symbols_list)
{
    for (auto const& symbol : symbols_list)
    {
        auto prices_response = session.get_prices(symbol, num_data_points);
        if (! prices_response)
        {
            // throw FXException { prices_response.error().where(), prices_response.error().what()};
        }
        nlohmann::json prices = prices_response.value();
        /*
            Do something with values
        */
    }
}

void FXOrderManagement::return_price_history(std::vector<std::string> const& symbols_list)
{
    BOOST_LOG_TRIVIAL(info) << "FX Order Management - Attempting to Fetch Price History";

    execute_list = {};

    for (auto const& symbol : symbols_list)
    {
        auto ohlc_response = session.get_ohlc(symbol, update_interval, num_data_points, update_span);
        try
        {
            if (! ohlc_response) { throw FXException {ohlc_response.error().where(), ohlc_response.error().what()}; }

            nlohmann::json ohlc_json = ohlc_response.value();

            std::string const last_bar_datetime = ohlc_json["PriceBars"].back()["BarDate"].dump();

            if (last_bar_datetime == "null") { throw FXException {std::source_location::current().function_name(), "JSON Key Error"}; }

            std::size_t last_timestamp = std::stoi(last_bar_datetime.substr(6, 10));

            if (last_timestamp < next_bar_timestamp)
            {
                throw FXException {std::source_location::current().function_name(), "Timestamp is Not Current"};
            }
            // Separate Data From OHLC into Individual Vectors
            int const data_length = ohlc_json["PriceBars"].size();
            if (data_length >= num_data_points)
            {
                execute_list.push_back(symbol);
                if (price_update_failure_count.count(symbol)) { price_update_failure_count.erase(symbol); }
                // ------------
                for (int x = 0; x < num_data_points; x++)
                {
                    int const x1 = data_length - num_data_points + x;
                    open_prices_map[symbol][x] = ohlc_json["PriceBars"][x1]["Open"];
                    high_prices_map[symbol][x] = ohlc_json["PriceBars"][x1]["High"];
                    low_prices_map[symbol][x] = ohlc_json["PriceBars"][x1]["Low"];
                    close_prices_map[symbol][x] = ohlc_json["PriceBars"][x1]["Close"];
                    datetime_map[symbol][x] = std::stof(ohlc_json["PriceBars"][x1]["BarDate"].dump().substr(6, 10));
                }
            }
        }
        catch (FXException const& e)
        {
            price_update_failure_count[symbol] += 1;
            BOOST_LOG_TRIVIAL(warning) << "OHLC Update Failed " << symbol << "; Number of Failed Loops: " << price_update_failure_count[symbol]
                                       << "Error Message: " << e.what();
        }
        catch (std::invalid_argument const& e)
        {
            price_update_failure_count[symbol] += 1;
            BOOST_LOG_TRIVIAL(warning) << "OHLC Update Failed " << symbol << "; Number of Failed Loops: " << price_update_failure_count[symbol]
                                       << "Error Message: " << e.what() << " | Problem with std::stoi or std::stof.";
        }
    }
}

std::expected<bool, FXException> FXOrderManagement::pause_till_next_bar()
{
    next_bar_timestamp = last_bar_timestamp + update_frequency_seconds;

    std::size_t const time_next_bar_will_be_ready = next_bar_timestamp + update_frequency_seconds;

    std::size_t timestamp_now = (std::chrono::system_clock::now().time_since_epoch()).count() * std::chrono::system_clock::period::num /
                                std::chrono::system_clock::period::den;

    // -----------------------------------
    // Execute Trading Model & Confirm All Data Acquired Properly
    while (! price_update_failure_count.empty() && timestamp_now <= time_next_bar_will_be_ready - 20)
    {
        std::vector<std::string> error_list;
        for (auto& [symbol, count] : price_update_failure_count)
        {
            return_price_history({symbol});
            if (! execute_list.empty()) { trade_order_sequence(); }

            if (price_update_failure_count[symbol] > max_retry_failures)
            {
                error_list.emplace_back(symbol);
                fx_symbols_to_trade.erase(remove(fx_symbols_to_trade.begin(), fx_symbols_to_trade.end(), symbol), fx_symbols_to_trade.end());
                position_multiplier[symbol] = 0;

                BOOST_LOG_TRIVIAL(error) << "Too Many Errors: " << symbol << " Removed from Trading";
            }
        }

        for (auto const& symbol : error_list) { price_update_failure_count.erase(symbol); }

        sleep(10);
        timestamp_now = (std::chrono::system_clock::now().time_since_epoch()).count() * std::chrono::system_clock::period::num /
                        std::chrono::system_clock::period::den;
    }

    // -----------------------------------
    // Wait Till New Bar Ready & Fetch OHLC
    int time_to_wait_now = time_next_bar_will_be_ready - timestamp_now;
    if (time_to_wait_now > 0)
    {
        BOOST_LOG_TRIVIAL(info) << "FX Order Management - Waiting For OHLC Bar Update";
        sleep(time_to_wait_now);
    }
    return_price_history(fx_symbols_to_trade);
    // -------------------
    return std::expected<bool, FXException> {true};
}

std::expected<bool, FXException> FXOrderManagement::execute_signals(std::vector<nlohmann::json>& trades_map)
{
    if (place_trades || emergency_close)
    {
        ++execution_loop_count;
        for (auto& trades_json : trades_map)
        {
            std::string symbol = trades_json.begin().key();
            auto trade_order_response = session.trade_order(trades_json, "MARKET");

            // // ------------
            // // Notify if any errors
            // if (! error_list.empty())
            // {
            //     for (auto const& elem : error_list) { BOOST_LOG_TRIVIAL(warning) << "Trading Error for: " << elem; }
            // }
        }

        auto active_orders_response = monitor_active_orders();
        // ------------
        // Recursive Trade Confirmation Can Only Happen A Maximum of (3) Times
        if (execution_loop_count <= 3) { verify_trades_opened(trades_map); }
        else {}
    }
    // -------------------
    return std::expected<bool, FXException> {true};
}

std::expected<bool, FXException> FXOrderManagement::monitor_active_orders()
{
    // Allow (5) seconds for market orders to fill
    sleep(5);

    auto active_orders_response = session.list_active_orders();
    if (! active_orders_response)
    {
        return std::expected<bool, FXException> {std::unexpect, active_orders_response.error().where(), active_orders_response.error().what()};
    }

    nlohmann::json active_orders_json = active_orders_response.value()["ActiveOrders"];
    for (auto& order : active_orders_json)
    {
        int const status = order["TradeOrder"]["StatusId"];
        // ---------------------------
        if (status == 1)
        {

            auto cancel_order_response = session.cancel_order(order["TradeOrder"]["OrderId"]);

            if (! cancel_order_response)
            {
                return std::expected<bool, FXException> {std::unexpect, cancel_order_response.error().where(), cancel_order_response.error().what()};
            }

            BOOST_LOG_TRIVIAL(warning) << "Canceled Order: " << cancel_order_response.value();
        }
        // ---------------------------
        std::vector<int> MAJOR_STATUS_ERROR_CODES = {6, 8, 10};
        if (std::find(MAJOR_STATUS_ERROR_CODES.begin(), MAJOR_STATUS_ERROR_CODES.end(), status) != MAJOR_STATUS_ERROR_CODES.end())
        {
            return std::expected<bool, FXException> {std::unexpect, std::source_location::current().function_name(),
                                                     "Major Order Status Error: " + std::to_string(status)};
        }
    }
    // -------------------
    return std::expected<bool, FXException> {true};
}

std::expected<bool, FXException> FXOrderManagement::verify_trades_opened(std::vector<nlohmann::json>& trades_map)
{
    auto open_positions_response = session.list_open_positions();
    if (! open_positions_response)
    {
        std::expected<bool, FXException> {std::unexpect, open_positions_response.error().where(), open_positions_response.error().what()};
    }

    nlohmann::json open_positions = open_positions_response.value()["OpenPositions"];
    for (auto& position : open_positions)
    {
        std::string const symbol = position["MarketName"];
        float const existing_quantity = position["Quantity"];
        std::string const existing_direction = position["Direction"];
        // ------------
        int iteration = 0;
        for (auto& trade_json : trades_map)
        {
            std::string key = trade_json.begin().key();
            if (key == symbol)
            {
                int const final_quantity = trade_json[symbol]["Final Quantity"];
                std::string const direction = trade_json[symbol]["Direction"];

                if (direction != existing_direction) { trade_json["Quantity"] = existing_quantity + final_quantity; }
                else if (direction == existing_direction && existing_quantity < final_quantity)
                {
                    trade_json["Quantity"] = final_quantity - existing_quantity;
                }
                else
                {
                    trades_map.erase(trades_map.begin() + iteration);
                    break;
                }
            }
            ++iteration;
        }
    }
    // ---------------------------
    // Execute Trades
    if (! trades_map.empty()) { auto execute_signal_response = execute_signals(trades_map); }
    // -------------------
    return std::expected<bool, FXException> {true};
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

    std::ifstream in(file_name);
    if (in.is_open())
    {
        nlohmann::json data = nlohmann::json::parse(in);
        in.close();

        if (paper_or_live == "PAPER") { trading_account = data["Paper_Username"].dump(); }
        else { trading_account = data["Username"].dump(); }
        if (trading_account == "null")
        {
            return std::expected<bool, FXException> {std::unexpect, std::source_location::current().function_name(),
                                                     "'Username' / 'Paper_Username' doesn't exist in user_settings.json."};
        }

        forex_api_key = data["API_Key"].dump();

        if (forex_api_key == "null")
        {
            return std::expected<bool, FXException> {std::unexpect, std::source_location::current().function_name(),
                                                     "'API_Key' doesn't exist in user_settings.json."};
        }

        if (data.contains("Positions") && data["Positions"].is_array()) { fx_symbols_to_trade = data["Positions"]; }
        else
        {
            return std::expected<bool, FXException> {std::unexpect, std::source_location::current().function_name(),
                                                     "Key 'Positions' must be an array in user_settings.json."};
        }

        if (data.contains("Order_Size") && data["Order_Size"].is_number())
        {
            order_position_size = data["Order_Size"];
            order_position_size = round(order_position_size / 1000.0) * 1000;
        }
        else
        {
            return std::expected<bool, FXException> {std::unexpect, std::source_location::current().function_name(),
                                                     "Key 'Order_Size' must be a number in user_settings.json."};
        }

        update_interval = data["Update_Interval"].dump();

        if (update_interval == "null")
        {
            return std::expected<bool, FXException> {std::unexpect, std::source_location::current().function_name(),
                                                     "'Update_Interval' doesn't exist in user_settings.json."};
        }

        if (data.contains("Update_Span") && data["Update_Span"].is_number()) { update_span = data["Update_Span"]; }
        else
        {
            return std::expected<bool, FXException> {std::unexpect, std::source_location::current().function_name(),
                                                     "Key 'Update_Span' must be a number in user_settings.json."};
        }

        if (data.contains("Num_Data_Points") && data["Num_Data_Points"].is_number()) { num_data_points = data["Num_Data_Points"]; }
        else
        {
            return std::expected<bool, FXException> {std::unexpect, std::source_location::current().function_name(),
                                                     "Key 'Num_Data_Points' must be a number in user_settings.json."};
        }

        if (data.contains("Start_Hour_London_Exchange") && data["Start_Hour_London_Exchange"].is_number())
        {
            start_hr = data["Start_Hour_London_Exchange"];
        }
        else
        {
            return std::expected<bool, FXException> {std::unexpect, std::source_location::current().function_name(),
                                                     "Key 'Start_Hour_London_Exchangee' must be a number in user_settings.json."};
        }

        if (data.contains("End_Hour_London_Exchange") && data["End_Hour_London_Exchange"].is_number()) { end_hr = data["End_Hour_London_Exchange"]; }
        else
        {
            return std::expected<bool, FXException> {std::unexpect, std::source_location::current().function_name(),
                                                     "Key 'End_Hour_London_Exchange' must be a number in user_settings.json."};
        }
    }
    else
    {
        return std::expected<bool, FXException> {
            std::unexpect, std::source_location::current().function_name(),
            "User Settings File Doesn't Exist. Download 'interface_files' folder from repository and include in workspace"};
    }
    // -------------------
    return std::expected<bool, FXException> {true};
}

std::expected<bool, FXException> FXOrderManagement::read_active_management_file()
{
    std::string const dir = sys_path + "/interface_file";
    std::string const file_name = dir + "/active_order_management.json";
    // -------------------
    try
    {
        bool valid = std::filesystem::is_directory(dir);
        if (! valid) { valid = std::filesystem::create_directories(dir); }
        if (! valid) { throw std::filesystem::filesystem_error {"Could not make directory", std::error_code {}}; }
    }
    catch (std::filesystem::filesystem_error const& e)
    {
        return std::expected<bool, FXException> {std::unexpect, std::source_location::current().function_name(), e.what()};
    }

    std::vector<std::string> copy_of_fx_symbols = fx_symbols_to_trade;
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
            copy_of_fx_symbols.erase(remove(copy_of_fx_symbols.begin(), copy_of_fx_symbols.end(), it.key()), copy_of_fx_symbols.end());
        }
    }

    // Add
    for (std::string const& symbol : copy_of_fx_symbols) { data[symbol] = {{"Close Immediately", false}, {"Close On Trade Signal Change", false}}; }

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

    auto margin_info_response = session.get_margin_info();

    if (! margin_info_response)
    {
        return std::expected<bool, FXException> {std::unexpect, margin_info_response.error().where(), margin_info_response.error().what()};
    }

    nlohmann::json margin_json = margin_info_response.value();

    float equity_total = 0, margin_total = 0;
    if (margin_json.contains("netEquity") && margin_json["netEquity"].is_number()) { equity_total = margin_json["netEquity"]; }
    else { BOOST_LOG_TRIVIAL(warning) << "'Net Equity' is not present in margin info. Profit Report will be invalid."; }
    if (margin_json.contains("margin") && margin_json["margin"].is_number()) { margin_total = margin_json["margin"]; }
    else { BOOST_LOG_TRIVIAL(warning) << "'Margin' is not present in margin info. Profit Report will be invalid."; }

    auto open_positions_response = session.list_open_positions();
    if (! open_positions_response)
    {
        std::expected<bool, FXException> {std::unexpect, open_positions_response.error().where(), open_positions_response.error().what()};
    }

    nlohmann::json open_positions = open_positions_response.value()["OpenPositions"];
    for (auto& position : open_positions)
    {
        std::string const market_name = position["MarketName"];
        int const direction = (position["Direction"] == "buy") ? 1 : -1;
        float const entry_price = position["Price"];
        int const quantity = position["Quantity"];

        float current_price = 0;
        auto prices_response = session.get_prices({market_name});
        if (! prices_response)
        {
            return std::expected<bool, FXException> {std::unexpect, prices_response.error().where(), prices_response.error().what()};
        }

        nlohmann::json prices_json = prices_response.value();
        if (prices_json["PriceTicks"][0]["Price"].dump() != "null" && prices_json["PriceTicks"][0]["Price"].is_number())
        {
            float current_price = prices_json["PriceTicks"][0]["Price"];
        }
        else { BOOST_LOG_TRIVIAL(warning) << "'Current Price' is not present in price request. " << market_name << " will be invalid."; }

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

    std::string const dir = sys_path + "/interface_file/reports";
    std::string const file_name = dir + "/FX_Management_Report_" + fx_utilities.get_todays_date() + ".json";
    // -------------------
    try
    {
        bool valid = std::filesystem::is_directory(dir);
        if (! valid) { valid = std::filesystem::create_directories(dir); }
        if (! valid) { throw std::filesystem::filesystem_error {"Could not make directory", std::error_code {}}; }
    }
    catch (std::filesystem::filesystem_error const& e)
    {
        return std::expected<bool, FXException> {std::unexpect, std::source_location::current().function_name(), e.what()};
    }
    // -------------------
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
