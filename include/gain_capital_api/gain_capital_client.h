// Copyright 2024, Andrew Drogalis
// GNU License

#ifndef GAIN_CAPITAL_CLIENT_H
#define GAIN_CAPITAL_CLIENT_H

#include <cstddef>        // for size_t
#include <expected>       // for expected
#include <source_location>// for source_location...
#include <string>         // for basic_string
#include <unordered_map>  // for unordered_map

#include "cpr/cprtypes.h"// for Header
#include "json/json.hpp" // for json_ref

#include "gain_capital_exception.h"// for GCException

namespace gaincapital
{

class GCClient
{

  public:
    std::string CLASS_trading_account_id;
    std::string CLASS_client_account_id;
    std::unordered_map<std::string, std::string> market_id_map;

    GCClient() = default;

    GCClient(std::string const& username, std::string const& password, std::string const& appkey);

    ~GCClient() = default;

    // Move ONLY | No Copy Constructor
    GCClient(GCClient const& obj) = delete;

    GCClient& operator=(GCClient const& obj) = delete;

    GCClient(GCClient&& obj) noexcept = default;

    GCClient& operator=(GCClient&& obj) noexcept = default;

    // =================================================================================================================
    // AUTHENTICATION
    // =================================================================================================================

    [[nodiscard]] std::expected<bool, GCException> authenticate_session();

    [[nodiscard]] std::expected<bool, GCException> validate_session();

    // =================================================================================================================
    // API CALLS
    // =================================================================================================================

    [[nodiscard]] std::expected<nlohmann::json, GCException> get_account_info();

    [[nodiscard]] std::expected<nlohmann::json, GCException> get_margin_info();

    [[nodiscard]] std::expected<nlohmann::json, GCException> get_market_id(std::string const& market_name);

    [[nodiscard]] std::expected<nlohmann::json, GCException> get_market_info(std::string const& market_name);

    [[nodiscard]] std::expected<nlohmann::json, GCException> get_prices(std::string const& market_name, std::size_t const num_ticks = 1,
                                                                        std::size_t const from_ts = 0, std::size_t const to_ts = 0,
                                                                        std::string price_type = "MID");

    [[nodiscard]] std::expected<nlohmann::json, GCException> get_ohlc(std::string const& market_name, std::string interval,
                                                                      std::size_t const num_ticks = 1, std::size_t span = 1,
                                                                      std::size_t const from_ts = 0, std::size_t const to_ts = 0);

    [[nodiscard]] std::expected<nlohmann::json, GCException> trade_order(nlohmann::json& trade_map, std::string type, std::string tr_account_id = "");

    [[nodiscard]] std::expected<nlohmann::json, GCException> list_open_positions(std::string tr_account_id = "");

    [[nodiscard]] std::expected<nlohmann::json, GCException> list_active_orders(std::string tr_account_id = "");

    [[nodiscard]] std::expected<nlohmann::json, GCException> cancel_order(std::string const& order_id, std::string tr_account_id = "");

    // =================================================================================================================
    // UTILITIES
    // =================================================================================================================

    [[nodiscard]] std::expected<bool, GCException> validate_session_header() const;

    [[nodiscard]] std::expected<bool, GCException> validate_auth_payload() const;

    [[nodiscard]] bool validate_account_ids() const noexcept;

    void set_testing_rest_urls(std::string const& url);

  private:
    std::string rest_url_v2 = "https://ciapi.cityindex.com/v2";
    std::string rest_url = "https://ciapi.cityindex.com/TradingAPI";
    cpr::Header session_header;
    nlohmann::json auth_payload, session_payload;

    // =================================================================================================================
    // AUTHENTICATION
    // =================================================================================================================

    [[nodiscard]] std::expected<bool, GCException> set_trading_account_id();

    // =================================================================================================================
    // UTILITIES
    // =================================================================================================================

    [[nodiscard]] std::expected<nlohmann::json, GCException> make_network_call(
        cpr::Header const& header, cpr::Url const& url, std::string const& payload, std::string const& type,
        std::source_location const& location = std::source_location::current());
};

}// namespace gaincapital

#endif
