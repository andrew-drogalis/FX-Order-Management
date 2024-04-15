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
    FXUtilities() noexcept = default;

    [[nodiscard]] bool setup_password_first_time(const std::string& account_type, const std::string& username);

    static void init_logging(const std::string& working_directory);

    [[nodiscard]] std::string get_todays_date() noexcept;

    [[nodiscard]] bool validate_user_interval(std::string update_interval, int update_span, int& update_frequency_seconds);
};

}// namespace fxordermgmt

#endif
