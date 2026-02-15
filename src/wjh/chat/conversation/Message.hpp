// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_CHAT_9F866B9E67364964BC5F0C4BDD2F63F6
#define WJH_CHAT_9F866B9E67364964BC5F0C4BDD2F63F6

#include "wjh/chat/types.hpp"
#include "wjh/chat/conversation/types.hpp"

#include <nlohmann/json.hpp>

#include <string>

namespace wjh::chat::conversation {

/**
 * A message in the conversation.
 *
 * Simplified for the starter kit: just role + text, no content blocks.
 *
 * Construction is restricted to factory methods and parse_message()
 * to prevent creation of semantically invalid messages.
 */
class Message
{
public:
    /**
     * Create a user message.
     */
    [[nodiscard]]
    static Message user(UserInput input);

    /**
     * Create an assistant message.
     */
    [[nodiscard]]
    static Message assistant(AssistantResponse response);

    [[nodiscard]]
    Role const & role() const
    {
        return role_;
    }

    [[nodiscard]]
    MessageText const & text() const
    {
        return text_;
    }

    /**
     * Equality: two messages are equal if role and text match.
     */
    Message(Message const &) = default;
    Message(Message &&) noexcept = default;
    Message & operator = (Message const &) = default;
    Message & operator = (Message &&) noexcept = default;

    friend bool operator == (Message const &, Message const &) = default;

private:
    Message(Role r, MessageText t)
    : role_(std::move(r))
    , text_(std::move(t))
    { }

    Role role_;
    MessageText text_;

    friend Message parse_message(nlohmann::json const & json);
};

/**
 * Convert a message to JSON for the API.
 */
[[nodiscard]]
nlohmann::json to_json(Message const & msg);

/**
 * Parse a message from API response JSON.
 */
[[nodiscard]]
Message parse_message(nlohmann::json const & json);

} // namespace wjh::chat::conversation

#endif // WJH_CHAT_9F866B9E67364964BC5F0C4BDD2F63F6
