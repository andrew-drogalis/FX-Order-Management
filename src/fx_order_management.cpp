
#include <fx_order_management.h>

#include <iostream>
#include <array>
#include <string>
#include <vector>
#include <unistd.h> 
#include <chrono>
#include <ctime>
#include <fstream>
#include <map>
#include <exception>
#include <format>
#include <valarray>

#include <gain_capital_api.h>
#include <json.hpp>
#include <keychain.h>
#include <boost/log/trivial.hpp>

#define VAR_DECLS
#define VAR_DECLS_ORDER

#include <fx_market_time.h>
#include <fx_utilities.h>
#include <trading_model.h>
#include <credentials.h>
#include <order_parameters.h>

namespace fxordermgmt {

FXOrderManagement::FXOrderManagement() { }

FXOrderManagement::~FXOrderManagement() { }

FXOrderManagement::FXOrderManagement(std::string account, bool place_trades, int emergency_close, std::string working_directory) : trading_account(account), place_trades(place_trades), emergency_close(emergency_close), sys_path(working_directory) {

    if (account == "PAPER") {
        trading_account = paper_account_username;
        service_id = "Test_Account";
        package = "com.gain_capital_forex.test_account";
    }
    else if (account == "LIVE") {
        trading_account = account_username;
        service_id = "Live_Account";
        package = "com.gain_capital_forex.live_account";
    }

    fx_utilities.init_logging(working_directory);
    fx_utilities.setup_password_first_time(account, trading_account);

    // Confirm Order Parameters are Valid
    transform(update_interval.begin(), update_interval.end(), update_interval.begin(), ::toupper);

    std::vector<int> SPAN_M = {1, 2, 3, 5, 10, 15, 30}; // Span intervals for minutes
    std::vector<int> SPAN_H = {1, 2, 4, 8}; // Span intervals for hours
    std::vector<std::string> INTERVAL = {"HOUR", "MINUTE"};


    if (std::find(INTERVAL.begin(), INTERVAL.end(), update_interval) == INTERVAL.end()) {
        std::cerr << "Interval Error - Provide one of the following intervals: 'HOUR', 'MINUTE'" << std::endl;
        std::terminate();
    }

    if (update_interval == "HOUR") {
        if (std::find(SPAN_H.begin(), SPAN_H.end(), update_span) == SPAN_H.end()) {
            std::cerr << "Span Hour Error - Provide one of the following spans: 1, 2, 4, 8" << std::endl;
            std::terminate();
        }
        update_frequency_seconds = 3600 * update_span;
    }
    else if (update_interval == "MINUTE") {
        if (std::find(SPAN_M.begin(), SPAN_M.end(), update_span) == SPAN_M.end()) {
            std::cerr << "Span Minute Error - Provide one of the following spans: 1, 2, 3, 5, 10, 15, 30" << std::endl;
            std::terminate();
        }
        update_frequency_seconds = 60 * update_span;
    }

    // -----------------
    fx_market_time = FXMarketTime(update_frequency_seconds);
    fx_market_time.forex_market_time_setup();
    fx_market_time.pause_till_market_open();
    // -------------------------------
    // Maps
    trading_model_map = {};
    historical_data_map = {};
    price_data_update_datetime = {};
    price_data_update_failure_count = {};
    position_multiplier = {};
    for (std::string symbol : fx_symbols_to_trade) { position_multiplier[symbol] = 1; }
    // Vectors
    live_symbols_list = fx_symbols_to_trade;    
    open_positions = {};
    data_error_list = {};
    price_data_update_failure = {};
    // Int
    next_bar_timestamp = 0;
    margin_total = equity_total = init_equity = execution_loop_count = 0;
    // -------------------------------
    gain_capital_session();
    // -------------
    if (emergency_close == 1) { emergency_position_close(); }
    // -------------------------------
    output_order_information();
    read_input_information();
}

void FXOrderManagement::run_order_management_system() {
    
    if (emergency_close == 0) {

        time_t time_now = time(NULL);
        std::cout << "FX Order Management - Currently Running " << ctime(&time_now) << std::endl;
        BOOST_LOG_TRIVIAL(info) << "FX Order Management - Currently Running";
        std::cout << "For Live Updates, See Log File" << std::endl;

        // Initialize Price History & Trading Model
        return_price_history(live_symbols_list);

        for (std::string symbol : execute_list) { 
            initalize_trading_model(symbol); 
        }

        // Start Loop
        while (true) {
            bool market_is_closed = fx_market_time.market_closed();
            if (market_is_closed) {
                std::cout << "FX Order Management - Market Closed" << std::endl;
                BOOST_LOG_TRIVIAL(info) << "FX Order Management - Market Closed"; break;
            }
            // ----------------
            pause_next_bar();

            get_trading_model_signal();

            read_input_information();

            build_trades_map();

            BOOST_LOG_TRIVIAL(info) << "FX Order Management - Loop Completed";
        }
    }
}
    
// --------- [Gain Capital API Code] ---------
void FXOrderManagement::gain_capital_session() {

    keychain::Error error{};
    std::string password = keychain::getPassword(package, service_id, trading_account, error);
    
    if (error.type == keychain::ErrorType::NotFound) {
        std::cerr << "Password not found. Closing Application." << std::endl;
        std::terminate(); 
    } else if (error) {
        std::cerr << error.message << std::endl;
        std::terminate();
    }
    // --------
    session = GCapiClient(trading_account, password, forex_api_key);

    BOOST_LOG_TRIVIAL(info) << "FX Order Management - New Gain Capital Session Initiated";

    session.get_market_ids(live_symbols_list);
}

// --------- [Trading Model Code] ---------
void FXOrderManagement::initalize_trading_model(std::string symbol) {

    TradingModel new_model = TradingModel(historical_data_map[symbol]);
    trading_model_map[symbol] = new_model; 

    BOOST_LOG_TRIVIAL(info) << "FX Order Management - Trading Model Initialized for " << symbol;
}

void FXOrderManagement::get_trading_model_signal() {

    main_signals = {};
    TradingModel trading_model_instance;
    // --------
    for (std::string symbol : execute_list) {
        try {
            trading_model_instance = trading_model_map[symbol];
        }
        catch (...) { 
            initalize_trading_model(symbol);
            trading_model_instance = trading_model_map[symbol];
        }

        trading_model_instance.receive_latest_market_data(historical_data_map[symbol]);

        int prediction = trading_model_instance.send_trading_signal();

        main_signals[symbol] = prediction;

        BOOST_LOG_TRIVIAL(debug) << "Trading Model Output - " << symbol << "; Prediction: " << prediction;
    }
}

// --------------------------------------------
void FXOrderManagement::build_trades_map() {

    nlohmann::json trades_map = {};

    std::vector<std::string> symbols_in_main_signals;
    for(std::map<std::string,int>::iterator it = main_signals.begin(); it != main_signals.end(); ++it) {
        symbols_in_main_signals.push_back(it->first);
    }

    // ------------------------------------
    // Check Account Positions
    open_positions = session.list_open_positions();
    bool forex_is_exit_only = fx_market_time.forex_market_exit_only();

    if (!forex_is_exit_only) {
        // ------------
        for (auto position : open_positions) {
            std::string symbol = position["MarketName"];
            // ------------
            if (find(symbols_in_main_signals.begin(),symbols_in_main_signals.end(),symbol) != symbols_in_main_signals.end()) {

                symbols_in_main_signals.erase(remove(symbols_in_main_signals.begin(),symbols_in_main_signals.end(),symbol), symbols_in_main_signals.end());

                int base_quantity;
                try {
                    base_quantity = round(position_multiplier[symbol]*order_position_size / 1000) * 1000;
                }
                catch (...) { base_quantity = order_position_size; }

                int live_quantity = position["Quantity"];
                std::string direction = position["Direction"];
                std::string new_direction = (direction == "buy") ? "sell" : "buy";
                // ---------------------------
                if (direction == "buy") {
                    // ------------
                    if (main_signals[symbol] == 1) {
                        // ------------
                        if (live_quantity < base_quantity) {
                            trades_map[symbol] = {
                                {"Quantity", base_quantity-live_quantity},
                                {"Direction", direction},
                                {"Final Quantity", base_quantity}
                                };
                        }
                    }
                    else if (main_signals[symbol] == 0) { 
                        trades_map[symbol] = {
                            {"Quantity", live_quantity},
                            {"Direction", new_direction},
                            {"Final Quantity", 0}
                            };
                    }
                    else {
                        trades_map[symbol] = {
                            {"Quantity", base_quantity+live_quantity},
                            {"Direction", new_direction},
                            {"Final Quantity", base_quantity}
                            };
                    }
                }
                // ---------------------------
                else if (direction == "sell") { 
                    // ------------
                    if (main_signals[symbol] == -1) {
                        // ------------
                        if (live_quantity < base_quantity) { 
                            trades_map[symbol] = {
                                {"Quantity", base_quantity-live_quantity},
                                {"Direction", direction},
                                {"Final Quantity", base_quantity}
                                };
                        }
                    }
                    else if (main_signals[symbol] == 0) {
                        trades_map[symbol] = {
                            {"Quantity",live_quantity},
                            {"Direction", new_direction},
                            {"Final Quantity", 0}
                            };
                    }
                    else {
                        trades_map[symbol] =  {
                            {"Quantity", base_quantity+live_quantity},
                            {"Direction", new_direction},
                            {"Final Quantity", base_quantity}
                            };
                    }
                }
            }
        }
        // ---------------------------
        if (!symbols_in_main_signals.empty()) {
            for (int x = 0; x < symbols_in_main_signals.size(); x++) {
                std::string symbol = symbols_in_main_signals[x];
                int position_signal = main_signals[symbol];
                int base_quantity;
                try {
                    base_quantity = round(position_multiplier[symbol]*order_position_size / 1000) * 1000;
                }
                catch (...) { base_quantity = order_position_size; }

                if (position_signal != 0 && base_quantity != 0) {
                    std::string direction = (position_signal == 1) ? "buy" : "sell";

                    trades_map[symbol] = {
                            {"Quantity", base_quantity},
                            {"Direction", direction},
                            {"Final Quantity", base_quantity}
                            };
                }
            }
        }
    }
    // ------------------------------------
    // Exit Only Positions
    else {
        for (auto position : open_positions) {
            std::string symbol = position["MarketName"];
            int quantity = position["Quantity"];
            std::string new_direction = (position["Direction"] == "buy") ? "sell" : "buy";
      
            trades_map[symbol] = {
                {"Quantity", quantity},
                {"Direction", new_direction},
                {"Final Quantity", 0}
                };
        }
    }
    
    // ---------------------------
    if (!trades_map.empty()) {
        execution_loop_count = 0; 
        execute_signals(trades_map);
    }
    // ------------
    nlohmann::json margin_info = session.get_margin_info();
    equity_total = margin_info["netEquity"];
    margin_total = margin_info["margin"];
    output_order_information();
}

void FXOrderManagement::emergency_position_close() {
    nlohmann::json trades_map = {};
    // ------------------------------------
    // Check Account Positions
    open_positions = session.list_open_positions();
    for (auto position : open_positions) {
        // ------------
        std::string symbol = position["MarketName"];
        int quantity = position["Quantity"];
        std::string direction = position["Direction"];
        std::string new_direction = (direction == "buy") ? "sell" : "buy";

        trades_map[symbol] = {
            {"Quantity", quantity},
            {"Direction", new_direction},
            {"Final Quantity", 0}
            };
    }
    // ---------------------------
    if (!trades_map.empty()) {
        execution_loop_count = 0;
        execute_signals(trades_map);
    }
    // ------------
    nlohmann::json margin_info = session.get_margin_info();
    equity_total = margin_info["netEquity"];
    margin_total = margin_info["margin"];
    output_order_information();
}

// This Function is Not Called
// User Has Option to Replace OHLC w/ Tick Data
void FXOrderManagement::return_tick_history(std::vector<std::string> symbols_list) {
    std::map<std::string, nlohmann::json> data = session.get_prices(symbols_list, num_data_points);
}

void FXOrderManagement::return_price_history(std::vector<std::string> symbols_list) {
    // Get Historical Forex Data
    BOOST_LOG_TRIVIAL(info) << "FX Order Management - Attempting to Fetch Price History";
    long unsigned int timestamp_now;
    long unsigned int start = (std::chrono::system_clock::now().time_since_epoch()).count() * std::chrono::system_clock::period::num / std::chrono::system_clock::period::den;
    price_data_update_datetime = {};
    price_data_update_failure = {};
    std::map<std::string, nlohmann::json> data;
    std::vector<std::string> loop_list = symbols_list;
    std::vector<std::string> completed_list = {};
 
    while (true) {
        data = session.get_ohlc(loop_list, update_interval, num_data_points, update_span);
        // ---------------------------
        for (std::string symbol : loop_list) { 
            try {
                nlohmann::json symbol_data = data[symbol];
                nlohmann::json last_bar = symbol_data[symbol_data.size()-1];
                std::string last_bar_datetime = last_bar["BarDate"];
  
                long unsigned int last_timestamp = std::stoi(last_bar_datetime.substr(6,10));

                if (last_timestamp >= next_bar_timestamp) {
                    completed_list.push_back(symbol);
                    price_data_update_datetime[symbol] = last_timestamp;
                }
            }
            catch (...) { 
                price_data_update_failure.push_back(symbol);
            }
        }
        // ------------
        loop_list = price_data_update_failure;
        timestamp_now = (std::chrono::system_clock::now().time_since_epoch()).count() * std::chrono::system_clock::period::num / std::chrono::system_clock::period::den;
        if (loop_list.empty() || timestamp_now-start > 10) { break; }                
        else {
            price_data_update_failure = {};
            sleep(1.5);
        }
    }   
 
    // Confirm Latest Data Provided
    // --------------------------------
    std::vector<long unsigned int> symbol_update_datetimes = {};
    for(std::map<std::string,long unsigned int>::iterator it = price_data_update_datetime.begin(); it != price_data_update_datetime.end(); ++it) {
        symbol_update_datetimes.push_back(it->second);
    }
    last_bar_timestamp = (!symbol_update_datetimes.empty()) ? *max_element(symbol_update_datetimes.begin(), symbol_update_datetimes.end()) : last_bar_timestamp;

    for(std::map<std::string,long unsigned int>::iterator it = price_data_update_datetime.begin(); it != price_data_update_datetime.end(); ++it) {
        if (it->second != last_bar_timestamp) {
            price_data_update_failure.push_back(it->first);
            try {
                completed_list.erase(remove(completed_list.begin(),completed_list.end(),it->first), completed_list.end());
            }
            catch (...) { }
        }
    }

    for (std::string symbol : price_data_update_failure) {
        BOOST_LOG_TRIVIAL(error) << "FX Order Management - Price Data Update Failure " << symbol << "; Will Retry After Trades Placed";
    }
    // ------------------------------------
    // Seperate Data From OHLC into Individual Vectors
    execute_list = {};

    for (std::string symbol : completed_list) {

        nlohmann::json ohlc_data = data[symbol]; 
        int data_length = ohlc_data.size(); 
        
        if (data_length >= num_data_points) {

            int x1;
            execute_list.push_back(symbol);
            std::vector<float> open_data = {};
            std::vector<float> high_data = {};
            std::vector<float> low_data = {};
            std::vector<float> close_data = {};
            std::vector<float> datetime_data = {};
            
            // ------------
            for (int x = 0; x < num_data_points; x++) {
                x1 = data_length - num_data_points + x;
                std::string bar_datetime = ohlc_data[x1]["BarDate"];

                open_data.push_back(ohlc_data[x1]["Open"]); 
                high_data.push_back(ohlc_data[x1]["High"]); 
                low_data.push_back(ohlc_data[x1]["Low"]); 
                close_data.push_back(ohlc_data[x1]["Close"]); 
                datetime_data.push_back(std::stof(bar_datetime.substr(6,10)));
            }
            historical_data_map[symbol] = {{"Open",open_data},{"High",high_data},{"Low",low_data},{"Close",close_data},{"Datetime",datetime_data}};
        }
        else {
            data_error_list.push_back(symbol);
            BOOST_LOG_TRIVIAL(warning) << "Data Aquisition Error; Length Too Short: " << symbol << " - " << data_length;
        }
    }
}

void FXOrderManagement::pause_next_bar() {

    next_bar_timestamp = last_bar_timestamp + update_frequency_seconds;
    long unsigned int time_next_bar_will_be_ready = next_bar_timestamp + update_frequency_seconds;
    unsigned int timestamp_now;
    // -----------------------------------
    // Execute Trading Model & Confirm All Data Aquired Properly

    for (int x = 0; x < 5; x++) {
        timestamp_now = (std::chrono::system_clock::now().time_since_epoch()).count() * std::chrono::system_clock::period::num / std::chrono::system_clock::period::den;
   
        if (!price_data_update_failure.empty()) { 
            if (timestamp_now <= time_next_bar_will_be_ready - 15) {

                return_price_history(price_data_update_failure);

                if (!execute_list.empty()) {
                    get_trading_model_signal(); 
                    read_input_information();
                    build_trades_map();
                }
            } else { break; }
        }
        else { 
            price_data_update_failure_count = {}; break;
        }
    }

    // ------------
    for (std::string symbol : price_data_update_failure) {
        try {
            price_data_update_failure_count[symbol] += 1;
        }
        catch (...) { price_data_update_failure_count[symbol] = 1; }

        BOOST_LOG_TRIVIAL(warning) << "Updates Failed " << symbol << "; Number of Failed Loops: " << price_data_update_failure_count[symbol] << std::endl;
    }
    // -----------------------------------
    // Notify Data Error
    if (!data_error_list.empty()) {
        for (std::string symbol : data_error_list) {
            try {
                live_symbols_list.erase(remove(live_symbols_list.begin(),live_symbols_list.end(),symbol), live_symbols_list.end());
                price_data_update_datetime.erase(symbol);

                BOOST_LOG_TRIVIAL(error) << "Data Aquisition Error: " << symbol << std::endl;
                position_multiplier[symbol] = 0;
                std::cout << "Data Aquisition Error: " << symbol << std::endl;
            }
            catch (...) { }
        }
        data_error_list = {};
    }
    // -----------------------------------
    timestamp_now = (std::chrono::system_clock::now().time_since_epoch()).count() * std::chrono::system_clock::period::num / std::chrono::system_clock::period::den;
    float time_to_wait_now = time_next_bar_will_be_ready - timestamp_now;
    if (time_to_wait_now > 0) {
        BOOST_LOG_TRIVIAL(info) << "FX Order Management - Waiting For OHLC Bar Update";
        sleep(time_to_wait_now);
    }
    // Get New Session if Expired
    session.validate_session();
    // ------------
    return_price_history(live_symbols_list);
}

void FXOrderManagement::execute_signals(nlohmann::json trade_dict) {

    if (place_trades || emergency_close == 1) { 

        std::vector<std::string> error_list = session.trade_market_order(trade_dict);
        execution_loop_count += 1;

        if (!error_list.empty()) {
            for (std::string error_item : error_list) {
                BOOST_LOG_TRIVIAL(warning) << "Trading Error List: " << error_item << std::endl;
            }
        }
        // ------------------------------------
        // Check On Active Orders
        for (int x = 0; x < 3; x++) {
            int pending_order_count = 0;

            sleep(1); // Pause For Each Loop
            nlohmann::json active_orders = session.list_active_orders();
            
            for (auto item : active_orders) {

                int status = item["TradeOrder"]["StatusId"];

                if (status == 1) {
                    pending_order_count += 1;
                    if (x == 2) {
                        nlohmann::json resp = session.cancel_order(item["TradeOrder"]["OrderId"]);
                        BOOST_LOG_TRIVIAL(warning) << "Cancelled Order: " << resp << std::endl;
                    }
                }
                // ------------
                std::vector<int> major_status_errors = {6, 8, 10};
                if (std::find(major_status_errors.begin(), major_status_errors.end(), status) != major_status_errors.end()) {
                    // Count Doesn't Increase Because Order Will Never Finish
                    BOOST_LOG_TRIVIAL(error) << "Major Order Status Error: " << status << std::endl;
                }
            }
            // ------------
            if (pending_order_count == 0) { break; }
        }
        // ---------------------------
        if (execution_loop_count < 4) {
            // Recursive Trade Confirmation Can Only Happen A Maximum of (3) Times
            verify_trades_opened(trade_dict);
        } else {
            // ------------
            for (std::string symbol : error_list) {
                BOOST_LOG_TRIVIAL(warning) << "Trading Error, Symbol Removed: " << symbol << std::endl;
                position_multiplier[symbol] = 0;
            }
        }
    }
}

void FXOrderManagement::verify_trades_opened(nlohmann::json trade_dict) {
    nlohmann::json trades_map = {};
    std::vector<std::string> keys_list = {}; 

    for(nlohmann::json::iterator it = trade_dict.begin(); it != trade_dict.end(); ++it) {
        keys_list.push_back(it.key());
    }

    // ------------------------------------
    // Check Account Positions
    open_positions = session.list_open_positions();

    for (auto position : open_positions) {
        std::string symbol = position["MarketName"];
        float existing_quantity = position["Quantity"];
        std::string existing_direction = position["Direction"];
        // ------------
        if (find(keys_list.begin(),keys_list.end(),symbol) != keys_list.end()) {
            // ------------
            keys_list.erase(remove(keys_list.begin(),keys_list.end(),symbol), keys_list.end());
            std::string direction = trade_dict[symbol]["Direction"];
            int final_quantity = trade_dict[symbol]["Final Quantity"];
            // ------------
            if (direction != existing_direction) {
                trades_map[symbol] = {
                    {"Quantity",existing_quantity+final_quantity},
                    {"Direction",direction},
                    {"Final Quantity",final_quantity}
                    };
            }
            else if (direction == existing_direction && existing_quantity < final_quantity) {
                trades_map[symbol] = {
                    {"Quantity",final_quantity-existing_quantity},
                    {"Direction",direction},
                    {"Final Quantity",final_quantity}
                    };
            }
        }
    }
    // ---------------------------
    // Confirm New Positions Are Open
    for (std::string symbol : keys_list) { 
        if (trade_dict[symbol]["Final Quantity"] != 0) { 
            trades_map[symbol] = trade_dict[symbol];
        }
    }
    // ---------------------------
    // Execute Trades
    if (!trades_map.empty()) {
        execute_signals(trades_map);
    }
}

// --------- [FX File IO Code] ---------
void FXOrderManagement::read_input_information() {

    std::string date = fx_utilities.get_todays_date();
    std::string file_name = sys_path + "/" + date + "_FX_Order_Deletion.json";
    nlohmann::json data;
    std::vector<std::string> double_check = live_symbols_list;

    try {
        std::ifstream f(file_name);
        data = nlohmann::json::parse(f);
    }
    catch ( ... ) { 
        for (std::string symbol : live_symbols_list) {
            data[symbol] = {{"Close Immediately", false}, {"Close On Trade Signal Change", false}};
        }
    }

    for(nlohmann::json::iterator it = data.begin(); it != data.end(); ++it) {
        nlohmann::json json_value = it.value();
        if (json_value.contains("Close Immediately")) {
            if (json_value["Close Immediately"] == true) {
                main_signals[it.key()] = 0;
            }
        }
        if (json_value.contains("Close On Trade Signal Change")) {
            if (json_value["Close On Trade Signal Change"] == true) {
                position_multiplier[it.key()] = 0;
            }
        }
        double_check.erase(remove(double_check.begin(),double_check.end(),it.key()), double_check.end());
    }

    for (std::string symbol : double_check) {
        data[symbol] = {{"Close Immediately", false}, {"Close On Trade Signal Change", false}};
    }

    std::string output_string = data.dump(4);
    
    std::ofstream out(file_name);
    out << output_string;
    out.close();

}

void FXOrderManagement::output_order_information() {
    nlohmann::json current_performance = {};
    nlohmann::json current_positions = {};
    time_t time_now = time(NULL);
    std::string time_str = ctime(&time_now);
    float profit;
    float profit_percent;

    for (auto position : open_positions) {
        std::string market_name = position["MarketName"];
        int direction = (position["Direction"] == "buy") ? 1 : -1;
        float entry_price = position["Price"];
        int quantity = position["Quantity"];

        float current_price = session.get_prices({market_name})[market_name][0]["Price"];

        profit = round((current_price - entry_price) * 100000) / 100000 * direction;

        profit_percent = (entry_price != 0) ? round(profit * 10000 / entry_price) / 100 : 0;

        current_positions[market_name] = {
            {"Direction", position["Direction"]}, 
            {"Quantity", position["Quantity"]}, 
            {"Entry Price", position["Price"]}, 
            {"Current Price", current_price},
            {"Profit", profit},
            {"Profit Percent", profit_percent}
            };
    } 
    // ---------------------------  
    if (init_equity == 0) { init_equity = equity_total; }
    float total_profit = round((equity_total - init_equity) * 100) / 100;
    float total_profit_percent = (init_equity != 0) ? round(profit * 10000 / init_equity) / 100 : 0;
   
    current_performance["Performance Information"] = {
        {"Inital Funds", init_equity},
        {"Current Funds", equity_total},
        {"Margin Utilized", margin_total},
        {"Profit Cumulative", total_profit},
        {"Profit Percent Cumulative", total_profit_percent}
    };

    current_performance["Position Information"] = current_positions;
    current_performance["Last Updated"] = time_str;

    std::string date = fx_utilities.get_todays_date();

    std::string file_name = sys_path + "/" + date + "_FX_Order_Information.json";

    std::string output_string = current_performance.dump(4);
    
    std::ofstream out(file_name);
    out << output_string;
    out.close();
}

}
