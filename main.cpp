
#include <fx_order_management.h>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <iostream>
#include <filesystem>
#include <string>
#include <ctime>
#include <exception>

using namespace std;

string get_todays_date() {
    time_t t;
    struct tm *tmp;
    char DATE_TODAY[50];
    time( &t );
    tmp = localtime( &t );
     
    strftime(DATE_TODAY, sizeof(DATE_TODAY), "%Y_%m_%d", tmp);

    return DATE_TODAY;
}

void init_logging(string working_directory) {
    string date = get_todays_date();
    string file_name = working_directory + "/" + date + "_FX_Order_Management.log";

    boost::log::add_file_log(        
        boost::log::keywords::file_name = file_name,                                                                   
        boost::log::keywords::format = "[%TimeStamp%]: %Message%"
    );

    boost::log::core::get()->set_filter
    (
        boost::log::trivial::severity >= boost::log::trivial::debug
    );

    boost::log::add_common_attributes();
}

int main() {
    // Set Account Type
    string ACCOUNT = "PAPER"; 
    bool PLACE_TRADES = false; 
    int EMERGENCY_CLOSE;

    if (ACCOUNT != "PAPER" && ACCOUNT != "LIVE") {
        std::cerr << "Wrong Account Type; Please Select 'PAPER' or 'LIVE'" << std::endl;
        std::terminate();
    }
    else {
        cout << "Account Type Initialized: " << ACCOUNT << "\n" << endl;
    }

    // Check if Emergency Scenario
    while (true) {
        cout << "Close All Open Positions?\n\nPlease Enter: 1 = True; 0 = False;\n\nYour Selection: ";
        cin >> EMERGENCY_CLOSE;
        cout << endl;

        if (EMERGENCY_CLOSE == 1 || EMERGENCY_CLOSE == 0) { break;}
        else {
            cout << "Wrong Input\n";
        }
    }

    // Set Logging Parameters
    string working_directory = filesystem::current_path();
    init_logging(working_directory);

    // Initalize Order Management
    FXOrderManagement fx = FXOrderManagement(ACCOUNT, PLACE_TRADES, EMERGENCY_CLOSE, working_directory);

    if (EMERGENCY_CLOSE == 0) {
        fx.run_order_management_system();
    }

    time_t end_time = time(NULL);
    cout << "FX Order Management - Program Terminated Successfully: " << ctime(&end_time) << endl;

    return 0;
}