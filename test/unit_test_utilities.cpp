// Copyright 2024, Andrew Drogalis
// GNU License

#include "fx_market_time.h"
#include "main_utilities.hpp"
#include "fx_utilities.h"

#include "gtest/gtest.h"

namespace {

TEST(ForexUtilitiesTests, DefaultConstructor) {
    int const update_seconds_interval = 60;
    fxordermgmt::FXUtilities fxUtils{};
    fxordermgmt::FXMarketTime fxTime{update_seconds_interval, fxUtils};
    
    //EXPECT_EQ(g.trading_account_id, "");
    //EXPECT_EQ(g.client_account_id, "");
}



} // namespace