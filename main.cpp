// Copyright 2024, Andrew Drogalis
// GNU License

#include "fx_main_utilities.hpp"// for validateMainParameters, validateAccountType...
#include "fx_order_management.h"// for FXOrderManagement

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
    int MAX_RETRY_FAILURES = 3;
    // ------------------
    if (! fxordermgmt::validateMainParameters(argc, argv, ACCOUNT, PLACE_TRADES, MAX_RETRY_FAILURES)) { return 1; }
    if (! fxordermgmt::validateAccountType(ACCOUNT)) { return 1; }
    int EMERGENCY_CLOSE;
    fxordermgmt::userInputEmergencyClose(EMERGENCY_CLOSE);

    // Initialize Order Management
    fxordermgmt::FXOrderManagement fx {ACCOUNT, PLACE_TRADES, MAX_RETRY_FAILURES, EMERGENCY_CLOSE, std::filesystem::current_path()};

    auto initialization_response = fx.initialize_order_management();
    if (! initialization_response)
    {
        std::cout << initialization_response.error().what() << '\n';
        return 1;
    }

    auto run_order_mgmt_response = fx.run_order_management_system();
    if (! run_order_mgmt_response)
    {
        std::cout << run_order_mgmt_response.error().what() << '\n';
        return 1;
    }
    // -------------------
    time_t end_time = time(NULL);
    std::cout << "FX Order Management - Program Terminated Successfully: " << ctime(&end_time) << '\n';
    // -------------------
    return 0;
}
