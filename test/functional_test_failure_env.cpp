// Copyright 2024, Andrew Drogalis
// GNU License

#include "fx_order_management.h"

#include <iostream>
#include <string>
#include <typeinfo>

#include "httpmockserver/mock_server.h"
#include "httpmockserver/test_environment.h"
#include "gtest/gtest.h"

#include "fx_exception.h"

namespace
{

std::string URL = "http://localhost:9201";

class HTTPMock : public httpmock::MockServer
{
  public:
    /// Create HTTP server on port 9201
    explicit HTTPMock(int port = 9201) : MockServer(port) {}

  private:
    /// Handler called by MockServer on HTTP request.
    Response responseHandler(std::string const& url, std::string const& method, std::string const& data, std::vector<UrlArg> const& urlArguments,
        std::vector<Header> const& headers)
    {
        // Authenticate Session
        if (method == "POST" && matchesPrefix(url, "/Session") && matchesAuth(url))
        {

            return Response(200, "{\"statusCode\": 0, \"session\": \"123\"}");
        }
        // Validate Session
        else if (method == "POST" && matchesPrefix(url, "/Session/validate"))
        {

            return Response(200, "{\"isAuthenticated\": true}");
        }
        // Account Info
        else if (method == "GET" && matchesPrefix(url, "/userAccount/ClientAndTradingAccount"))
        {

            return Response(200,
                "{\"tradingAccounts\": [{\"tradingAccountId\":\"TradingTestID\", \"clientAccountId\":\"ClientTestID\",\"SampleParam\":\"123\"}]}");
        }
        // Margin Info
        else if (method == "GET" && matchesMargin(url, "/margin/clientAccountMargin"))
        {

            return Response(200, "{\"SampleParam\":\"123\"}");
        }
        // Market IDs & Market Info
        else if (method == "GET" && matchesMarkets(url, "/cfd/markets"))
        {

            return Response(200, "{\"Markets\": [{\"MarketId\": 123,\"SampleParam\":\"123\"}]}");
        }
        // Prices
        else if (method == "GET" && matchesPrices(url, "/market/123/tickhistory"))
        {

            return Response(200, "{\"PriceTicks\":[{\"Price\" : 1.0}]}");
        }
        // OHLC
        else if (method == "GET" && matchesOHLC(url, "/market/123/barhistory"))
        {

            return Response(200, "{\"PriceBars\": \"123\"}");
        }
        // Trade Market Order
        else if (method == "POST" && matchesPrefix(url, "/order/newtradeorder"))
        {

            return Response(200, "{\"OrderId\": 1}");
        }
        // Trade Limit Order
        else if (method == "POST" && matchesPrefix(url, "/order/newstoplimitorder"))
        {

            return Response(200, "{\"OrderId\": 1}");
        }
        // List Open Positons
        else if (method == "GET" && matchesOpenPositions(url, "/order/openpositions"))
        {

            return Response(200, "{\"OpenPositions\": \"123\"}");
        }
        // List Active Orders
        else if (method == "POST" && matchesPrefix(url, "/order/activeorders"))
        {

            return Response(200, "{\"ActiveOrders\": \"123\"}");
        }
        // Cancel Order
        else if (method == "POST" && matchesPrefix(url, "/order/cancel"))
        {

            return Response(200, "{\"RESPONSE\": 123}");
        }
        // Return "URI not found" for the undefined methods
        return Response(404, "Not Found");
    }

    /// Return true if \p url starts with \p str.
    bool matchesPrefix(std::string const& url, std::string const& str) const { return url.substr(0, str.size()) == str; }

    bool matchesAuth(std::string const& url) const { return url == "/Session"; }

    bool matchesMargin(std::string const& url, std::string const& str) const { return url.substr(0, 27) == str.substr(0, 27); }

    bool matchesMarkets(std::string const& url, std::string const& str) const { return url.substr(0, 12) == str.substr(0, 12); }

    bool matchesOpenPositions(std::string const& url, std::string const& str) const { return url.substr(0, 20) == str.substr(0, 20); }

    bool matchesPrices(std::string const& url, std::string const& str) const { return url.substr(0, 23) == str.substr(0, 23); }

    bool matchesOHLC(std::string const& url, std::string const& str) const { return url.substr(0, 22) == str.substr(0, 22); }
};

TEST(GainCapital, DefaultConstructor) {}


} //namespace

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new httpmock::TestEnvironment<HTTPMock>());
    return RUN_ALL_TESTS();
}
