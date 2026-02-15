// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_CHAT_B3A4C5D6E7F84890AB12CD34EF567890
#define WJH_CHAT_B3A4C5D6E7F84890AB12CD34EF567890

#include "wjh/chat/Result.hpp"
#include "wjh/chat/types.hpp"

#include <optional>
#include <span>
#include <string>

namespace wjh::chat {

/**
 * Parsed command-line arguments.
 *
 * All optional fields use the same strong types as Config,
 * preventing accidental field swaps at the call site.
 */
struct CommandLineArgs
{
    std::optional<ModelId> model;
    std::optional<SystemPrompt> system_prompt;
    std::optional<MaxTokens> max_tokens;
    ShowConfig show_config;
    ShowHelp help;
};

/**
 * Parse command-line arguments.
 *
 * Supported flags:
 *   -m, --model <id>          Model ID override
 *   -s, --system-prompt <text> System prompt
 *   -t, --max-tokens <n>      Max response tokens
 *   --show-config              Display resolved config and exit
 *   -h, --help                 Show help
 */
[[nodiscard]]
Result<CommandLineArgs> parse_args(std::span<char const * const> args);

/**
 * Generate help text for the program.
 */
[[nodiscard]]
HelpText help_text(ProgramName const & program_name);

} // namespace wjh::chat

#endif // WJH_CHAT_B3A4C5D6E7F84890AB12CD34EF567890
