// Copyright 2024, Andrew Drogalis
// GNU License

#include <algorithm>// for transform
#include <ctype.h>  // for tolower, toupper
#include <iostream> // for operator<<, basic_ostream, cout, basic_istream:...
#include <string>   // for char_traits, basic_string, allocator, operator==

namespace fxordermgmt
{

[[nodiscard]] bool validateAccountType(std::string& account) noexcept
{
    std::transform(account.begin(), account.end(), account.begin(), ::toupper);
    // -------------------------------
    if (account != "PAPER" && account != "LIVE")
    {
        std::cout << "Wrong Account Type; Please Select 'PAPER' or 'LIVE'" << '\n';
        return false;
    }
    else
    {
        std::cout << "Account Type Initialized: " << account << "\n\n";
        return true;
    }
}

[[nodiscard]] bool validateMainParameters(int argc, char* argv[], std::string& ACCOUNT, bool& PLACE_TRADES)
{
    if (argc == 2) { ACCOUNT = argv[1]; }
    else if (argc >= 3)
    {
        ACCOUNT = argv[1];
        std::string place_trade_arg = argv[2];
        std::transform(place_trade_arg.begin(), place_trade_arg.end(), place_trade_arg.begin(), ::tolower);
        // -------------------------------
        if (place_trade_arg == "1" || place_trade_arg == "true") { PLACE_TRADES = true; }
        else if (place_trade_arg == "0" || place_trade_arg == "false") { PLACE_TRADES = false; }
        else
        {
            std::cout << "Incorrect Argument for PLACE_TRADES - Provide Either true (1) or false (0)";
            return false;
        }
        // -------------------------------
        if (argc > 3)
        {
            std::cout << "Too Many Arguments - Only (2) Args Total: 1. Account Type, 2. Place Trade Bool" << '\n';
            return false;
        }
    }
    // -------------
    return true;
}

void userInputEmergencyClose(int& emergencyClose)
{
    while (true)
    {
        std::cout << "Close All Open Positions?\n\nPlease Enter: 1 = True; 0 = False;\n\nYour Selection: ";
        // -------------------------------
        try
        {
            std::cin >> emergencyClose;
        }
        catch (...)
        {
            std::cout << "\nWrong Input\n";
            continue;
        }
        // -------------------------------
        std::cout << '\n';

        if (emergencyClose == 1 || emergencyClose == 0) { break; }
        else { std::cout << "Wrong Input\n"; }
    }
}

}// namespace fxordermgmt
