// Copyright 2024, Andrew Drogalis
// GNU License

#ifndef FX_UTILITIES_H
#define FX_UTILITIES_H

#include <expected>// for expected
#include <string>  // for basic_string

#include "fx_exception.h"// for FXException

namespace fxordermgmt
{

class FXUtilities
{

  public:
    FXUtilities() noexcept = default;

    [[nodiscard]] std::expected<std::string, FXException> keyring_unlock_get_password(std::string const& account_type, std::string const& username);

    [[nodiscard]] std::expected<bool, FXException> validate_user_settings(std::string& update_interval, int update_span,
                                                                          int& update_frequency_seconds);

    static std::expected<bool, FXException> initialize_logging_file(std::string const& working_directory);

    static void log_to_std_output();

    [[nodiscard]] std::string get_todays_date() noexcept;
};

}// namespace fxordermgmt

#endif
