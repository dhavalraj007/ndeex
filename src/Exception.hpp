#pragma once
#include <format>
#include <print>
#include <stdexcept>

namespace Core
{
// adds a layer to accept formatted string
struct runtime_error : public std::runtime_error
{
    template <class... Types>
    explicit runtime_error(std::string_view rtfmt, Types &&...args)
        : std::runtime_error{std::vformat(rtfmt, std::make_format_args(args...))}
    {
    }
};
} // namespace Core