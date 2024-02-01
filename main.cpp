
#include <fx_order_management.h>
#include <iostream>
#include <filesystem>
#include <string>
#include <ctime>
#include <exception>

using namespace std;

int main() {
    // Set Account Type
    string ACCOUNT = "PAPER"; 
    bool PLACE_TRADES = true; 
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

    // Initalize Order Management
    FXOrderManagement fx = FXOrderManagement(ACCOUNT, PLACE_TRADES, EMERGENCY_CLOSE, working_directory);

    if (EMERGENCY_CLOSE == 0) {
        fx.run_order_management_system();
    }

    time_t end_time = time(NULL);
    cout << "FX Order Management - Program Terminated Successfully: " << ctime(&end_time) << endl;

    return 0;
}