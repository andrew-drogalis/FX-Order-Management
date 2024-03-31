// Copyright 2024, Andrew Drogalis
// GNU License

#include "fx_order_management.h"

#include <ctime>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

bool validateAccountType(std::string& account)
{
    transform(account.begin(), account.end(), account.begin(), ::toupper);
    if (account != "PAPER" && account != "LIVE")
    {
        std::cerr << "Wrong Account Type; Please Select 'PAPER' or 'LIVE'" << '\n';
        return false;
    }
    else
    {
        std::cout << "Account Type Initialized: " << account << "\n\n";
        return true;
    }
}

void userInputEmergencyClose(int& emergencyClose)
{
    while (true)
    {
        std::cout << "Close All Open Positions?\n\nPlease Enter: 1 = True; 0 = False;\n\nYour Selection: ";
        std::cin >> emergencyClose;
        std::cout << '\n';
        if (emergencyClose == 1 || emergencyClose == 0) { break; }
        else { std::cout << "Wrong Input\n"; }
    }
}

int main(int argc, char* argv[])
{
    // =============================================================
    // USER INPUT DEFAULTS
    // =============================================================
    std::string ACCOUNT = "PAPER";
    bool PLACE_TRADES = true;
    int EMERGENCY_CLOSE = 0;

    if (argc == 2) { ACCOUNT = argv[1]; }
    else if (argc >= 3)
    {
        ACCOUNT = argv[1];
        std::string arg2 = argv[2];
        if (arg2 == "1" || arg2 == "true") { PLACE_TRADES = true; }
        else if (arg2 == "0" || arg2 == "false") { PLACE_TRADES = false; }
        if (argc > 3) { std::cout << "Too Many Arguements - (2) Args Total: 1. Account Type, 2. Place Trade Bool" << '\n'; }
    }
    if (! validateAccountType(ACCOUNT)) { return 1; }
    userInputEmergencyClose(EMERGENCY_CLOSE);

    // Set Logging Parameters
    std::string working_directory = std::filesystem::current_path();
    // Initalize Order Management
    fxordermgmt::FXOrderManagement fx = fxordermgmt::FXOrderManagement(ACCOUNT, PLACE_TRADES, EMERGENCY_CLOSE, working_directory);

    if (! fx.initalize_order_management()) { return 1; }
    if (! fx.run_order_management_system()) { return 1; }

    time_t end_time = time(NULL);
    std::cout << "FX Order Management - Program Terminated Successfully: " << ctime(&end_time) << '\n';

    return 0;
}
