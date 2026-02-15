// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_CHAT_64D97F5A46A04F34AB04BFAFB1E77796
#define WJH_CHAT_64D97F5A46A04F34AB04BFAFB1E77796

#include "wjh/chat/types.hpp"

#include <type_traits>

namespace wjh::chat {

/// Convert a value for JSON serialization, unwrapping Atlas strong
/// types to their underlying representation.
///
/// Uses perfect forwarding to avoid unnecessary copies.
template <typename T>
constexpr decltype(auto)
json_value(T && v)
{
    if constexpr (atlas::AtlasTypeC<std::remove_cvref_t<T>>) {
        return atlas::undress(std::forward<T>(v));
    } else {
        return std::forward<T>(v);
    }
}

} // namespace wjh::chat

#endif // WJH_CHAT_64D97F5A46A04F34AB04BFAFB1E77796
