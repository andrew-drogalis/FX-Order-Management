#ifndef FX_ORDER_MANAGEMENT_H
#define FX_ORDER_MANAGEMENT_H

#include <trading_model.h>
#include <gain_capital_api.h>
#include <json.hpp>
#include <string>
#include <vector>
#include <map>

using namespace std;

class FXOrderManagement {

    public:

        FXOrderManagement();

        FXOrderManagement(string account, bool place_trades, int clear_system, string sys_path);

        void run_order_management_system();

    private:

        // Passed Through Constructor
        bool place_trades;
        int emergency_close;
        string sys_path;

        // Gain Capital Parameters
        string trading_account;
        string service_id;
        string package;
        unsigned int refresh_session_time;
        GCapiClient session;

        // For Trading Indicator
        map<string, int> main_signals;
        map<string, TradingModel> trading_model_map;
        map<string, map<string,vector<float>>> historical_data_map;

        // Build Trades Map
        map<string, map<string, string>> trades_map;
        map<string, int> position_multiplier;
        vector<string> execute_list;
        nlohmann::json open_positions;

        // Getting Price History
        long unsigned int last_bar_timestamp;
        long unsigned int next_bar_timestamp;
        vector<string> price_data_update_failure;
        map<string, int> price_data_update_failure_count;
        map<string, long unsigned int> price_data_update_datetime;
        vector<string> data_error_list;
        vector<string> live_symbols_list;
        // Placing Trades
        int execution_loop_count;

        // Output Order Information
        float margin_total;
        float equity_total;
        float init_equity;
        
        // Forex Time 
        long unsigned int FX_market_start;
        long unsigned int FX_market_end;
        long unsigned int market_close_time;

        // General Use
        bool will_market_be_open_today;
        int update_frequency_seconds;


        void gain_capital_session();

        void output_order_information();

        void initalize_trading_model(string symbol);

        void get_trading_model_signal();

        void forex_market_time_setup();

        bool market_closed();

        bool forex_market_exit_only();

        void build_trades_map();

        void emergency_position_close();

        void return_price_history(vector<string> symbols_list);

        void return_tick_history(vector<string> symbols_list);

        void pause_next_bar();

        void execute_signals(nlohmann::json trade_dict);

        void verify_trades_opened(nlohmann::json trade_dict);

        void setup_password_first_time(string account_type);
};

#endif