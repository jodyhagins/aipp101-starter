// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_ASSERTS_RETURN_VALUES
#include "wjh/chat/client/OpenRouterClient.hpp"

#include "wjh/chat/conversation/Conversation.hpp"

#include "testing/doctest.hpp"

namespace {
using namespace wjh::chat;
using namespace wjh::chat::client;
using namespace wjh::chat::conversation;

OpenRouterClientConfig
makeTestConfig()
{
    return OpenRouterClientConfig{
        .api_key = ApiKey("test-api-key"),
        .model = ModelId("openai/gpt-4"),
        .max_tokens = MaxTokens(4096u),
        .system_prompt = SystemPrompt{"Test system prompt"},
        .temperature = std::nullopt};
}

TEST_SUITE("OpenRouterClient")
{
    TEST_CASE("Client configuration")
    {
        SUBCASE("Basic configuration") {
            auto config = makeTestConfig();
            OpenRouterClient client(std::move(config));

            CHECK(client.model() == ModelId("openai/gpt-4"));
        }

        SUBCASE("Without system prompt") {
            OpenRouterClientConfig config{
                .api_key = ApiKey("test-key"),
                .model = ModelId("meta-llama/llama-3-70b-instruct"),
                .max_tokens = MaxTokens(2048u),
                .system_prompt = std::nullopt,
                .temperature = std::nullopt};

            OpenRouterClient client(std::move(config));
            CHECK(client.model() == ModelId("meta-llama/llama-3-70b-instruct"));
        }

        SUBCASE("With temperature") {
            OpenRouterClientConfig config{
                .api_key = ApiKey("test-key"),
                .model = ModelId("openai/gpt-4"),
                .max_tokens = MaxTokens(4096u),
                .system_prompt = std::nullopt,
                .temperature = Temperature{0.7f}};

            OpenRouterClient client(std::move(config));
            CHECK(client.model() == ModelId("openai/gpt-4"));
        }
    }

    TEST_CASE("Message conversion scenarios")
    {
        SUBCASE("Empty conversation") {
            OpenRouterClient client(makeTestConfig());
            Conversation conversation;
            CHECK(conversation.empty());
        }

        SUBCASE("Simple text message") {
            OpenRouterClient client(makeTestConfig());
            Conversation conversation;
            conversation.add_message(UserInput{"Hello, world!"});

            CHECK(conversation.size() == 1);
        }

        SUBCASE("Multi-turn conversation") {
            Conversation conversation;
            conversation.add_message(UserInput{"Hello"});
            conversation.add_message(AssistantResponse{"Hi there!"});
            conversation.add_message(UserInput{"How are you?"});

            CHECK(conversation.size() == 3);
        }
    }
}

} // anonymous namespace
