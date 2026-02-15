// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "wjh/chat/conversation/Conversation.hpp"

namespace wjh::chat::conversation {

void
Conversation::
add_message(Message msg)
{
    messages_.push_back(std::move(msg));
}

void
Conversation::
add_message(UserInput text)
{
    add_message(Message::user(std::move(text)));
}

void
Conversation::
add_message(AssistantResponse text)
{
    add_message(Message::assistant(std::move(text)));
}

nlohmann::json
Conversation::
to_json() const
{
    auto result = nlohmann::json::array();
    for (auto const & msg : messages_) {
        result.push_back(conversation::to_json(msg));
    }
    return result;
}

} // namespace wjh::chat::conversation
