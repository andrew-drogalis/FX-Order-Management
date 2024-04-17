// Copyright 2024, Andrew Drogalis
// GNU License

#ifndef ORDER_PARAMETERS_H
#define ORDER_PARAMETERS_H

#include <string>// for basic_string
#include <vector>// for vector

namespace fxordermgmt
{

extern std::vector<std::string> fx_symbols_to_trade = {"USD/JPY", "EUR/USD", "USD/CHF", "USD/CAD"};

// Lot Size
extern int const order_position_size = 2'000;

// Historical Data Length
extern int const num_data_points = 1'000;

// MINUTE or HOUR
extern std::string update_interval = "MINUTE";

// Span of Interval e.g. 5 Minutes
// MINUTES: 1, 2, 3, 5, 10, 15, 30; HOURS: 1, 2, 4, 8;
extern int const update_span = 5;

// User defined Start Time in London Time (Forex Trading on London exchange)
extern int const start_hr = 8;

// User defined End Time in London Time (Forex Trading on London exchange)
extern int const end_hr = 20;

}// namespace fxordermgmt

#endif