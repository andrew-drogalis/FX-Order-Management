
# FX Order Management

## Table of Contents

* [Getting Started](#Getting-Started)
    * [Downloading Dependencies](#Downloading-Dependencies)
* [Instructions](#Instructions)
    * [Storing User Credentials](#Storing-User-Credentials)
    * [Updating Order Parameters](#Updating-Order-Parameters)
    * [Setting Password in Keyring](#Setting-Password-in-Keyring)
    * [Updating Trading Model](#Updating-Trading-Model)
    * [Switch to Live Trading](#Switch-to-Live-Trading)
    * [Profitability Reports](#Profitability-Reports)
    * [Closing Trades Manually](#Closing-Trades-Manually)
* [Building Executable](#Building-Executable)
* [Dependencies](#Dependencies)
* [License](#License)
* [Contributing](#Contribution)

## Getting Started

This repository is dependent upon the API from Gain Capital's Forex.com. To make an account visit their website [Forex.com](https://www.forex.com). Once an account is made you will need to send them an email and request an API key.

### Downloading Dependencies

Please see a link to required dependencies [below](#Dependencies). If you are in Linux, Boost & OpenSSL & libsecret can be installed with the following commands below. Additionally, this repository comes with a .devcontainer directory. The .devcontainer has all the required dependencies and can be run inside a Docker container.

```
    Fedora:
        dnf install boost-devel openssl-devel libsecret-devel libmicrohttpd-devel
    Debian:
        apt-get install libboost-all-dev libssl-dev libsecret-1-dev libmicrohttpd-devel
    Arch: 
        pacman -Ss boost libmicrohttpd
        pacman -S --needed openssl
        pacman -Sy libsecret
```

## Instructions

### Running the Executable

The arguments to the main function are as follows:

```bash
./FX-Order-Management -a [Account Type], -p [Place Trades] -m [Max Retry Failures]
```

```text
Options:
    Account Type: Must be "Live" or "Paper" - capitalization doesn't matter
    Place Trades: Boolean - spelled out or binary values
    Max Retry Failures: Int - recommended over 0
```

The default settings in the main.cpp file are the following:

```c
int main(int argc, char* argv[])
{
    std::string ACCOUNT = "PAPER"; 
    bool PLACE_TRADES = true; 
    int MAX_RETRY_FAILURES = 3;
    ...
```

### Storing User Credentials

The user should replace the usernames and API key, but the password should not be stored in plain text. The program will prompt the user to assign a new password if one doesn't already exist in the keyring.

```json
{
    "Username": "Blank",
    "Paper_Username": "Blank",
    "API_Key": "Blank",
    "Positions": [
        "USD/JPY",
        "EUR/USD",
        "USD/CHF",
        "USD/CAD"
    ],
    "Order_Size": 1000,
    "Update_Interval": "Minute",
    "Update_Span": 5,
    "Num_Data_Points": 10000,
    "Start_Hour_London_Exchange": 8,
    "End_Hour_London_Exchange": 20
}
```

```txt
Options:
    Order_Size: Intervals of 1,000;
    Update_Interval: Minute or Hour;
    Update_Span: MINUTES: 1, 2, 3, 5, 10, 15, 30; HOURS: 1, 2, 4, 8;
    Start_Hour_London_Exchange: 0 - 24; All local times are adjusted to coordinate with the London Forex Exchange;
    End_Hour_London_Exchange: 0 - 24; All local times are adjusted to coordinate with the London Forex Exchange;
```

### Updating Order Parameters

The user can replace the order parameters with any valid combination as described in the Gain Capital API documents. In the case of a typo, the code provides appropriate checks to confirm the user is compliant with the documentation.


### Setting Password in Keyring

User will be prompted the first time they use the application. The password will be stored securely in the keyring and automatically loaded for the next use.

### Updating Trading Model

Please don't run in a live trading environment with the placeholder trading model provided. The user should modify with their own trading strategy.

```c
int FXTradingModel::send_trading_signal()
{
    // User's code to Buy / Sell
    int const signal = rand() % 2;
    return (signal) ? 1 : -1;
}
```

### Profitability Reports

```json
{
    "Last Updated": "Thu Feb  1 14:04:06 2024",
    "Performance Information": 
    {
        "Current Funds": 45884.16,
        "Inital Funds": 45887.76,
        "Margin Utilized": 121.73,
        "Profit Cumulative": -3.59,
        "Profit Percent Cumulative": -0.0
    },
    "Position Information": 
    {
        "EUR/USD": 
        {
            "Current Price": 1.087210,
            "Direction": "buy",
            "Entry Price": 1.08727,
            "Profit": -5.999999e-05,
            "Profit Percent": -0.009999,
            "Quantity": 1000.0
        },
        "USD/CAD": 
        {
            "Current Price": 1.337329,
            "Direction": "buy",
            "Entry Price": 1.33754,
            "Profit": -0.000209,
            "Profit Percent": -0.019999,
            "Quantity": 1000.0
        }
    }
}
```

### Closing Trades Manually

Changing the value to true will either close the trade immediately upon the update interval, or wait until the trading model signal changes.

```json
{
    "EUR/USD": 
    {
        "Close_Position": false,
        "Position_Size_Multiple": 1.0
    },
    "USD/CAD": 
    {
        "Close_Position": false,
        "Position_Size_Multiple": 1.0
    }
}
```

```txt
Options:
    Close_Position: true or false; Closes position at next update interval;
    Position_Size_Multiple: Any Double Number; Changes the quantity relative to the Order_Size;
```

# Building Executable

```
    $ mkdir build
    $ cmake -S . -B build
    $ cmake --build build --target FX-Order-Management
```

# Dependencies

- [Boost](https://www.boost.org/) - *Must be installed by user*
- [Libsecret](https://wiki.gnome.org/Projects/Libsecret) - *For Linux, Must be installed by user*
- [OpenSSL](https://www.openssl.org/) - *Must be installed by user*
- [libmicrohttpd](https://www.gnu.org/software/libmicrohttpd/) - Required for Tests Only - *Must be installed by user*
- [CPR](https://github.com/libcpr/cpr) - Included in CMake Fetch Content
- [JSON](https://github.com/nlohmann/json) - .hpp file included in repository
- [Gain Capital API C++](https://github.com/andrew-drogalis/Gain-Capital-API-Cpp) - Library included in repository
- [Keychain](https://github.com/hrantzsch/keychain) - Library included in repository
- [httpmockserver](https://github.com/seznam/httpmockserver) - Required for Tests Only - Library included in repository

## License

This software is distributed under the GNU license. Please read [LICENSE](https://github.com/andrew-drogalis/FX-Order-Management/blob/main/LICENSE) for information on the software availability and distribution.

## Contribution

Please open an issue of if you have any questions, suggestions, or feedback.

Please submit bug reports, suggestions, and pull requests to the [GitHub issue tracker](https://github.com/andrew-drogalis/FX-Order-Management/issues).
