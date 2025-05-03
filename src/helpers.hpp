#pragma once
#include <utility>
#include "helpers_vulkan.hpp"
#include "Exception.hpp"
namespace helpers
{
    namespace detail
    {
        template <typename T, size_t size, typename F, size_t... Is>
        auto initArr_impl(F &&f, std::index_sequence<Is...>)
        {
            return std::array<T, size>{f.operator()<Is>()...};
        }
    }

    template <typename T, size_t size, typename F>
    auto initArr(F &&f)
    {
        return detail::initArr_impl<T, size>(std::forward<F>(f), std::make_index_sequence<size>{});
    }

    template <typename T>
    void print_type_name()
    {
        static_assert(sizeof(T) == 0, "Type is:");
    }

    namespace detail
    {
        template <typename T>
            requires std::is_convertible_v<T &&, bool>
        decltype(auto) SDL_CHECKTHROW_impl(const char *expressionStr, T &&expression)
        {
            if (!expression)
            {
                throw Core::runtime_error("SDL function :{} failed! SDL_GetError():{}", expressionStr, SDL_GetError());
            }
            return std::forward<T>(expression);
        }
    }

}
#define SDL_CHECKTHROW(expression) helpers::detail::SDL_CHECKTHROW_impl(#expression, expression)
