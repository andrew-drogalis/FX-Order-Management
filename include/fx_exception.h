// Copyright 2024, Andrew Drogalis
// GNU License

#ifndef FX_EXCEPTION_H
#define FX_EXCEPTION_H

#include <stdexcept>// for runtime_error
#include <string>   // for basic_string

namespace fxordermgmt
{

class FXException : public std::runtime_error
{
  public:
    std::string message;
    std::string func_name;

    FXException() = delete;

    FXException(std::string const& func_name, std::string const& message);

    char const* what() const noexcept override;

    char const* where() const noexcept;
};

}// namespace fxordermgmt

#endif
