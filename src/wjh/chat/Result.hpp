// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_CHAT_7459B64EB09F40359CC9861E95F5F917
#define WJH_CHAT_7459B64EB09F40359CC9861E95F5F917

#include <tl/expected.hpp>

#include <cstdio>
#include <format>
#include <string>

namespace wjh::chat {

/**
 * Result type for error handling.
 *
 * Uses tl::expected which provides std::expected-like semantics for C++20.
 */
template <typename T>
using Result = tl::expected<T, std::string>;

namespace detail {
struct MakeError
{
    [[nodiscard]]
    tl::unexpected<std::string>
    operator () (std::string msg) const
    {
        return tl::unexpected<std::string>(std::move(msg));
    }

    template <typename... ArgTs>
    [[nodiscard]]
    tl::unexpected<std::string>
    operator () (std::format_string<ArgTs...> fmt, ArgTs &&... args) const
    {
        return tl::unexpected<std::string>(
            std::format(std::move(fmt), std::forward<ArgTs>(args)...));
    }
};
} // namespace detail

/**
 * Helper to create error results
 *
 * Usage:
 *   return make_error("message");
 *   return make_error("Can't open file '{}'", path);
 */
inline constexpr auto make_error = detail::MakeError{};

} // namespace wjh::chat

#endif // WJH_CHAT_7459B64EB09F40359CC9861E95F5F917
