// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_ASSERTS_RETURN_VALUES
#include "wjh/chat/conversation/Conversation.hpp"

#include "testing/doctest.hpp"

namespace {
using namespace wjh::chat;
using namespace wjh::chat::conversation;

TEST_SUITE("Conversation")
{
    TEST_CASE("Empty conversation")
    {
        Conversation conv;

        CHECK(conv.empty());
        CHECK(conv.size() == 0);
    }

    TEST_CASE("Add messages")
    {
        Conversation conv;

        conv.add_message(UserInput{"Hello"});
        CHECK_FALSE(conv.empty());
        CHECK(conv.size() == 1);

        conv.add_message(AssistantResponse{"Hi there"});
        CHECK(conv.size() == 2);
    }

    TEST_CASE("Add pre-built message")
    {
        Conversation conv;
        auto msg = Message::user(UserInput{"Direct add"});
        conv.add_message(std::move(msg));

        CHECK(conv.size() == 1);
        CHECK(conv.messages()[0].role() == Role::user);
    }

    TEST_CASE("Clear conversation")
    {
        Conversation conv;
        conv.add_message(UserInput{"Hello"});
        conv.add_message(AssistantResponse{"Hi"});

        conv.clear();

        CHECK(conv.empty());
        CHECK(conv.size() == 0);
    }

    TEST_CASE("System prompt")
    {
        Conversation conv;

        CHECK_FALSE(conv.system_prompt().has_value());

        conv.set_system_prompt(SystemPrompt{"You are helpful"});
        REQUIRE(conv.system_prompt().has_value());
        CHECK(*conv.system_prompt() == SystemPrompt{"You are helpful"});

        conv.clear_system_prompt();
        CHECK_FALSE(conv.system_prompt().has_value());
    }

    TEST_CASE("Serialize to JSON")
    {
        Conversation conv;
        conv.add_message(UserInput{"Hello"});
        conv.add_message(AssistantResponse{"Hi there"});

        auto json = conv.to_json();

        REQUIRE(json.is_array());
        REQUIRE(json.size() == 2);

        CHECK(json[0]["role"] == "user");
        CHECK(json[0]["content"] == "Hello");
        CHECK(json[1]["role"] == "assistant");
        CHECK(json[1]["content"] == "Hi there");
    }
}

} // anonymous namespace
