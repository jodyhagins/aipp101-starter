// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_CHAT_F6ECA88581C6415AB5A8A5194B14F202
#define WJH_CHAT_F6ECA88581C6415AB5A8A5194B14F202

#include "wjh/chat/conversation/Message.hpp"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <vector>

namespace wjh::chat::conversation {

/**
 * Manages conversation history between user and assistant.
 *
 * The conversation maintains message history that can be serialized
 * to JSON for API requests. Since the LLM API is stateless,
 * all messages must be sent with each request.
 *
 * Convenience overloads of add_message() accept domain types directly
 * -- the strong type of the argument selects the correct overload at
 * compile time (UserInput vs AssistantResponse).
 */
class Conversation
{
public:
    /**
     * Add a pre-built message to the conversation.
     */
    void add_message(Message msg);

    /**
     * Add a user text message.
     * Overload selected by UserInput type.
     */
    void add_message(UserInput text);

    /**
     * Add an assistant text message.
     * Overload selected by AssistantResponse type.
     */
    void add_message(AssistantResponse text);

    /**
     * Get all messages.
     */
    [[nodiscard]]
    std::vector<Message> const & messages() const
    {
        return messages_;
    }

    /**
     * Check if conversation is empty.
     */
    [[nodiscard]]
    bool empty() const
    {
        return messages_.empty();
    }

    /**
     * Get number of messages.
     */
    [[nodiscard]]
    std::size_t size() const
    {
        return messages_.size();
    }

    /**
     * Clear all messages.
     */
    void clear() { messages_.clear(); }

    /**
     * Remove the last message (e.g., on send failure).
     */
    void pop_back()
    {
        if (not messages_.empty()) {
            messages_.pop_back();
        }
    }

    /**
     * Convert messages to JSON array for API.
     */
    [[nodiscard]]
    nlohmann::json to_json() const;

    /**
     * Get the system prompt.
     */
    [[nodiscard]]
    std::optional<SystemPrompt> const & system_prompt() const
    {
        return system_prompt_;
    }

    /**
     * Set the system prompt.
     */
    void set_system_prompt(SystemPrompt prompt)
    {
        system_prompt_ = std::move(prompt);
    }

    /**
     * Clear the system prompt.
     */
    void clear_system_prompt() { system_prompt_.reset(); }

private:
    std::vector<Message> messages_;
    std::optional<SystemPrompt> system_prompt_;
};

} // namespace wjh::chat::conversation

#endif // WJH_CHAT_F6ECA88581C6415AB5A8A5194B14F202
