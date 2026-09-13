// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_CHAT_D89DA1D4ECD4490E9036E1CC76F6DEEF
#define WJH_CHAT_D89DA1D4ECD4490E9036E1CC76F6DEEF

#include "wjh/chat/client/IClient.hpp"

#include <queue>

namespace testing {

/**
 * Mock client for testing without making real API calls.
 *
 * Usage:
 *   MockClient mock;
 *   mock.queue_response(AssistantResponse{"Hello!"});
 *   mock.queue_error("Network timeout");
 *   // Use mock as IClient...
 */
class MockClient
: public wjh::chat::client::IClient
{
public:
    ~MockClient() override;

    /**
     * Queue a successful response.
     */
    void queue_response(wjh::chat::AssistantResponse response)
    {
        results_.push(std::move(response));
    }

    /**
     * Queue an error result.
     */
    void queue_error(std::string error)
    {
        results_.push(tl::make_unexpected(std::move(error)));
    }

    /**
     * Get the last conversation that was sent.
     */
    [[nodiscard]]
    wjh::chat::conversation::Conversation const * last_conversation() const
    {
        return last_conversation_.get();
    }

    /**
     * Get the number of times send_message was called.
     */
    [[nodiscard]]
    std::size_t call_count() const
    {
        return call_count_;
    }

private:
    wjh::chat::Result<wjh::chat::AssistantResponse> do_send_message(
        wjh::chat::conversation::Conversation const & conversation) override;

    std::queue<wjh::chat::Result<wjh::chat::AssistantResponse>> results_;
    std::unique_ptr<wjh::chat::conversation::Conversation> last_conversation_;
    std::size_t call_count_ = 0;
};

} // namespace testing

#endif // WJH_CHAT_D89DA1D4ECD4490E9036E1CC76F6DEEF
