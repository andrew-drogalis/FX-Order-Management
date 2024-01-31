
#include <fx_order_management.h>
#include <trading_model.h>
#include <credentials.h>
#include <order_parameters.h>
#include <gain_capital_api.h>
#include <json.hpp>
#include <keychain.h>
#include <boost/log/trivial.hpp>
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

    setup_password_first_time(account);

    // Confirm Order Parameters are Valid
    transform(update_interval.begin(), update_interval.end(), update_interval.begin(), ::toupper);

    vector<int> SPAN_M = {1, 2, 3, 5, 10, 15, 30}; // Span intervals for minutes
    vector<int> SPAN_H = {1, 2, 4, 8}; // Span intervals for hours
    vector<string> INTERVAL = {"HOUR", "MINUTE", "DAY", "WEEK", "MONTH"};


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
        } else { will_market_be_open_today = false; }   
    }  

    if (!will_market_be_open_today) {
        std::cout << "Market is Closed Today: Terminating Program" << std::endl;
        std::terminate();
    }
    // -------------------------------
    // Maps
    trading_model_map = {};
    historical_data_map = {};
    price_data_update_datetime = {};
    price_data_update_failure_count = {};
    position_multiplier = {};
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
}

void FXOrderManagement::run_order_management_system() {
    
    if (emergency_close == 0) {

        time_t time_now = time(NULL);
        cout << "Order Management Currently Running " << ctime(&time_now) << endl;

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

    session.get_account_info();
    session.get_market_ids(live_symbols_list);

    // Save Time for Token Expiration
    unsigned int time_now = (std::chrono::system_clock::now().time_since_epoch()).count();

    int offset = 20 * 60; // Minutes -> Seconds

    refresh_session_time = time_now + offset;
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
        int direction = (position["Direction"] == "Buy") ? 1 : -1;
        float entry_price = position["Price"];
        float current_price = session.get_prices({market_name})[market_name][0]["Prices"];
        profit = (current_price - entry_price) * direction;
        profit_percent = (entry_price != 0) ? profit * 100 / entry_price : 0;

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
    float total_profit = equity_total - init_equity;
    float total_profit_percent = (init_equity != 0) ? profit * 100 / init_equity : 0;
   
    current_performance["Performance Information"] = {
        {"Inital Funds", init_equity},
        {"Current Funds", equity_total},
        {"Margin Utilized", margin_total},
        {"Profit Cumulative", total_profit},
        {"Profit Percent Cumulative", total_profit_percent}
    };

    current_performance["Position Information"] = current_positions;
    current_performance["Last Updated"] = time_str;

    time_t t;
    struct tm *tmp;
    char DATE_TODAY[50];
    time( &t );
    tmp = localtime( &t );
    strftime(DATE_TODAY, sizeof(DATE_TODAY), "%Y_%m_%d", tmp);

    string file_name = sys_path + "/" + DATE_TODAY + "_FX_Order_Management.json";

    string output_string = current_performance.dump(4);
    
    std::ofstream out(file_name);
    out << output_string;
    out.close();
}
    
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

        BOOST_LOG_TRIVIAL(debug) << "Main Signals - " << symbol << "; Prediction: " << prediction;
    }
}

