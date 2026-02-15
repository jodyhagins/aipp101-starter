// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_CHAT_DC3A9C528451407085E361D21FBA4344
#define WJH_CHAT_DC3A9C528451407085E361D21FBA4344

#include <cstdio>
#include <format>

namespace wjh::chat {

inline constexpr auto print = []<typename... ArgTs>(
                                  std::FILE * fp,
                                  std::format_string<ArgTs...> fmt,
                                  ArgTs &&... args) {
    std::fputs(std::format(fmt, std::forward<ArgTs>(args)...).c_str(), fp);
};

} // namespace wjh::chat

#endif // WJH_CHAT_DC3A9C528451407085E361D21FBA4344
