// Copyright 2024, Andrew Drogalis
// GNU License

#ifndef GAIN_CAPITAL_EXCEPTION_H
#define GAIN_CAPITAL_EXCEPTION_H

#include <stdexcept>// for runtime_error
#include <string>   // for basic_string

namespace gaincapital
{

class GCException : public std::runtime_error
{
  public:
    std::string message;
    std::string func_name;

    GCException() = delete;

    GCException(std::string const& func_name, std::string const& message);

    char const* what() const noexcept override;

    char const* where() const noexcept;
};

}// namespace gaincapital

#endif
