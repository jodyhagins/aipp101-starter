// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_CHAT_C4D5E6F7A8B9401CABCDEF1234567890
#define WJH_CHAT_C4D5E6F7A8B9401CABCDEF1234567890

#include "wjh/chat/CommandLine.hpp"
#include "wjh/chat/Result.hpp"
#include "wjh/chat/types.hpp"

#include <optional>
#include <ostream>

namespace wjh::chat {

/**
 * Resolved application configuration.
 */
struct Config
{
    ApiKey api_key;
    ModelId model;
    MaxTokens max_tokens;
    std::optional<SystemPrompt> system_prompt;
    ShowConfig show_config;
};

/**
 * Load .env files in precedence order.
 *
 * Files loaded (highest to lowest precedence):
 *   1. .env.local (gitignored, local overrides)
 *   2. .env (project config)
 *   3. ~/.config/aipp101_chat/.env (user preferences)
 */
void load_env_files();

/**
 * Resolve configuration from environment + CLI args.
 *
 * Resolution order (highest to lowest precedence):
 *   1. CLI arguments
 *   2. Environment variables (from .env files)
 *   3. Built-in defaults
 *
 * @return Config or error (e.g., missing OPENROUTER_API_KEY)
 */
[[nodiscard]]
Result<Config> resolve_config(CommandLineArgs const & args);

/**
 * Print the resolved configuration.
 */
void print_config(Config const & config, std::ostream & out);

} // namespace wjh::chat

#endif // WJH_CHAT_C4D5E6F7A8B9401CABCDEF1234567890
