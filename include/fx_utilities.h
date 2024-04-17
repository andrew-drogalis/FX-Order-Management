// Copyright 2024, Andrew Drogalis
// GNU License

#ifndef FX_UTILITIES_H
#define FX_UTILITIES_H

#include <string>// for basic_string

namespace fxordermgmt
{

class FXUtilities
{

  public:
    FXUtilities() noexcept = default;

    [[nodiscard]] bool setup_password_first_time(std::string const& account_type, std::string const& username);

    static void init_logging(std::string const& working_directory);

    [[nodiscard]] std::string get_todays_date() noexcept;

    [[nodiscard]] bool validate_user_input(std::string update_interval, int update_span, int& update_frequency_seconds);
};

}// namespace fxordermgmt

#endif
