// Copyright 2024, Andrew Drogalis
// GNU License

#include <filesystem>
#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "json/json.hpp"

#include "fx_order_management.h"

namespace
{

nlohmann::json parse_json_file(std::string const& file_path)
{
    nlohmann::json data;
    std::ifstream in(file_path);
    if (in.is_open())
    {
        try
        {
            data = nlohmann::json::parse(in);
            in.close();
        }
        catch (nlohmann::json::exception const& e)
        {
            if (in.is_open())
            {
                in.close();
            }
        }
    }
    return data;
}

TEST(FXOrderManagementTests, Parameterized_Constructor)
{
    std::string paper_or_live = "PAPER";
    bool place_trades = true, file_logging = true;
    int max_retry_failures = 3, emergency_close = 0;

    fxordermgmt::FXOrderManagement fx_mgmt {};

    // EXPECT_EQ(g.trading_account_id, "");
    // EXPECT_EQ(g.client_account_id, "");
}

}// namespace