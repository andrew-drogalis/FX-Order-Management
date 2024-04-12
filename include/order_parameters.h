// Copyright 2024, Andrew Drogalis
// GNU License

#ifndef ORDER_PARAMETERS_H
#define ORDER_PARAMETERS_H

#ifndef VAR_DECLS_ORDER
#define _DECL extern
#define _INIT(x)
#define _INIT_VECT(...)
#else
#define _DECL
#define _INIT(x)        = x
#define _INIT_VECT(...) = {__VA_ARGS__}
#endif

#include <string>
#include <vector>

namespace fxordermgmt
{

_DECL std::vector<std::string> const fx_symbols_to_trade _INIT_VECT("USD/JPY", "EUR/USD", "USD/CHF", "USD/CAD");

_DECL int const order_position_size _INIT(2'000);// Lot Size

_DECL int const num_data_points _INIT(1'000);// Historical Data

_DECL std::string update_interval _INIT("MINUTE");// MINUTE or HOUR

_DECL int const update_span _INIT(5);// Span of Interval e.g. 5 Minutes

_DECL int const start_hr _INIT(8);// User defined Start Time in London Time (Forex Trading on London exchange)

_DECL int const end_hr _INIT(20);// User defined End Time in London Time (Forex Trading on London exchange)

}// namespace fxordermgmt

#endif
