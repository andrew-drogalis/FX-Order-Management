// Copyright 2024, Andrew Drogalis
// GNU License

#include "fx_order_management.h"// for FXOrderManagement
#include "main_utilities.hpp"   // for validateMainParameters, validateAccountType...

#include <filesystem>// for current_path, path
#include <iostream>  // for operator<<, basic_ostream, cout
#include <string>    // for char_traits, allocator, basic_string, string
#include <time.h>    // for ctime, time, NULL, time_t

int main(int argc, char* argv[])
{
    // =============================================================
    // USER INPUT DEFAULTS
    // =============================================================
    std::string ACCOUNT = "PAPER";
    bool PLACE_TRADES = true;
    int EMERGENCY_CLOSE = 0;

    // -------------------------------
    if (! fxordermgmt::validateMainParameters(argc, argv, ACCOUNT, PLACE_TRADES)) { return 1; }
    if (! fxordermgmt::validateAccountType(ACCOUNT)) { return 1; }
    fxordermgmt::userInputEmergencyClose(EMERGENCY_CLOSE);

    std::string working_directory = std::filesystem::current_path();
    // Initialize Order Management
    fxordermgmt::FXOrderManagement fx = fxordermgmt::FXOrderManagement(ACCOUNT, PLACE_TRADES, EMERGENCY_CLOSE, working_directory);

    if (! fx.initialize_order_management()) { return 1; }
    if (! fx.run_order_management_system()) { return 1; }

    // -------------------------------
    time_t end_time = time(NULL);
    std::cout << "FX Order Management - Program Terminated Successfully: " << ctime(&end_time) << '\n';

    return 0;
}
