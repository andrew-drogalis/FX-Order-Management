// Copyright 2024, Andrew Drogalis
// GNU License

#include <iostream>
#include <string>

namespace fxordermgmt
{

bool validateAccountType(std::string& account)
{
    std::transform(account.begin(), account.end(), account.begin(), ::toupper);
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

bool validateMainParameters(int argc, char* argv[], std::string& ACCOUNT, bool& PLACE_TRADES) {
    if (argc == 2) { ACCOUNT = argv[1]; }
    else if (argc >= 3)
    {
        ACCOUNT = argv[1];
        std::string arg2 = argv[2];
        std::transform(arg2.begin(), arg2.end(), arg2.begin(), ::tolower);
        if (arg2 == "1" || arg2 == "true") { PLACE_TRADES = true; }
        else if (arg2 == "0" || arg2 == "false") { PLACE_TRADES = false; }
        else {
            std::cout << "Incorrect Argument for PLACE_TRADES - Provide Either true (1) or false (0);
            return false;
        }
        if (argc > 3) 
        { 
           std::cout << "Too Many Arguments - (2) Args Total: 1. Account Type, 2. Place Trade Bool" << '\n'; 
           return false;
         }
    }
    return true;
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

} // fxordermgmt
