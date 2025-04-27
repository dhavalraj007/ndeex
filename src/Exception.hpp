#pragma once
#include <stdexcept>
#include <print>
#include <format>
#include <stacktrace>
namespace Core
{
    // adds a layer to accept formatted string
    struct runtime_error : public std::runtime_error
    {
        template <class... Types>
        explicit runtime_error(std::string_view rtfmt, Types &&...args) : std::runtime_error{std::vformat(std::string(rtfmt) + std::string(" \n\n current stack trace is:\n") + std::to_string(std::stacktrace::current()), std::make_format_args(args...))}
        {
        }
    };
}