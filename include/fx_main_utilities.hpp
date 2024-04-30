// Copyright 2024, Andrew Drogalis
// GNU License

#include <ctype.h> // for tolower, toupper
#include <unistd.h>// for optarg, getopt

#include <algorithm>// for transform
#include <iostream> // for operator<<, basic_ostream, cout, basic_istream
#include <print>    // for print
#include <stdexcept>// for invalid_argument
#include <string>   // for char_traits, basic_string, allocator, operator==

namespace fxordermgmt
{

[[nodiscard]] bool check_if_boolean(std::string optarg, std::string const& name, bool& bool_value) noexcept
{
    std::transform(optarg.begin(), optarg.end(), optarg.begin(), ::tolower);

    if (optarg == "1" || optarg == "true")
    {
        bool_value = true;
    }
    else if (optarg == "0" || optarg == "false")
    {
        bool_value = false;
    }
    else
    {
        std::cout << "Incorrect Argument for " << name << " - Provide Either true (1) or false (0)";
        return false;
    }
    // -------------------
    return true;
}

[[nodiscard]] bool validateMainParameters(
    int argc, char* argv[], std::string& ACCOUNT, int& MAX_RETRY_FAILURES, bool& PLACE_TRADES, bool& EMERGENCY_CLOSE, bool& FILE_LOGGING)
{
    if (argc > 11)
    {
        std::cout << "Too Many Arguments - Only (5) Arguments: -a [Account Type], -p [Place Trades] -m [Max Retry Failures] -e [Emergency Close] -f "
                     "[File Logging]\n";
        return false;
    }
    if (argc == 1)
    {
        return true;
    }

    int c;
    while ((c = getopt(argc, argv, "a:p:m:e:f:")) != -1)
    {
        switch (c)
        {
        case 'a': {
            ACCOUNT = optarg;
            if (ACCOUNT != "PAPER" && ACCOUNT != "LIVE")
            {
                std::cout << "Wrong Account Type; Select either 'PAPER' or 'LIVE''\n'";
                return false;
            }
            break;
        }
        case 'p': {
            if (! check_if_boolean(optarg, "PLACE_TRADES", PLACE_TRADES))
            {
                return false;
            }
            break;
        }
        case 'e': {
            if (! check_if_boolean(optarg, "EMERGENCY_CLOSE", EMERGENCY_CLOSE))
            {
                return false;
            }
            break;
        }
        case 'f': {
            if (! check_if_boolean(optarg, "FILE_LOGGING", FILE_LOGGING))
            {
                return false;
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
                return false;
            }
            break;
        }
        default:
            std::cout << "Incorrect Argument. Flags are -a [Account Type], -p [Place Trades] -m [Max Retry Failures] -e [Emergency Close] -f [File "
                         "Logging]\n";
            return false;
        }
    }
    std::print(std::cout, "Account Type: {}, Place Trades: {}, Max Retry Failures: {}, Emergency Close: {}, File Logging: {}", ACCOUNT, PLACE_TRADES,
        MAX_RETRY_FAILURES, EMERGENCY_CLOSE, FILE_LOGGING);
    // -------------------
    return true;
}

}// namespace fxordermgmt
