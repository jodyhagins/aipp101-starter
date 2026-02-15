// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "wjh/chat/conversation/Message.hpp"

#include "wjh/chat/json_convert.hpp"

namespace wjh::chat::conversation {

Message
Message::
user(UserInput input)
{
    // Type boundary: converting UserInput → MessageText
    return Message{Role::user, MessageText{atlas::undress(std::move(input))}};
}

Message
Message::
assistant(AssistantResponse response)
{
    // Type boundary: converting AssistantResponse → MessageText
    return Message{
        Role::assistant,
        MessageText{atlas::undress(std::move(response))}};
}

nlohmann::json
to_json(Message const & msg)
{
    return {
        {"role", json_value(msg.role())},
        {"content", json_value(msg.text())}};
}

Message
parse_message(nlohmann::json const & json)
{
    return Message{
        Role(json.at("role").get<std::string>()),
        MessageText{json.at("content").get<std::string>()}};
}

} // namespace wjh::chat::conversation
