
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

Please see a link to required dependencies [below](#Dependencies). If you are in Linux, Boost & OpenSSL & libsecret can be installed with the following commands below.

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

### Storing User Credentials

```c
    #include <string>

    namespace fxordermgmt {

    _DECL std::string account_username _INIT("BLANK");

    _DECL std::string paper_account_username _INIT("BLANK");

    _DECL std::string forex_api_key _INIT("BLANK");

    // Store Passwords in Keyring.

    }
```

### Updating Order Parameters

```c
    #include <string>
    #include <vector>

    namespace fxordermgmt {

    _DECL std::vector<std::string> fx_symbols_to_trade _INIT_VECT("USD/JPY", "EUR/USD", "USD/CHF", "USD/CAD");

    _DECL int order_position_size _INIT(2'000); // Lot Size

    _DECL int num_data_points _INIT(1'000);

    _DECL std::string update_interval _INIT("MINUTE"); // MINUTE or HOUR

    _DECL int update_span _INIT(5); // Span of Interval e.g. 5 Minutes

    }
```

### Setting Password in Keyring

User will be prompted the first time they use the application. The password will be stored securely in the keyring and automatically loaded for the next use.

```c
   if (account_type == "PAPER") {
        error = keychain::Error{};
        password = keychain::getPassword(package_test, service_id_test, paper_account_username, error);
        
        if (error.type == keychain::ErrorType::NotFound) {
            std::cout << "Test Account password not found. Please input password: ";
            std::cin >> test_account_password;
            // Test Password Setup
            keychain::setPassword(package_test, service_id_test, paper_account_username, test_account_password, error);
            if (error) {
                std::cerr << "Test Account " << error.message << std::endl;
                return false;
            }
        } else if (error) {
            std::cerr << error.message << std::endl;
            return false;
        }
    }
```

### Updating Trading Model

Please don't run in a live trading environment with the placeholder trading model provided. 

```c
#include <trading_model.h>

#include <unordered_map>
#include <vector>
#include <string>

namespace fxordermgmt {

TradingModel::TradingModel() { }

TradingModel::~TradingModel() { }

TradingModel::TradingModel(std::unordered_map<std::string, std::vector<float>> historical_data){
    open_data = historical_data["Open"];
    high_data = historical_data["High"];
    low_data = historical_data["Low"];
    close_data = historical_data["Close"];
    datetime_data = historical_data["Datetime"];
}

void TradingModel::receive_latest_market_data(std::unordered_map<string, vector<float>> historical_data) {
    // User Defined Trading Model
    open_data = historical_data["Open"];
    high_data = historical_data["High"];
    low_data = historical_data["Low"];
    close_data = historical_data["Close"];
    datetime_data = historical_data["Datetime"];
}

int TradingModel::send_trading_signal(){
    float signal = ((double) rand() / (RAND_MAX));
    signal = (signal > 0.5) ? 1 : -1;
    return signal;
}

} // namespace fxordermgmt
```

### Switch to Live Trading

The default is paper trading, switch to live account and set place trades to true.

```c
int main(int argc, char* argv[]) {
    // =============================================================
    // USER INPUT DEFAULTS
    // =============================================================
    std::string ACCOUNT = "PAPER"; 
    bool PLACE_TRADES = true; 
    int EMERGENCY_CLOSE = 0;
```

### Profitability Reports

```json
{
    "Last Updated": "Thu Feb  1 14:04:06 2024",
    "Performance Information": {
        "Current Funds": 45884.16,
        "Inital Funds": 45887.76,
        "Margin Utilized": 121.73,
        "Profit Cumulative": -3.59,
        "Profit Percent Cumulative": -0.0
    },
    "Position Information": {
        "EUR/USD": {
            "Current Price": 1.087210,
            "Direction": "buy",
            "Entry Price": 1.08727,
            "Profit": -5.999999e-05,
            "Profit Percent": -0.009999,
            "Quantity": 1000.0
        },
        "USD/CAD": {
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
    "EUR/USD": {
        "Close Immediately": false,
        "Close On Trade Signal Change": false
    },
    "USD/CAD": {
        "Close Immediately": false,
        "Close On Trade Signal Change": false
    }
}
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
