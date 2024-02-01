
#include <fx_order_management.h>
#include <trading_model.h>
#include <credentials.h>
#include <order_parameters.h>
#include <gain_capital_api.h>
#include <json.hpp>
#include <keychain.h>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <unistd.h> 
#include <chrono>
#include <ctime>
#include <map>
#include <exception>
#include <format>
#include <valarray>

using namespace std;

FXOrderManagement::FXOrderManagement() { }

FXOrderManagement::FXOrderManagement(string account, bool place_tr, int emerg_close, string working_directory) {

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

    place_trades = place_tr;
    emergency_close = emerg_close;
    sys_path = working_directory;

    init_logging(working_directory);
    setup_password_first_time(account);

    // Confirm Order Parameters are Valid
    transform(update_interval.begin(), update_interval.end(), update_interval.begin(), ::toupper);

    vector<int> SPAN_M = {1, 2, 3, 5, 10, 15, 30}; // Span intervals for minutes
    vector<int> SPAN_H = {1, 2, 4, 8}; // Span intervals for hours
    vector<string> INTERVAL = {"HOUR", "MINUTE"};


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
    forex_market_time_setup();
    bool market_is_closed = market_closed();
    bool forex_is_exit_only = forex_market_exit_only();

    if (!market_is_closed && forex_is_exit_only) {
        
        float time_to_wait_now = FX_market_start - (std::chrono::system_clock::now().time_since_epoch()).count() * chrono::system_clock::period::num / chrono::system_clock::period::den;

        std::cout << "Market Closed Today; Waiting for Tomorrow; Will be waiting for " << round(time_to_wait_now / 36) / 100 << " Hours..." << std::endl;

        // Pause Until Market Open
        if (time_to_wait_now > 0) { 
            sleep(time_to_wait_now);
        } else { will_market_be_open_tomorrow = false; }   
    }  

    if (!will_market_be_open_tomorrow) {
        std::cout << "Market is Closed Today; Weekend Approaching; Terminating Program" << std::endl;
        std::terminate();
    }
    // -------------------------------
    // Maps
    trading_model_map = {};
    historical_data_map = {};
    price_data_update_datetime = {};
    price_data_update_failure_count = {};
    position_multiplier = {};
    for (string symbol : fx_symbols_to_trade) { position_multiplier[symbol] = 1; }
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
        cout << "FX Order Management - Currently Running " << ctime(&time_now) << endl;
        BOOST_LOG_TRIVIAL(info) << "FX Order Management - Currently Running";
        cout << "For Live Updates, See Log File" << endl;

        // Initialize Price History & Trading Model
        return_price_history(live_symbols_list);

        for (std::string symbol : execute_list) { 
            initalize_trading_model(symbol); 
        }

        // Start Loop
        while (true) {
            bool market_is_closed = market_closed();
            if (market_is_closed) {
                cout << "FX Order Management - Market Closed" << endl;
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
    
void FXOrderManagement::gain_capital_session() {

    keychain::Error error{};
    string password = keychain::getPassword(package, service_id, trading_account, error);
    
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

// --------- [File I/O Code] ---------
void FXOrderManagement::read_input_information() {

    string date = get_todays_date();
    string file_name = sys_path + "/" + date + "_FX_Order_Deletion.json";
    nlohmann::json data;
    vector<string> double_check = live_symbols_list;

    for (string symbol : live_symbols_list ) { cout << "3" << symbol << endl; }

    try {
        std::ifstream f(file_name);
        data = nlohmann::json::parse(f);
    }
    catch ( ... ) { 
        for (string symbol : live_symbols_list) {
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

    for (string symbol : double_check) {
        data[symbol] = {{"Close Immediately", false}, {"Close On Trade Signal Change", false}};
    }

    for (string symbol : live_symbols_list ) { cout << "4" << symbol << endl; }

    string output_string = data.dump(4);
    
    std::ofstream out(file_name);
    out << output_string;
    out.close();

}

void FXOrderManagement::output_order_information() {
    nlohmann::json current_performance = {};
    nlohmann::json current_positions = {};
    time_t time_now = time(NULL);
    string time_str = ctime(&time_now);
    float profit;
    float profit_percent;

    for (auto position : open_positions) {
        string market_name = position["MarketName"];
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

    string date = get_todays_date();

    string file_name = sys_path + "/" + date + "_FX_Order_Information.json";

    string output_string = current_performance.dump(4);
    
    std::ofstream out(file_name);
    out << output_string;
    out.close();
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

// --------- [FX Time Management Code] ---------
void FXOrderManagement::forex_market_time_setup() {
    // 24 HR Start Trading Time & End Trading Time
    // In London Time 8 = 8:00 AM London Time
    int start_hr = 8;
    int end_hr = 20;
    // ----------------------------------------
    time_t now = time(0);
    struct tm *now_local;
    now_local = localtime(&now);
    string date = get_todays_date();
    int day_of_week = now_local->tm_wday;

    std::chrono::zoned_time local_time = std::chrono::zoned_time{std::chrono::current_zone(), std::chrono::system_clock::now()};
    std::chrono::zoned_time london_time = std::chrono::zoned_time{"Europe/London", std::chrono::system_clock::now()};
    // --------
    int offset = (std::stoi(std::format("{:%z}", local_time)) - std::stoi(std::format("{:%z}", london_time))) / 100;
    int offset_seconds = -offset * 3600;
    start_hr += offset; 
    end_hr += offset;
    // ----------------------------------------
    // Must Be Updated as Required
    std::vector<std::string> holidays = {"2024_01_01","2024_01_15","2024_02_19","2024_03_29","2024_05_27","2024_06_19","2024_07_04","2024_09_02","2024_11_28","2024_11_29","2024_12_24","2024_12_25"};
    std::vector<int> days_of_week_to_trade = {6, 0, 1, 2, 3, 4};

    // Regular Market Hours
    if (std::find(days_of_week_to_trade.begin(), days_of_week_to_trade.end(), day_of_week) != days_of_week_to_trade.end() && std::find(holidays.begin(), holidays.end(), date) == holidays.end()) { 
        will_market_be_open_tomorrow = true;
        std::chrono::time_point time_now = std::chrono::system_clock::now();

        if ((std::stoi(std::format("{:%H}", local_time)) < end_hr || day_of_week == 4) && day_of_week != 6) {
            FX_market_start = ((std::chrono::floor<std::chrono::days>(time_now) + std::chrono::hours{start_hr}).time_since_epoch()).count() * 3600 + offset_seconds;
            FX_market_end = ((std::chrono::floor<std::chrono::days>(time_now) + std::chrono::hours{end_hr-1} + std::chrono::minutes{58}).time_since_epoch()).count() * 60 + offset_seconds;
            market_close_time = ((std::chrono::floor<std::chrono::days>(time_now) + std::chrono::hours{end_hr}).time_since_epoch()).count() * 3600 + offset_seconds;
        }
        else {
            time_now += std::chrono::days{1};
            FX_market_start = ((std::chrono::floor<std::chrono::days>(time_now) + std::chrono::hours{start_hr}).time_since_epoch()).count() * 3600 + offset_seconds;
            FX_market_end = ((std::chrono::floor<std::chrono::days>(time_now) + std::chrono::hours{end_hr-1} + std::chrono::minutes{58}).time_since_epoch()).count() * 60 + offset_seconds;
            market_close_time = ((std::chrono::floor<std::chrono::days>(time_now) + std::chrono::hours{end_hr}).time_since_epoch()).count() * 3600 + offset_seconds;
        }
    }
    // Market Closed
    else {
        will_market_be_open_tomorrow = false; 
        FX_market_start = 0;
        FX_market_end = (std::chrono::system_clock::now().time_since_epoch()).count() * chrono::system_clock::period::num / chrono::system_clock::period::den - 1;
        market_close_time = (std::chrono::system_clock::now().time_since_epoch()).count() * chrono::system_clock::period::num / chrono::system_clock::period::den - 1;
    }
}

bool FXOrderManagement::market_closed() {
    unsigned int time_now = (std::chrono::system_clock::now().time_since_epoch()).count() * chrono::system_clock::period::num / chrono::system_clock::period::den;
    if (market_close_time-1 < time_now && time_now < market_close_time+update_frequency_seconds+60) {
        return true;
    } else { return false; }    
}

bool FXOrderManagement::forex_market_exit_only() {
    unsigned int time_now = (std::chrono::system_clock::now().time_since_epoch()).count() * chrono::system_clock::period::num / chrono::system_clock::period::den;
    if (FX_market_start <= time_now && time_now <= FX_market_end) {
        return false;
    } else { return true; } 
}

// --------------------------------------------
void FXOrderManagement::build_trades_map() {

    nlohmann::json trades_map = {};

    std::vector<std::string> symbols_in_main_signals;
    for(std::map<string,int>::iterator it = main_signals.begin(); it != main_signals.end(); ++it) {
        symbols_in_main_signals.push_back(it->first);
    }

    // ------------------------------------
    // Check Account Positions
    open_positions = session.list_open_positions();
    bool forex_is_exit_only = forex_market_exit_only();

    if (!forex_is_exit_only) {
        // ------------
        for (auto position : open_positions) {
            string symbol = position["MarketName"];
            // ------------
            if (find(symbols_in_main_signals.begin(),symbols_in_main_signals.end(),symbol) != symbols_in_main_signals.end()) {

                symbols_in_main_signals.erase(remove(symbols_in_main_signals.begin(),symbols_in_main_signals.end(),symbol), symbols_in_main_signals.end());

                int base_quantity;
                try {
                    base_quantity = round(position_multiplier[symbol]*order_position_size / 1000) * 1000;
                }
                catch (...) { base_quantity = order_position_size; }

                int live_quantity = position["Quantity"];
                string direction = position["Direction"];
                string new_direction = (direction == "buy") ? "sell" : "buy";
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
                            {"Final Quantity", "0"}
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
                            {"Final Quantity", "0"}
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
                string symbol = symbols_in_main_signals[x];
                int position_signal = main_signals[symbol];
                int base_quantity;
                try {
                    base_quantity = round(position_multiplier[symbol]*order_position_size / 1000) * 1000;
                }
                catch (...) { base_quantity = order_position_size; }

                if (position_signal != 0 && base_quantity != 0) {
                    string direction = (position_signal == 1) ? "buy" : "sell";

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
            string symbol = position["MarketName"];
            int quantity = position["Quantity"];
            string new_direction = (position["Direction"] == "buy") ? "sell" : "buy";
      
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
        string symbol = position["MarketName"];
        int quantity = position["Quantity"];
        string direction = position["Direction"];
        string new_direction = (direction == "buy") ? "sell" : "buy";

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
    std::map<string, nlohmann::json> data = session.get_prices(symbols_list, num_data_points);
}

void FXOrderManagement::return_price_history(std::vector<std::string> symbols_list) {
    // Get Historical Forex Data
    BOOST_LOG_TRIVIAL(info) << "FX Order Management - Attempting to Fetch Price History";
    long unsigned int timestamp_now;
    long unsigned int start = (std::chrono::system_clock::now().time_since_epoch()).count() * chrono::system_clock::period::num / chrono::system_clock::period::den;
    price_data_update_datetime = {};
    price_data_update_failure = {};
    std::map<string, nlohmann::json> data;
    vector<string> loop_list = symbols_list;
    vector<string> completed_list = {};
 
    while (true) {
        data = session.get_ohlc(loop_list, update_interval, num_data_points, update_span);
        // ---------------------------
        for (string symbol : loop_list) { 
            try {
                nlohmann::json symbol_data = data[symbol];
                nlohmann::json last_bar = symbol_data[symbol_data.size()-1];
                string last_bar_datetime = last_bar["BarDate"];
  
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
        timestamp_now = (std::chrono::system_clock::now().time_since_epoch()).count() * chrono::system_clock::period::num / chrono::system_clock::period::den;
        if (loop_list.empty() || timestamp_now-start > 10) { break; }                
        else {
            price_data_update_failure = {};
            sleep(1.5);
        }
    }   
 
    // Confirm Latest Data Provided
    // --------------------------------
    vector<long unsigned int> symbol_update_datetimes = {};
    for(std::map<string,long unsigned int>::iterator it = price_data_update_datetime.begin(); it != price_data_update_datetime.end(); ++it) {
        symbol_update_datetimes.push_back(it->second);
    }
    last_bar_timestamp = (!symbol_update_datetimes.empty()) ? *max_element(symbol_update_datetimes.begin(), symbol_update_datetimes.end()) : last_bar_timestamp;

    for(std::map<string,long unsigned int>::iterator it = price_data_update_datetime.begin(); it != price_data_update_datetime.end(); ++it) {
        if (it->second != last_bar_timestamp) {
            price_data_update_failure.push_back(it->first);
            try {
                completed_list.erase(remove(completed_list.begin(),completed_list.end(),it->first), completed_list.end());
            }
            catch (...) { }
        }
    }

    for (string symbol : price_data_update_failure) {
        BOOST_LOG_TRIVIAL(error) << "FX Order Management - Price Data Update Failure " << symbol << "; Will Retry After Trades Placed";
    }
    // ------------------------------------
    // Seperate Data From OHLC into Individual Vectors
    execute_list = {};

    for (string symbol : completed_list) {

        nlohmann::json ohlc_data = data[symbol]; 
        int data_length = ohlc_data.size(); 
        
        if (data_length >= num_data_points) {

            int x1;
            execute_list.push_back(symbol);
            vector<float> open_data = {};
            vector<float> high_data = {};
            vector<float> low_data = {};
            vector<float> close_data = {};
            vector<float> datetime_data = {};
            
            // ------------
            for (int x = 0; x < num_data_points; x++) {
                x1 = data_length - num_data_points + x;
                string bar_datetime = ohlc_data[x1]["BarDate"];

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
        timestamp_now = (std::chrono::system_clock::now().time_since_epoch()).count() * chrono::system_clock::period::num / chrono::system_clock::period::den;
   
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
                std::cout << symbol << endl << data_error_list.size() << endl;
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
    timestamp_now = (std::chrono::system_clock::now().time_since_epoch()).count() * chrono::system_clock::period::num / chrono::system_clock::period::den;
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

        std::cout << trade_dict << endl;
        vector<string> error_list = session.trade_market_order(trade_dict);
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
    vector<string> keys_list = {}; 

    for(nlohmann::json::iterator it = trade_dict.begin(); it != trade_dict.end(); ++it) {
        keys_list.push_back(it.key());
    }

    // ------------------------------------
    // Check Account Positions
    open_positions = session.list_open_positions();

    for (auto position : open_positions) {
        string symbol = position["MarketName"];
        float existing_quantity = position["Quantity"];
        string existing_direction = position["Direction"];
        // ------------
        if (find(keys_list.begin(),keys_list.end(),symbol) != keys_list.end()) {
            // ------------
            keys_list.erase(remove(keys_list.begin(),keys_list.end(),symbol), keys_list.end());
            string direction = trade_dict[symbol]["Direction"];
            float final_quantity = trade_dict[symbol]["Final Quantity"];
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
 
// --------- [Utilities Code] ---------
void FXOrderManagement::setup_password_first_time(string account_type) {
    string test_account_password;
    string account_password;    
    string password;
    
    string service_id_test = "Test_Account";
    string service_id_live = "Live_Account";

    string package_test = "com.gain_capital_forex.test_account";
    string package_live = "com.gain_capital_forex.live_account";
  
    keychain::Error error = keychain::Error{};
    // Prompt Keyring Unlock
    keychain::setPassword("Forex_Keychain_Unlocker", "", "", "", error);
    if (error) {
        std::cerr << error.message << std::endl;
        std::terminate();
    }

    if (account_type == "PAPER") {
        error = keychain::Error{};
        password = keychain::getPassword(package_test, service_id_test, paper_account_username, error);
        
        if (error.type == keychain::ErrorType::NotFound) {
            std::cout << "Test Account password not found. Please input password: ";
            cin >> test_account_password;

            // Test Password Setup
            keychain::setPassword(package_test, service_id_test, paper_account_username, test_account_password, error);
            if (error) {
                std::cerr << "Test Account " << error.message << std::endl;
                std::terminate();
            }
        } else if (error) {
            std::cerr << error.message << std::endl;
            std::terminate();
        }
    }
    // -------------------------------
    if (account_type == "LIVE") {
        error = keychain::Error{};
        password = keychain::getPassword(package_live, service_id_live, account_username, error);
        
        if (error.type == keychain::ErrorType::NotFound) {
            std::cout << "Live Account password not found. Please input password: ";
            cin >> account_password;

            // Live Password Setup
            keychain::setPassword(package_live, service_id_live, account_username, account_password, error);
            if (error) {
                std::cerr << "Live Account " << error.message << std::endl;
                std::terminate();
            }
        } else if (error) {
            std::cerr << error.message << std::endl;
            std::terminate();
        }
    }
}

void FXOrderManagement::init_logging(string working_directory) {
    string date = get_todays_date();
    string file_name = working_directory + "/" + date + "_FX_Order_Management.log";

    boost::log::add_file_log(        
        boost::log::keywords::file_name = file_name,                                                                   
        boost::log::keywords::format = "[%TimeStamp%]: %Message%",
        boost::log::keywords::auto_flush = true
    );

    boost::log::core::get()->set_filter
    (
        boost::log::trivial::severity >= boost::log::trivial::debug
    );

    boost::log::add_common_attributes();
}

string FXOrderManagement::get_todays_date() {
    time_t t;
    struct tm *tmp;
    char DATE_TODAY[50];
    time( &t );
    tmp = localtime( &t );
     
    strftime(DATE_TODAY, sizeof(DATE_TODAY), "%Y_%m_%d", tmp);

    return DATE_TODAY;
}