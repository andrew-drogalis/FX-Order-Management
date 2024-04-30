// Copyright 2024, Andrew Drogalis
// GNU License

#include "fx_main_utilities.hpp"// for validateMainParameters
#include "fx_order_management.h"// for FXOrderManagement

#include <filesystem>// for current_path, path
#include <iostream>  // for operator<<, basic_ostream, cout
#include <string>    // for char_traits, allocator, basic_string, string
#include <time.h>    // for ctime, time, NULL, time_t

int main(int argc, char* argv[])
{
    // USER INPUT DEFAULTS
    std::string ACCOUNT = "PAPER";
    bool PLACE_TRADES = true, EMERGENCY_CLOSE = false, FILE_LOGGING = true;
    int MAX_RETRY_FAILURES = 3;

    if (! fxordermgmt::validateMainParameters(argc, argv, ACCOUNT, MAX_RETRY_FAILURES, PLACE_TRADES, EMERGENCY_CLOSE, FILE_LOGGING))
    {
        return 1;
    }
    // ------------------
    fxordermgmt::FXOrderManagement fxOrderMgmt {
        ACCOUNT, MAX_RETRY_FAILURES, PLACE_TRADES, EMERGENCY_CLOSE, FILE_LOGGING, std::filesystem::current_path()};

    auto initialization_response = fxOrderMgmt.initialize_order_management();
    if (! initialization_response)
    {
        std::cout << "Error Location: " << initialization_response.error().where() << '\n';
        std::cout << initialization_response.error().what() << '\n';
        return 1;
    }

    auto run_order_mgmt_response = fxOrderMgmt.run_order_management_system();
    if (! run_order_mgmt_response)
    {
        std::cout << "Error Location: " << run_order_mgmt_response.error().where() << '\n';
        std::cout << run_order_mgmt_response.error().what() << '\n';
        return 1;
    }

    time_t end_time = time(NULL);
    std::cout << "Program Terminated Successfully: " << ctime(&end_time) << '\n';
    // -------------------
    return 0;
}
