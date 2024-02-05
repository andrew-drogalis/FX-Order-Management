#ifndef FX_ORDER_MANAGEMENT_H
#define FX_ORDER_MANAGEMENT_H

#include <string>
#include <vector>
#include <map>

#include <gain_capital_api.h>
#include <json.hpp>

#include <trading_model.h>
#include <fx_utilities.h>
#include <fx_market_time.h>

namespace fxordermgmt {

class FXOrderManagement {

    public:

        FXOrderManagement();

        ~FXOrderManagement();

        FXOrderManagement(std::string account, bool place_trades, int clear_system, std::string sys_path);

        void run_order_management_system();

    private:

        // Passed Through Constructor
        bool place_trades;
        int emergency_close;
        std::string sys_path;

        // Gain Capital Parameters
        std::string trading_account;
        std::string service_id;
        std::string package;
        GCapiClient session;

        // For Trading Indicator
        std::map<std::string, int> main_signals;
        std::map<std::string, TradingModel> trading_model_map;
        std::map<std::string, std::map<std::string,std::vector<float>>> historical_data_map;

        // Build Trades Map
        std::map<std::string, std::map<std::string, std::string>> trades_map;
        std::map<std::string, int> position_multiplier;
        std::vector<std::string> execute_list;
        nlohmann::json open_positions;

        // Getting Price History
        long unsigned int last_bar_timestamp;
        long unsigned int next_bar_timestamp;
        std::vector<std::string> price_data_update_failure;
        std::map<std::string, int> price_data_update_failure_count;
        std::map<std::string, long unsigned int> price_data_update_datetime;
        std::vector<std::string> data_error_list;
        std::vector<std::string> live_symbols_list;

        // Placing Trades
        int execution_loop_count;

        // Output Order Information
        float margin_total;
        float equity_total;
        float init_equity;
        
        // General Use
        int update_frequency_seconds;
        FXUtilities fx_utilities = FXUtilities();
        FXMarketTime fx_market_time;

        void gain_capital_session();

        void initalize_trading_model(std::string symbol);

        void get_trading_model_signal();

        void build_trades_map();

        void emergency_position_close();

        void return_tick_history(std::vector<std::string> symbols_list);

        void return_price_history(std::vector<std::string> symbols_list);        

        void pause_next_bar();

        void execute_signals(nlohmann::json trade_dict);

        void verify_trades_opened(nlohmann::json trade_dict);

        void read_input_information();

        void output_order_information();

};

}

#endif