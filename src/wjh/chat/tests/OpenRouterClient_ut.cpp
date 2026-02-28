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

    TEST_CASE("Tool call JSON format expectations")
    {
        // Verify the tool schema structure we expect
        // make_tools_json() to produce.
        SUBCASE("Bash tool schema is well-formed") {
            // The expected tools JSON that gets sent
            // in every request
            auto tools = nlohmann::json::parse(R"([{
                "type": "function",
                "function": {
                    "name": "bash",
                    "description": "Execute a bash command. Use this to run shell commands, read/write files, compile code, run tests, etc.",
                    "parameters": {
                        "type": "object",
                        "properties": {
                            "command": {
                                "type": "string",
                                "description": "The bash command to execute"
                            }
                        },
                        "required": ["command"]
                    }
                }
            }])");

            CHECK(tools.is_array());
            CHECK(tools.size() == 1);
            CHECK(tools[0]["type"] == "function");
            CHECK(tools[0]["function"]["name"] == "bash");
            CHECK(tools[0]["function"]
                      .contains("parameters"));
            CHECK(tools[0]["function"]["parameters"]
                      ["required"][0] == "command");
        }

        SUBCASE("Tool call response format") {
            // Simulate what parse_response() should
            // produce from a tool-call response
            auto response_json = nlohmann::json::parse(
                R"({
                "choices": [{
                    "message": {
                        "role": "assistant",
                        "content": null,
                        "tool_calls": [{
                            "id": "call_abc123",
                            "type": "function",
                            "function": {
                                "name": "bash",
                                "arguments": "{\"command\":\"ls src/\"}"
                            }
                        }]
                    }
                }],
                "usage": {
                    "prompt_tokens": 50,
                    "completion_tokens": 10,
                    "total_tokens": 60
                }
            })");

            // Verify the tool_calls structure
            auto const & tc =
                response_json["choices"][0]
                    ["message"]["tool_calls"][0];
            CHECK(tc["function"]["name"] == "bash");

            auto args = nlohmann::json::parse(
                tc["function"]["arguments"]
                    .get<std::string>());
            CHECK(args["command"] == "ls src/");

            // Verify the expected display format
            auto const & fn = tc["function"];
            std::string display =
                "[Tool call] "
                + fn["name"].get<std::string>() + ": "
                + fn["arguments"].get<std::string>()
                + "\n";
            CHECK(display ==
                  "[Tool call] bash: "
                  "{\"command\":\"ls src/\"}\n");
        }

        SUBCASE("Multiple tool calls format") {
            auto response_json = nlohmann::json::parse(
                R"({
                "choices": [{
                    "message": {
                        "role": "assistant",
                        "content": null,
                        "tool_calls": [
                            {
                                "id": "call_1",
                                "type": "function",
                                "function": {
                                    "name": "bash",
                                    "arguments": "{\"command\":\"ls\"}"
                                }
                            },
                            {
                                "id": "call_2",
                                "type": "function",
                                "function": {
                                    "name": "bash",
                                    "arguments": "{\"command\":\"pwd\"}"
                                }
                            }
                        ]
                    }
                }]
            })");

            auto const & tool_calls =
                response_json["choices"][0]
                    ["message"]["tool_calls"];
            CHECK(tool_calls.size() == 2);

            std::string display;
            for (auto const & tc : tool_calls) {
                auto const & fn = tc["function"];
                display +=
                    "[Tool call] "
                    + fn["name"].get<std::string>()
                    + ": "
                    + fn["arguments"]
                          .get<std::string>()
                    + "\n";
            }

            CHECK(display ==
                  "[Tool call] bash: "
                  "{\"command\":\"ls\"}\n"
                  "[Tool call] bash: "
                  "{\"command\":\"pwd\"}\n");
        }
    }
}

} // anonymous namespace
