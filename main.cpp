// Copyright 2024, Andrew Drogalis
// GNU License

#include <iostream>
#include <filesystem>
#include <string>
#include <ctime>
#include <exception>

#include "fx_order_management.h"

int main() {
    // Set Account Type
    std::string ACCOUNT = "PAPER"; 
    bool PLACE_TRADES = true; 
    int EMERGENCY_CLOSE = 0;

    if (ACCOUNT != "PAPER" && ACCOUNT != "LIVE") {
        std::cerr << "Wrong Account Type; Please Select 'PAPER' or 'LIVE'" << std::endl;
        std::terminate();
    }
    else {
        std::cout << "Account Type Initialized: " << ACCOUNT << "\n" << std::endl;
    }

    // Check if Emergency Scenario
    while (true) {
        std::cout << "Close All Open Positions?\n\nPlease Enter: 1 = True; 0 = False;\n\nYour Selection: ";
        std::cin >> EMERGENCY_CLOSE;
        std::cout << std::endl;

        if (EMERGENCY_CLOSE == 1 || EMERGENCY_CLOSE == 0) { break;}
        else {
            std::cout << "Wrong Input\n";
        }
    }

    // Set Logging Parameters
    std::string working_directory = std::filesystem::current_path();

    // Initalize Order Management
    fxordermgmt::FXOrderManagement fx = fxordermgmt::FXOrderManagement(ACCOUNT, PLACE_TRADES, EMERGENCY_CLOSE, working_directory);

    if (EMERGENCY_CLOSE == 0) {
        fx.run_order_management_system();
    }

    time_t end_time = time(NULL);
    std::cout << "FX Order Management - Program Terminated Successfully: " << ctime(&end_time) << std::endl;

    return 0;
}
