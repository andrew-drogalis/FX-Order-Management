// Copyright 2024, Andrew Drogalis
// GNU License

#include <ctype.h>// for tolower, toupper
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <algorithm>// for transform
#include <iostream> // for operator<<, basic_ostream, cout, basic_istream:...
#include <string>   // for char_traits, basic_string, allocator, operator==

namespace fxordermgmt
{

[[nodiscard]] bool validateMainParameters(int argc, char* argv[], std::string& ACCOUNT, bool& PLACE_TRADES, int& MAX_RETRY_FAILURES)
{
    if (argc > 7)
    {
        std::cout << "Too Many Arguments - Only (3) Arguments: -a [Account Type], -p [Place Trades] -m [Max Retry Failures]\n";
        return false;
    }
    if (argc == 1)
    {
        return true;
    }

    int c;
    bool success = true;
    while ((c = getopt(argc, argv, "a:p:m:")) != -1)
    {
        switch (c)
        {
        case 'a':
            ACCOUNT = optarg;
            break;
        case 'p': {
            std::string place_trades_str = optarg;
            std::transform(place_trades_str.begin(), place_trades_str.end(), place_trades_str.begin(), ::tolower);
            // -------------------
            if (place_trades_str == "1" || place_trades_str == "true")
            {
                PLACE_TRADES = true;
            }
            else if (place_trades_str == "0" || place_trades_str == "false")
            {
                PLACE_TRADES = false;
            }
            else
            {
                std::cout << "Incorrect Argument for PLACE_TRADES - Provide Either true (1) or false (0)";
                success = false;
            }
            break;
        }
        case 'm': {
            try
            {
                MAX_RETRY_FAILURES = std::stoi(optarg);
            }
            catch (std::invalid_argument const& e)
            {
                std::cout << "Provide Integer for MAX_RETRY_FAILURES. " << e.what();
                success = false;
            }
            break;
        }
        default:
            std::cout << "Incorrect Argument. Flags are -a [Account Type], -p [Place Trades] -m [Max Retry Failures]\n";
            success = false;
        }
    }
    // -------------------
    return success;
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
        if (emergencyClose == 1 || emergencyClose == 0)
        {
            break;
        }
        else
        {
            std::cout << "\nWrong Input\n";
        }
        std::cout << '\n';
    }
}

}// namespace fxordermgmt
