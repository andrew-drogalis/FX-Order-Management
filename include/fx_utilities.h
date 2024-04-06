// Copyright 2024, Andrew Drogalis
// GNU License

#ifndef FX_UTILITIES_H
#define FX_UTILITIES_H

#include <string>

namespace fxordermgmt
{

class FXUtilities
{

  public:
    FXUtilities();

    bool setup_password_first_time(std::string account_type, std::string username);

    void init_logging(std::string working_directory);

    std::string get_todays_date();

    bool validate_user_interval(std::string update_interval, int update_span, int& update_frequency_seconds);
};

}// namespace fxordermgmt

#endif
