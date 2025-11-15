#pragma once

#include <asio.hpp>
#include <liberror/Try.hpp>

#include <functional>

template <auto Func, class ... Args>
auto make_async(Args&&... args)
{
    return std::bind_front([] (auto&&... args) -> asio::awaitable<decltype(Func(std::decay_t<Args>{}...))> {
        co_return std::invoke(Func, args...);
    }, std::forward<Args>(args)...);
}
