// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_CHAT_A7B3C9D1E5F6482394AD8E1F2C3B4A56
#define WJH_CHAT_A7B3C9D1E5F6482394AD8E1F2C3B4A56

#include "wjh/chat/types.hpp"

#include <optional>

namespace wjh::chat {

/**
 * Token usage statistics from a single API response.
 */
struct TokenUsage
{
    PromptTokens prompt_tokens{};
    CompletionTokens completion_tokens{};
    TotalTokens total_tokens{};
};

/**
 * Full response from the LLM client.
 *
 * Bundles the assistant's text with optional token usage
 * statistics (not all providers return usage data).
 */
struct ChatResponse
{
    AssistantResponse response;
    std::optional<TokenUsage> usage;
};

} // namespace wjh::chat

#endif // WJH_CHAT_A7B3C9D1E5F6482394AD8E1F2C3B4A56
