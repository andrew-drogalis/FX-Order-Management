// Copyright 2024, Andrew Drogalis
// GNU License

#include <iostream>
#include <filesystem>
#include <string>
#include <ctime>
#include <exception>

#include "fx_order_management.h"

constexpr bool validateAccountType(std::string account) {
    if (account != "PAPER" && account != "LIVE") {
        std::cerr << "Wrong Account Type; Please Select 'PAPER' or 'LIVE'" << std::endl;
        return false;
    }
    else {
        std::cout << "Account Type Initialized: " << account << "\n" << std::endl;
        return true;
    }
}

void userInputEmergencyClose(int &emergencyClose) {
    while (true) {
        std::cout << "Close All Open Positions?\n\nPlease Enter: 1 = True; 0 = False;\n\nYour Selection: ";
        std::cin >> emergencyClose;
        std::cout << std::endl;

        if (emergencyClose == 1 ||emergencyClose == 0) { break; }
        else {
            std::cout << "Wrong Input\n";
        }
    }
}

int main() {
    // =============================================================
    // USER INPUT
    // =============================================================
    std::string ACCOUNT = "PAPER"; 
    bool PLACE_TRADES = true; 
    int EMERGENCY_CLOSE = 0;

    if (!validateAccountType(ACCOUNT)) { return 1; }
    userInputEmergencyClose(EMERGENCY_CLOSE);

    // Set Logging Parameters
    std::string working_directory = std::filesystem::current_path();

    // Initalize Order Management
    fxordermgmt::FXOrderManagement fx = fxordermgmt::FXOrderManagement(ACCOUNT, PLACE_TRADES, EMERGENCY_CLOSE, working_directory);

    if (!fx.initalize_order_management()) { return 1; }

    bool success = fx.run_order_management_system();
    if (!success) { return 1; }

    time_t end_time = time(NULL);
    std::cout << "FX Order Management - Program Terminated Successfully: " << ctime(&end_time) << std::endl;

    return 0;
}
