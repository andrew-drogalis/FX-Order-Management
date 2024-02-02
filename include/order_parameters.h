#ifndef ORDER_PARAMETERS_H
#define ORDER_PARAMETERS_H

#include <string>
#include <vector>

namespace std {

vector<string> fx_symbols_to_trade = {"USD/JPY", "EUR/USD", "USD/CHF", "USD/CAD"};

int order_position_size = 2'000; // Lot Size

int num_data_points = 1'000;

string update_interval = "MINUTE"; // MINUTE or HOUR

int update_span = 1; // Span of Interval e.g. 5 Minutes

}

#endif
