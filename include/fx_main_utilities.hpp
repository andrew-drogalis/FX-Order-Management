// Copyright 2024, Andrew Drogalis
// GNU License

#include <ctype.h>// for tolower, toupper

#include <algorithm>// for transform
#include <iostream> // for operator<<, basic_ostream, cout, basic_istream:...
#include <string>   // for char_traits, basic_string, allocator, operator==

namespace fxordermgmt
{

[[nodiscard]] bool validateMainParameters(int argc, char* argv[], std::string& ACCOUNT, bool& PLACE_TRADES, int& MAX_RETRY_FAILURES)
{
    if (argc > 4)
    {
        std::cout << "Too Many Arguments - Only (3) Arguments: 1. Account Type, 2. Place Trades" << '\n';
        return false;
    }
    if (argc == 1) { return true; }
    if (argc >= 2) { ACCOUNT = argv[1]; }
    if (argc >= 3)
    {
        std::string place_trades_str = argv[2];
        std::transform(place_trades_str.begin(), place_trades_str.end(), place_trades_str.begin(), ::tolower);
        // -------------------
        if (place_trades_str == "1" || place_trades_str == "true") { PLACE_TRADES = true; }
        else if (place_trades_str == "0" || place_trades_str == "false") { PLACE_TRADES = false; }
        else
        {
            std::cout << "Incorrect Argument for PLACE_TRADES - Provide Either true (1) or false (0)";
            return false;
        }
    }
    if (argc == 4)
    {
        try
        {
            MAX_RETRY_FAILURES = std::stoi(argv[3]);
        }
        catch (std::invalid_argument const& e)
        {
            std::cout << "Provide Integer for MAX_RETRY_FAILURES. " << e.what();
            return false;
        }
    }
    // -------------------
    return true;
}

[[nodiscard]] bool validateAccountType(std::string& account) noexcept
{
    std::transform(account.begin(), account.end(), account.begin(), ::toupper);
    // -------------------
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

void userInputEmergencyClose(int& emergencyClose) noexcept
{
    while (true)
    {
        std::cout << "Emergency Close All Positions?\n\nPlease Enter: 1 (Yes); 0 (No);\n\nYour Selection: ";
        // -------------------
        std::cin >> emergencyClose;
        if (std::cin.fail())
        {
            std::cout << "\nWrong Input\n";
            continue;
        }
        if (emergencyClose == 1 || emergencyClose == 0) { break; }
        else { std::cout << "\nWrong Input\n"; }
        std::cout << '\n';
    }
}

}// namespace fxordermgmt
