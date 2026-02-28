// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_CHAT_5FD39B99AAA445C9B2ADEF44D94D2866
#define WJH_CHAT_5FD39B99AAA445C9B2ADEF44D94D2866

#include "wjh/chat/Result.hpp"
#include "wjh/chat/TokenUsage.hpp"
#include "wjh/chat/types.hpp"
#include "wjh/chat/conversation/Conversation.hpp"

namespace wjh::chat::client {

/**
 * Abstract interface for LLM API clients.
 *
 * This interface allows for dependency injection and mocking in tests.
 *
 * This interface uses the Non-Virtual Interface (NVI) pattern. Derived
 * classes must override the private virtual do_send_message function.
 */
class IClient
{
public:
    virtual ~IClient();

    /**
     * Send a conversation and get a response.
     * @param conversation The conversation history
     * @return Chat response with text and optional usage, or error
     */
    [[nodiscard]]
    Result<ChatResponse> send_message(
        conversation::Conversation const & conversation)
    {
        return do_send_message(conversation);
    }

private:
    virtual Result<ChatResponse> do_send_message(
        conversation::Conversation const & conversation) = 0;
};

} // namespace wjh::chat::client

#endif // WJH_CHAT_5FD39B99AAA445C9B2ADEF44D94D2866
