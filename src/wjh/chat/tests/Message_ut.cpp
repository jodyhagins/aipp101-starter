// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_ASSERTS_RETURN_VALUES
#include "wjh/chat/conversation/Message.hpp"

#include "testing/doctest.hpp"

namespace {
using namespace wjh::chat;
using namespace wjh::chat::conversation;

TEST_SUITE("Message")
{
    TEST_CASE("Create user message")
    {
        auto msg = Message::user(UserInput{"Hello"});

        CHECK(msg.role() == Role::user);
        CHECK(msg.text() == MessageText{"Hello"});
    }

    TEST_CASE("Create assistant message")
    {
        auto msg = Message::assistant(AssistantResponse{"Hi there"});

        CHECK(msg.role() == Role::assistant);
        CHECK(msg.text() == MessageText{"Hi there"});
    }

    TEST_CASE("Serialize user message to JSON")
    {
        auto msg = Message::user(UserInput{"Hello"});
        auto json = to_json(msg);

        CHECK(json["role"] == "user");
        CHECK(json["content"] == "Hello");
    }

    TEST_CASE("Serialize assistant message to JSON")
    {
        auto msg = Message::assistant(AssistantResponse{"Hi there"});
        auto json = to_json(msg);

        CHECK(json["role"] == "assistant");
        CHECK(json["content"] == "Hi there");
    }

    TEST_CASE("Parse message from JSON")
    {
        nlohmann::json json = {{"role", "user"}, {"content", "Hello"}};
        auto msg = parse_message(json);

        CHECK(msg.role() == Role::user);
        CHECK(msg.text() == MessageText{"Hello"});
    }

    TEST_CASE("Round-trip: serialize then parse")
    {
        auto original = Message::user(UserInput{"Round trip test"});
        auto json = to_json(original);
        auto parsed = parse_message(json);

        CHECK(parsed.role() == original.role());
        CHECK(parsed.text() == original.text());
    }
}

} // anonymous namespace
