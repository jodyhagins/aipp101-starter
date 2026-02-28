// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "MockClient.hpp"

namespace testing {

MockClient::
~MockClient() = default;

wjh::chat::Result<wjh::chat::ChatResponse>
MockClient::
do_send_message(wjh::chat::conversation::Conversation const & conversation)
{
    ++call_count_;

    // Store a copy for inspection
    last_conversation_ =
        std::make_unique<wjh::chat::conversation::Conversation>(conversation);

    // Return next queued result
    if (results_.empty()) {
        return wjh::chat::make_error("MockClient: No result queued");
    }

    auto result = std::move(results_.front());
    results_.pop();
    return result;
}

} // namespace testing
