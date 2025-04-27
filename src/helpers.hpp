#include <utility>
#include "helpers_vulkan.hpp"
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

}
