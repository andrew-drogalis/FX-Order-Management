// Copyright 2024, Andrew Drogalis
// GNU License

#include "fx_order_management.h"

#include "boost/log/trivial.hpp"
#include "boost/log/utility/setup/console.hpp"
#include "gain_capital_api/gain_capital_api.h"
#include "keychain/keychain.h"

#include "json/json.hpp"
#include <array>
#include <chrono>
#include <ctime>
#include <exception>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <valarray>
#include <vector>

#define VAR_DECLS
#define VAR_DECLS_ORDER

#include "credentials.h"
#include "fx_market_time.h"
#include "fx_utilities.h"
#include "order_parameters.h"
#include "trading_model.h"

namespace fxordermgmt
{

FXOrderManagement::FXOrderManagement() {}

FXOrderManagement::~FXOrderManagement() {}

FXOrderManagement::FXOrderManagement(std::string paper_or_live, bool place_trades, int emergency_close, std::string working_directory)
    : paper_or_live(paper_or_live), place_trades(place_trades), emergency_close(emergency_close), sys_path(working_directory)
{
    if (paper_or_live == "PAPER")
    {
        trading_account = paper_account_username;
        service_id = "Test_Account";
        package = "com.gain_capital_forex.test_account";
    }
    else if (paper_or_live == "LIVE")
    {
        trading_account = account_username;
        service_id = "Live_Account";
        package = "com.gain_capital_forex.live_account";
    }

    // Optional Boost Logging to STD Output
    boost::log::add_console_log(std::cout, boost::log::keywords::format = ">> %Message%");
}

bool FXOrderManagement::initalize_order_management()
{
    fx_utilities = FXUtilities();
    if (! fx_utilities.validate_user_interval(update_interval, update_span, update_frequency_seconds)) { return false; }

    fx_utilities.init_logging(sys_path);
    if (! fx_utilities.setup_password_first_time(paper_or_live, trading_account)) { return false; }
    // -----------------
    fx_market_time = FXMarketTime(update_frequency_seconds, fx_utilities);
    fx_market_time.forex_market_time_setup();
    // -------------------------------
    for (std::string symbol : fx_symbols_to_trade) { position_multiplier[symbol] = 1; }
    live_symbols_list = fx_symbols_to_trade;
    // -------------------------------
    if (! fx_market_time.pause_till_market_open()) { return false; }

    if (! gain_capital_session()) { return false; }
    // -------------
    if (emergency_close == 1) { emergency_position_close(); }
    // -------------------------------
    output_order_information();
    read_input_information();

    return true;
}

bool FXOrderManagement::run_order_management_system()
{
    if (emergency_close == 0)
    {
        time_t time_now = time(NULL);
        BOOST_LOG_TRIVIAL(info) << "FX Order Management - Currently Running" << ctime(&time_now);
        std::cout << "For Live Updates, See Log File" << std::endl;
        // Initialize Price History & Trading Model
        return_price_history(live_symbols_list);

        for (std::string symbol : execute_list) { initalize_trading_model(symbol); }
        // Start Loop
        while (true)
        {
            if (fx_market_time.market_closed())
            {
                std::cout << "FX Order Management - Market Closed" << std::endl;
                BOOST_LOG_TRIVIAL(info) << "FX Order Management - Market Closed";
                break;
            }
            // ----------------
            bool success = pause_next_bar();

            get_trading_model_signal();

            read_input_information();

            build_trades_map();

            BOOST_LOG_TRIVIAL(info) << "FX Order Management - Loop Completed";
        }
    }
    return true;
}

// ==============================================================================================
// Gain Capital API
// ==============================================================================================
bool FXOrderManagement::gain_capital_session()
{
    keychain::Error error {};
    std::string password = keychain::getPassword(package, service_id, trading_account, error);

    if (error.type == keychain::ErrorType::NotFound)
    {
        std::cerr << "Password not found. Closing Application." << std::endl;
        return false;
    }
    else if (error)
    {
        std::cerr << error.message << std::endl;
        return false;
    }
    // ---------------------------------------
    session = gaincapital::GCapiClient(trading_account, password, forex_api_key);
    if (! session.authenticate_session()) { return false; }

    BOOST_LOG_TRIVIAL(info) << "FX Order Management - New Gain Capital Session Initiated";

    if (! live_symbols_list.empty() && session.get_market_ids(live_symbols_list) == std::unordered_map<std::string, int>()) { return false; }

    return true;
}

// ==============================================================================================
// Trading Model
// ==============================================================================================
void FXOrderManagement::initalize_trading_model(std::string symbol)
{
    TradingModel new_model = TradingModel(historical_data_map[symbol]);
    trading_model_map[symbol] = new_model;
    BOOST_LOG_TRIVIAL(info) << "FX Order Management - Trading Model Initialized for " << symbol;
}

void FXOrderManagement::get_trading_model_signal()
{
    main_signals = {};
    TradingModel trading_model_instance;
    // --------
    for (std::string symbol : execute_list)
    {
        if (trading_model_map.count(symbol)) { trading_model_instance = trading_model_map[symbol]; }
        else
        {
            initalize_trading_model(symbol);
            trading_model_instance = trading_model_map[symbol];
        }
        trading_model_instance.receive_latest_market_data(historical_data_map[symbol]);
        int prediction = trading_model_instance.send_trading_signal();
        main_signals[symbol] = prediction;
        BOOST_LOG_TRIVIAL(debug) << "Trading Model Output - " << symbol << "; Prediction: " << prediction;
    }
}

// ==============================================================================================
// Forex Order Management
// ==============================================================================================
void FXOrderManagement::build_trades_map()
{
    nlohmann::json trades_map = {};

    std::vector<std::string> symbols_in_main_signals;
    for (std::unordered_map<std::string, int>::iterator it = main_signals.begin(); it != main_signals.end(); ++it)
    {
        symbols_in_main_signals.push_back(it->first);
    }
    // ------------------------------------
    // Check Account Positions
    open_positions = session.list_open_positions();
    bool forex_is_exit_only = fx_market_time.forex_market_exit_only();

    if (! forex_is_exit_only)
    {
        // ------------
        for (auto position : open_positions)
        {
            std::string symbol = position["MarketName"];
            // ------------
            if (find(symbols_in_main_signals.begin(), symbols_in_main_signals.end(), symbol) != symbols_in_main_signals.end())
            {
                symbols_in_main_signals.erase(remove(symbols_in_main_signals.begin(), symbols_in_main_signals.end(), symbol),
                                              symbols_in_main_signals.end());

                int base_quantity;
                if (position_multiplier.count(symbol)) { base_quantity = round(position_multiplier[symbol] * order_position_size / 1000) * 1000; }
                else { base_quantity = order_position_size; }
                int live_quantity = position["Quantity"];
                std::string direction = position["Direction"];
                std::string new_direction = (direction == "buy") ? "sell" : "buy";
                // ---------------------------
                if (direction == "buy")
                {
                    // ------------
                    if (main_signals[symbol] == 1)
                    {
                        // ------------
                        if (live_quantity < base_quantity)
                        {
                            trades_map[symbol] = {
                                {"Quantity", base_quantity - live_quantity}, {"Direction", direction}, {"Final Quantity", base_quantity}};
                        }
                    }
                    else if (main_signals[symbol] == 0)
                    {
                        trades_map[symbol] = {{"Quantity", live_quantity}, {"Direction", new_direction}, {"Final Quantity", 0}};
                    }
                    else
                    {
                        trades_map[symbol] = {
                            {"Quantity", base_quantity + live_quantity}, {"Direction", new_direction}, {"Final Quantity", base_quantity}};
                    }
                }
                // ---------------------------
                else if (direction == "sell")
                {
                    // ------------
                    if (main_signals[symbol] == -1)
                    {
                        // ------------
                        if (live_quantity < base_quantity)
                        {
                            trades_map[symbol] = {
                                {"Quantity", base_quantity - live_quantity}, {"Direction", direction}, {"Final Quantity", base_quantity}};
                        }
                    }
                    else if (main_signals[symbol] == 0)
                    {
                        trades_map[symbol] = {{"Quantity", live_quantity}, {"Direction", new_direction}, {"Final Quantity", 0}};
                    }
                    else
                    {
                        trades_map[symbol] = {
                            {"Quantity", base_quantity + live_quantity}, {"Direction", new_direction}, {"Final Quantity", base_quantity}};
                    }
                }
            }
        }
        // ---------------------------
        if (! symbols_in_main_signals.empty())
        {
            for (int x = 0; x < symbols_in_main_signals.size(); x++)
            {
                std::string symbol = symbols_in_main_signals[x];
                int position_signal = main_signals[symbol];
                int base_quantity;
                if (position_multiplier.count(symbol)) { base_quantity = round(position_multiplier[symbol] * order_position_size / 1000) * 1000; }
                else { base_quantity = order_position_size; }
                if (position_signal != 0 && base_quantity != 0)
                {
                    std::string direction = (position_signal == 1) ? "buy" : "sell";
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
            std::string symbol = position["MarketName"];
            int quantity = position["Quantity"];
            std::string new_direction = (position["Direction"] == "buy") ? "sell" : "buy";
            trades_map[symbol] = {{"Quantity", quantity}, {"Direction", new_direction}, {"Final Quantity", 0}};
        }
    }
    // ---------------------------
    if (! trades_map.empty())
    {
        execution_loop_count = 0;
        execute_signals(trades_map);
    }
    // ------------
    nlohmann::json margin_info = session.get_margin_info();
    equity_total = (! margin_info.empty()) ? static_cast<float>(margin_info["netEquity"]) : 0.0;
    margin_total = (! margin_info.empty()) ? static_cast<float>(margin_info["margin"]) : 0.0;
    output_order_information();
}

void FXOrderManagement::emergency_position_close()
{
    nlohmann::json trades_map = {};
    // ------------------------------------
    // Check Account Positions
    open_positions = session.list_open_positions();
    for (auto position : open_positions)
    {
        // ------------
        std::string symbol = position["MarketName"];
        int quantity = position["Quantity"];
        std::string direction = position["Direction"];
        std::string new_direction = (direction == "buy") ? "sell" : "buy";
        trades_map[symbol] = {{"Quantity", quantity}, {"Direction", new_direction}, {"Final Quantity", 0}};
    }
    // ---------------------------
    if (! trades_map.empty())
    {
        execution_loop_count = 0;
        execute_signals(trades_map);
    }
    // ------------
    nlohmann::json margin_info = session.get_margin_info();
    equity_total = (! margin_info.empty()) ? static_cast<float>(margin_info["netEquity"]) : 0.0;
    margin_total = (! margin_info.empty()) ? static_cast<float>(margin_info["margin"]) : 0.0;
    output_order_information();
}

/*
This Function is Not Called
User Has Option to Replace OHLC w/ Tick Data
*/
void FXOrderManagement::return_tick_history(std::vector<std::string> symbols_list)
{
    std::unordered_map<std::string, nlohmann::json> data = session.get_prices(symbols_list, num_data_points);
}

void FXOrderManagement::return_price_history(std::vector<std::string> symbols_list)
{
    // Get Historical Forex Data
    BOOST_LOG_TRIVIAL(info) << "FX Order Management - Attempting to Fetch Price History";
    long unsigned int timestamp_now;
    long unsigned int start = (std::chrono::system_clock::now().time_since_epoch()).count() * std::chrono::system_clock::period::num /
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
                long unsigned int last_timestamp = std::stoi(last_bar_datetime.substr(6, 10));
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
        timestamp_now = (std::chrono::system_clock::now().time_since_epoch()).count() * std::chrono::system_clock::period::num /
                        std::chrono::system_clock::period::den;
        if (loop_list.empty() || timestamp_now - start > 10) { break; }
        else
        {
            price_data_update_failure = {};
            sleep(1.5);
        }
    }
    // Confirm Latest Data Provided
    // --------------------------------
    std::vector<long unsigned int> symbol_update_datetimes = {};
    for (std::unordered_map<std::string, long unsigned int>::iterator it = price_data_update_datetime.begin(); it != price_data_update_datetime.end();
         ++it)
    {
        symbol_update_datetimes.push_back(it->second);
    }
    last_bar_timestamp =
        (! symbol_update_datetimes.empty()) ? *max_element(symbol_update_datetimes.begin(), symbol_update_datetimes.end()) : last_bar_timestamp;

    for (std::unordered_map<std::string, long unsigned int>::iterator it = price_data_update_datetime.begin(); it != price_data_update_datetime.end();
         ++it)
    {
        if (it->second != last_bar_timestamp)
        {
            price_data_update_failure.push_back(it->first);
            try
            {
                completed_list.erase(remove(completed_list.begin(), completed_list.end(), it->first), completed_list.end());
            }
            catch (...)
            {
            }
        }
    }

    for (std::string symbol : price_data_update_failure)
    {
        BOOST_LOG_TRIVIAL(error) << "FX Order Management - Price Data Update Failure " << symbol << "; Will Retry After Trades Placed";
    }
    // ------------------------------------
    // Seperate Data From OHLC into Individual Vectors
    execute_list = {};
    for (std::string symbol : completed_list)
    {
        nlohmann::json ohlc_data = data[symbol];
        int data_length = ohlc_data.size();
        if (data_length >= num_data_points)
        {
            execute_list.push_back(symbol);
            std::vector<float> open_data = {};
            std::vector<float> high_data = {};
            std::vector<float> low_data = {};
            std::vector<float> close_data = {};
            std::vector<float> datetime_data = {};
            // ------------
            for (int x = 0; x < num_data_points; x++)
            {
                int const x1 = data_length - num_data_points + x;
                std::string bar_datetime = ohlc_data[x1]["BarDate"];

                open_data.push_back(ohlc_data[x1]["Open"]);
                high_data.push_back(ohlc_data[x1]["High"]);
                low_data.push_back(ohlc_data[x1]["Low"]);
                close_data.push_back(ohlc_data[x1]["Close"]);
                datetime_data.push_back(std::stof(bar_datetime.substr(6, 10)));
            }
            historical_data_map[symbol] = {
                {"Open", open_data}, {"High", high_data}, {"Low", low_data}, {"Close", close_data}, {"Datetime", datetime_data}};
        }
        else
        {
            data_error_list.push_back(symbol);
            BOOST_LOG_TRIVIAL(warning) << "Data Aquisition Error; Length Too Short: " << symbol << " - " << data_length;
        }
    }
}

bool FXOrderManagement::pause_next_bar()
{
    next_bar_timestamp = last_bar_timestamp + update_frequency_seconds;
    long unsigned int time_next_bar_will_be_ready = next_bar_timestamp + update_frequency_seconds;
    unsigned int timestamp_now;
    // -----------------------------------
    // Execute Trading Model & Confirm All Data Aquired Properly
    for (int x = 0; x < 5; x++)
    {
        timestamp_now = (std::chrono::system_clock::now().time_since_epoch()).count() * std::chrono::system_clock::period::num /
                        std::chrono::system_clock::period::den;
        if (! price_data_update_failure.empty())
        {
            if (timestamp_now <= time_next_bar_will_be_ready - 15)
            {
                return_price_history(price_data_update_failure);
                if (! execute_list.empty())
                {
                    get_trading_model_signal();
                    read_input_information();
                    build_trades_map();
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
    for (std::string symbol : price_data_update_failure)
    {
        price_data_update_failure_count[symbol] += 1;
        BOOST_LOG_TRIVIAL(warning) << "Updates Failed " << symbol << "; Number of Failed Loops: " << price_data_update_failure_count[symbol]
                                   << std::endl;
    }
    // -----------------------------------
    // Notify Data Error
    if (! data_error_list.empty())
    {
        for (std::string symbol : data_error_list)
        {
            try
            {
                live_symbols_list.erase(remove(live_symbols_list.begin(), live_symbols_list.end(), symbol), live_symbols_list.end());
                price_data_update_datetime.erase(symbol);

                BOOST_LOG_TRIVIAL(error) << "Data Aquisition Error: " << symbol << std::endl;
                position_multiplier[symbol] = 0;
                std::cout << "Data Aquisition Error: " << symbol << std::endl;
            }
            catch (...)
            {
            }
        }
        data_error_list = {};
    }
    // -----------------------------------
    timestamp_now = (std::chrono::system_clock::now().time_since_epoch()).count() * std::chrono::system_clock::period::num /
                    std::chrono::system_clock::period::den;
    float time_to_wait_now = time_next_bar_will_be_ready - timestamp_now;
    if (time_to_wait_now > 0)
    {
        BOOST_LOG_TRIVIAL(info) << "FX Order Management - Waiting For OHLC Bar Update";
        sleep(time_to_wait_now);
    }
    // ------------
    return_price_history(live_symbols_list);
    // ------------
    return true;
}

void FXOrderManagement::execute_signals(nlohmann::json trade_dict)
{
    if (place_trades || emergency_close == 1)
    {
        execution_loop_count += 1;
        std::vector<std::string> error_list = session.trade_order(trade_dict, "MARKET");
        if (! error_list.empty())
        {
            for (std::string error_item : error_list) { BOOST_LOG_TRIVIAL(warning) << "Trading Error List: " << error_item << std::endl; }
        }
        // ------------------------------------
        // Check On Active Orders
        for (int x = 0; x < 3; x++)
        {
            int pending_order_count = 0;
            sleep(1);// Pause For Each Loop
            nlohmann::json active_orders = session.list_active_orders();
            for (auto item : active_orders)
            {
                int status = item["TradeOrder"]["StatusId"];
                if (status == 1)
                {
                    pending_order_count += 1;
                    if (x == 2)
                    {
                        nlohmann::json resp = session.cancel_order(item["TradeOrder"]["OrderId"]);
                        BOOST_LOG_TRIVIAL(warning) << "Cancelled Order: " << resp << std::endl;
                    }
                }
                // ------------
                std::vector<int> major_status_errors = {6, 8, 10};
                if (std::find(major_status_errors.begin(), major_status_errors.end(), status) != major_status_errors.end())
                {
                    // Count Doesn't Increase Because Order Will Never Finish
                    BOOST_LOG_TRIVIAL(error) << "Major Order Status Error: " << status << std::endl;
                }
            }
            // ------------
            if (pending_order_count == 0) { break; }
        }
        // ---------------------------
        if (execution_loop_count < 4)
        {
            // Recursive Trade Confirmation Can Only Happen A Maximum of (3) Times
            verify_trades_opened(trade_dict);
        }
        else
        {
            // ------------
            for (std::string symbol : error_list)
            {
                BOOST_LOG_TRIVIAL(warning) << "Trading Error, Symbol Removed: " << symbol << std::endl;
                position_multiplier[symbol] = 0;
            }
        }
    }
}

void FXOrderManagement::verify_trades_opened(nlohmann::json trade_dict)
{
    nlohmann::json trades_map = {};
    std::vector<std::string> keys_list = {};

    for (nlohmann::json::iterator it = trade_dict.begin(); it != trade_dict.end(); ++it) { keys_list.push_back(it.key()); }
    // ------------------------------------
    // Check Account Positions
    open_positions = session.list_open_positions();
    for (auto position : open_positions)
    {
        std::string symbol = position["MarketName"];
        float existing_quantity = position["Quantity"];
        std::string existing_direction = position["Direction"];
        // ------------
        if (find(keys_list.begin(), keys_list.end(), symbol) != keys_list.end())
        {
            // ------------
            keys_list.erase(remove(keys_list.begin(), keys_list.end(), symbol), keys_list.end());
            std::string direction = trade_dict[symbol]["Direction"];
            int final_quantity = trade_dict[symbol]["Final Quantity"];
            // ------------
            if (direction != existing_direction)
            {
                trades_map[symbol] = {{"Quantity", existing_quantity + final_quantity}, {"Direction", direction}, {"Final Quantity", final_quantity}};
            }
            else if (direction == existing_direction && existing_quantity < final_quantity)
            {
                trades_map[symbol] = {{"Quantity", final_quantity - existing_quantity}, {"Direction", direction}, {"Final Quantity", final_quantity}};
            }
        }
    }
    // ---------------------------
    // Confirm New Positions Are Open
    for (std::string symbol : keys_list)
    {
        if (trade_dict[symbol]["Final Quantity"] != 0) { trades_map[symbol] = trade_dict[symbol]; }
    }
    // ---------------------------
    // Execute Trades
    if (! trades_map.empty()) { execute_signals(trades_map); }
}

// ==============================================================================================
// Forex Order File I/O
// ==============================================================================================
bool FXOrderManagement::read_input_information()
{
    std::string date = fx_utilities.get_todays_date();
    std::string file_name = sys_path + "/" + date + "_FX_Order_Deletion.json";
    nlohmann::json data;
    std::vector<std::string> double_check = live_symbols_list;
    try
    {
        std::ifstream f(file_name);
        data = nlohmann::json::parse(f);
    }
    catch (...)
    {
        for (std::string symbol : live_symbols_list) { data[symbol] = {{"Close Immediately", false}, {"Close On Trade Signal Change", false}}; }
    }
    for (nlohmann::json::iterator it = data.begin(); it != data.end(); ++it)
    {
        nlohmann::json json_value = it.value();
        if (json_value.contains("Close Immediately"))
        {
            if (json_value["Close Immediately"] == true) { main_signals[it.key()] = 0; }
        }
        if (json_value.contains("Close On Trade Signal Change"))
        {
            if (json_value["Close On Trade Signal Change"] == true) { position_multiplier[it.key()] = 0; }
        }
        double_check.erase(remove(double_check.begin(), double_check.end(), it.key()), double_check.end());
    }

    for (std::string symbol : double_check) { data[symbol] = {{"Close Immediately", false}, {"Close On Trade Signal Change", false}}; }
    std::string output_string = data.dump(4);
    try
    {
        std::ofstream out(file_name);
        out << output_string;
        out.close();
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool FXOrderManagement::output_order_information()
{
    nlohmann::json current_performance = {};
    nlohmann::json current_positions = {};
    time_t time_now = time(NULL);
    std::string time_str = ctime(&time_now);
    float profit = 0;
    float profit_percent = 0;

    for (auto position : open_positions)
    {
        std::string market_name = position["MarketName"];
        int direction = (position["Direction"] == "buy") ? 1 : -1;
        float entry_price = position["Price"];
        int quantity = position["Quantity"];

        std::unordered_map<std::string, nlohmann::json> response = session.get_prices({market_name});
        float current_price = (! response.empty()) ? static_cast<float>(response[market_name][0]["Price"]) : 0.0;

        profit = round((current_price - entry_price) * 100000) / 100000 * direction;
        profit_percent = (entry_price != 0) ? round(profit * 10000 / entry_price) / 100 : 0;

        current_positions[market_name] = {{"Direction", position["Direction"]},
                                          {"Quantity", position["Quantity"]},
                                          {"Entry Price", position["Price"]},
                                          {"Current Price", current_price},
                                          {"Profit", profit},
                                          {"Profit Percent", profit_percent}};
    }
    // ---------------------------
    if (init_equity == 0) { init_equity = equity_total; }
    float total_profit = round((equity_total - init_equity) * 100) / 100;
    float total_profit_percent = (init_equity != 0) ? round(profit * 10000 / init_equity) / 100 : 0;

    current_performance["Performance Information"] = {{"Inital Funds", init_equity},
                                                      {"Current Funds", equity_total},
                                                      {"Margin Utilized", margin_total},
                                                      {"Profit Cumulative", total_profit},
                                                      {"Profit Percent Cumulative", total_profit_percent}};

    current_performance["Position Information"] = current_positions;
    current_performance["Last Updated"] = time_str;

    std::string date = fx_utilities.get_todays_date();
    std::string file_name = sys_path + "/" + date + "_FX_Order_Information.json";
    std::string output_string = current_performance.dump(4);
    try
    {
        std::ofstream out(file_name);
        out << output_string;
        out.close();
    }
    catch (...)
    {
        return false;
    }
    return true;
}

}// namespace fxordermgmt