void FXOrderManagement::forex_market_time_setup() {
    // 24 HR Start Trading Time & End Trading Time
    // In London Time 8 = 8:00 AM London Time
    int start_hr = 8;
    int end_hr = 23;
    // ----------------------------------------
    time_t now = time(0);
    struct tm *now_local;
    now_local = localtime(&now);
    char DATE_TODAY[50];
    strftime(DATE_TODAY, sizeof(DATE_TODAY), "%Y-%m-%d", now_local);
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
    std::vector<std::string> holidays = {"2024-01-01","2024-01-15","2024-02-19","2024-03-29","2024-05-27","2023-06-19","2023-07-04","2023-09-04","2023-11-23","2023-11-24","2023-12-24","2023-12-25"};
    std::vector<int> days_of_week_to_trade = {6, 0, 1, 2, 3, 4};

    // Regular Market Hours
    if (std::find(days_of_week_to_trade.begin(), days_of_week_to_trade.end(), day_of_week) != days_of_week_to_trade.end() && std::find(holidays.begin(), holidays.end(), DATE_TODAY) == holidays.end()) { 
        will_market_be_open_today = true;
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
        will_market_be_open_today = false; 
        FX_market_start = 0;
        FX_market_end = (std::chrono::system_clock::now().time_since_epoch()).count() * chrono::system_clock::period::num / chrono::system_clock::period::den - 1;
        market_close_time = (std::chrono::system_clock::now().time_since_epoch()).count() * chrono::system_clock::period::num / chrono::system_clock::period::den - 1;
    }
    cout << will_market_be_open_today << endl << FX_market_start << endl << FX_market_end << endl << market_close_time << endl;
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
    nlohmann::json open_positions = session.list_open_positions();
    bool forex_is_exit_only = forex_market_exit_only();

    if (!forex_is_exit_only) {
        // ------------
        for (auto position : open_positions) {
            string symbol = position["MarketName"];
            // ------------
            if (find(symbols_in_main_signals.begin(),symbols_in_main_signals.end(),symbol) != symbols_in_main_signals.end()) {

                remove(symbols_in_main_signals.begin(),symbols_in_main_signals.end(),symbol);

                float base_quantity;
                try {
                    base_quantity = round(position_multiplier[symbol]*order_position_size / 1000) * 1000;
                }
                catch (...) { base_quantity = order_position_size; }

                float live_quantity = position["Quantity"];
                string direction = position["Direction"];
                string new_direction = (direction == "buy") ? "sell" : "buy";
                // ---------------------------
                if (direction == "buy") {
                    // ------------
                    if (main_signals[symbol] == 1) {
                        // ------------
                        if (live_quantity < base_quantity) {
                            trades_map[symbol] = {
                                {"Quantity", std::to_string(base_quantity-live_quantity)},
                                {"Direction", direction},
                                {"Final Quantity", std::to_string(base_quantity)}
                                };
                        }
                    }
                    else if (main_signals[symbol] == 0) { 
                        trades_map[symbol] = {
                            {"Quantity", std::to_string(live_quantity)},
                            {"Direction", new_direction},
                            {"Final Quantity", "0"}
                            };
                    }
                    else {
                        trades_map[symbol] = {
                            {"Quantity", std::to_string(base_quantity+live_quantity)},
                            {"Direction", new_direction},
                            {"Final Quantity", std::to_string(base_quantity)}
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
                                {"Quantity", std::to_string(base_quantity-live_quantity)},
                                {"Direction", direction},
                                {"Final Quantity", std::to_string(base_quantity)}
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
                            {"Quantity", std::to_string(base_quantity+live_quantity)},
                            {"Direction", new_direction},
                            {"Final Quantity", std::to_string(base_quantity)}
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
                float base_quantity;
                try {
                    base_quantity = round(position_multiplier[symbol]*order_position_size / 1000) * 1000;
                }
                catch (...) { base_quantity = order_position_size; }

                if (position_signal != 0 && base_quantity != 0) {
                    string direction = (position_signal == 1) ? "buy" : "sell";

                    trades_map[symbol] = {
                            {"Quantity", std::to_string(base_quantity)},
                            {"Direction", direction},
                            {"Final Quantity", std::to_string(base_quantity)}
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
            float quantity = position["Quantity"];
            string new_direction = (position["Direction"] == "buy") ? "sell" : "buy";
      
            trades_map[symbol] = {
                {"Quantity", quantity},
                {"Direction", new_direction},
                {"Final Quantity", "0"}
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
        float quantity = position["Quantity"];
        string direction = position["Direction"];
        string new_direction = (direction == "buy") ? "sell" : "buy";

        trades_map[symbol] = {
            {"Quantity", quantity},
            {"Direction", new_direction},
            {"Final Quantity", "0"}
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
    long unsigned int start = (std::chrono::system_clock::now().time_since_epoch()).count() * chrono::system_clock::period::num / chrono::system_clock::period::den;
    long unsigned int timestamp_now;
    std::map<string, nlohmann::json> data;
    price_data_update_datetime = {};
    price_data_update_failure = {};
    vector<string> loop_list = symbols_list;
    vector<string> completed_list = {};
    nlohmann::json symbol_data;
    nlohmann::json last_bar;
    
    while (true) {
        data = session.get_ohlc(loop_list, update_interval, num_data_points, update_span);
        // ---------------------------
        for (string symbol : loop_list) { 
            try {
                symbol_data = data[symbol];
                last_bar = symbol_data[symbol_data.size()-1];
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
    last_bar_timestamp = *max_element(symbol_update_datetimes.begin(), symbol_update_datetimes.end());

    for(std::map<string,long unsigned int>::iterator it = price_data_update_datetime.begin(); it != price_data_update_datetime.end(); ++it) {
        if (it->second != last_bar_timestamp) {
            price_data_update_failure.push_back(it->first);
            try {
                remove(completed_list.begin(),completed_list.end(),it->first);
            }
            catch (...) { }
        }
    }
    // ------------------------------------
    // Seperate Data From OHLC into Individual Vectors
    execute_list = {};

    cout << data_error_list.size() << endl;
    for (string symbol : completed_list) {

        nlohmann::json ohlc_data = data[symbol]; 
        int data_length = ohlc_data.size(); 
        
        if (data_length >= num_data_points) {

            vector<float> open_data = {};
            vector<float> high_data = {};
            vector<float> low_data = {};
            vector<float> close_data = {};
            vector<float> datetime_data = {};
            execute_list.push_back(symbol);
    
            int x1;
            // ------------
            for (int x = 0; x < num_data_points; x++) {
                x1 = data_length - num_data_points + x;
                string bar_datetime = ohlc_data[x1]["BarDate"];

                open_data.push_back(ohlc_data[x1]["Open"]); 
                high_data.push_back(ohlc_data[x1]["High"]); 
                low_data.push_back(ohlc_data[x1]["Low"]); 
                close_data.push_back(ohlc_data[x1]["Close"]); 
                datetime_data.push_back(std::stof(bar_datetime.substr(6,10)));
          
                historical_data_map[symbol] = {{"Open",open_data},{"High",high_data},{"Low",low_data},{"Close",close_data},{"Datetime",datetime_data}};
            }
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
    // Execute Trading Model & Confirm All Data Proper Aquired
    for (int x = 0; x < 5; x++) {
        timestamp_now = (std::chrono::system_clock::now().time_since_epoch()).count() * chrono::system_clock::period::num / chrono::system_clock::period::den;
   
        if (!price_data_update_failure.empty()) { 
            if (timestamp_now <= time_next_bar_will_be_ready - 15) {
                return_price_history(price_data_update_failure);
                if (!execute_list.empty()) {
                    get_trading_model_signal(); 
                    build_trades_map();
                }
            } else{ break; }
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

        BOOST_LOG_TRIVIAL(warning) << "Updates Failed " << symbol << "; Loop: " << price_data_update_failure_count[symbol] << std::endl;
    }
    // -----------------------------------
    // Notify Data Error
    if (!data_error_list.empty()) {
        for (std::string symbol : data_error_list) {
            try {
                cout << symbol << endl << data_error_list.size() << endl;
                remove(live_symbols_list.begin(),live_symbols_list.end(),symbol);
                price_data_update_datetime.erase(symbol);

                BOOST_LOG_TRIVIAL(error) << "DATA AQUISITION ERROR: " << symbol << std::endl;
                position_multiplier[symbol] = 0;
                std::cout << "DATA AQUISITION ERROR: " << symbol << std::endl;
            }
            catch (...) { }
        }
        data_error_list = {};
    }
    // -----------------------------------
    // Access Token | Refresh - (Update Interval + 1) Mins Before Expiration
    timestamp_now = (std::chrono::system_clock::now().time_since_epoch()).count() * chrono::system_clock::period::num / chrono::system_clock::period::den;
    if (refresh_session_time - timestamp_now <= update_frequency_seconds + 60) {
        gain_capital_session();
    }
    // -----------------------------------
    timestamp_now = (std::chrono::system_clock::now().time_since_epoch()).count() * chrono::system_clock::period::num / chrono::system_clock::period::den;
    float time_to_wait_now = time_next_bar_will_be_ready - timestamp_now;
    if (time_to_wait_now > 0) {
        sleep(time_to_wait_now);
    }
    // ------------
    return_price_history(live_symbols_list);
}

void FXOrderManagement::execute_signals(nlohmann::json trade_dict) {

    if (place_trades || emergency_close == 1) { 

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
    vector<string> keys_list; 

    for(nlohmann::json::iterator it = trade_dict.begin(); it != trade_dict.end(); ++it) {
        string key = it.key();
        keys_list.push_back(key);
    }
    // ------------------------------------
    // Check Account Positions
    nlohmann::json open_positions = session.list_open_positions();

    for (auto position : open_positions) {
        string symbol = position["MarketName"];
        float existing_quantity = position["Quantity"];
        string existing_direction = position["Direction"];
        // ------------
        if (find(keys_list.begin(),keys_list.end(),symbol) != keys_list.end()) {
            // ------------
            remove(keys_list.begin(),keys_list.end(),symbol);
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