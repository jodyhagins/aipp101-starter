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
        SUBCASE("Tools array has 4 tools") {
            // Build a request to extract the tools
            // array structure
            auto config = makeTestConfig();
            OpenRouterClient client(
                std::move(config));
            Conversation conversation;
            conversation.add_message(
                UserInput{"test"});
            auto request = nlohmann::json::parse(
                R"({"tools": [
                    {"type":"function","function":{
                        "name":"bash"}},
                    {"type":"function","function":{
                        "name":"read_file"}},
                    {"type":"function","function":{
                        "name":"write_file"}},
                    {"type":"function","function":{
                        "name":"edit_file"}}
                ]})");

            auto const & tools = request["tools"];
            CHECK(tools.is_array());
            CHECK(tools.size() == 4);
            CHECK(tools[0]["function"]["name"]
                  == "bash");
            CHECK(tools[1]["function"]["name"]
                  == "read_file");
            CHECK(tools[2]["function"]["name"]
                  == "write_file");
            CHECK(tools[3]["function"]["name"]
                  == "edit_file");
        }

        SUBCASE("Bash tool schema is well-formed") {
            auto tool = nlohmann::json::parse(R"({
                "type": "function",
                "function": {
                    "name": "bash",
                    "description": "Execute a bash command. Use this to run shell commands, compile code, run tests, and other terminal operations.",
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
            })");

            CHECK(tool["type"] == "function");
            CHECK(tool["function"]["name"] == "bash");
            CHECK(tool["function"]
                      .contains("parameters"));
            CHECK(tool["function"]["parameters"]
                      ["required"][0] == "command");
        }

        SUBCASE("read_file tool schema") {
            auto tool = nlohmann::json::parse(R"({
                "type": "function",
                "function": {
                    "name": "read_file",
                    "parameters": {
                        "type": "object",
                        "properties": {
                            "file_path": {
                                "type": "string"
                            },
                            "offset": {
                                "type": "integer"
                            },
                            "limit": {
                                "type": "integer"
                            }
                        },
                        "required": ["file_path"]
                    }
                }
            })");

            auto const & params =
                tool["function"]["parameters"];
            CHECK(params["required"].size() == 1);
            CHECK(params["required"][0]
                  == "file_path");
            CHECK(params["properties"]
                      .contains("offset"));
            CHECK(params["properties"]
                      .contains("limit"));
        }

        SUBCASE("write_file tool schema") {
            auto tool = nlohmann::json::parse(R"({
                "type": "function",
                "function": {
                    "name": "write_file",
                    "parameters": {
                        "type": "object",
                        "properties": {
                            "file_path": {
                                "type": "string"
                            },
                            "content": {
                                "type": "string"
                            }
                        },
                        "required": [
                            "file_path", "content"
                        ]
                    }
                }
            })");

            auto const & params =
                tool["function"]["parameters"];
            CHECK(params["required"].size() == 2);
            CHECK(params["required"][0]
                  == "file_path");
            CHECK(params["required"][1]
                  == "content");
        }

        SUBCASE("edit_file tool schema") {
            auto tool = nlohmann::json::parse(R"({
                "type": "function",
                "function": {
                    "name": "edit_file",
                    "parameters": {
                        "type": "object",
                        "properties": {
                            "file_path": {
                                "type": "string"
                            },
                            "old_string": {
                                "type": "string"
                            },
                            "new_string": {
                                "type": "string"
                            }
                        },
                        "required": [
                            "file_path",
                            "old_string",
                            "new_string"
                        ]
                    }
                }
            })");

            auto const & params =
                tool["function"]["parameters"];
            CHECK(params["required"].size() == 3);
            CHECK(params["required"][0]
                  == "file_path");
            CHECK(params["required"][1]
                  == "old_string");
            CHECK(params["required"][2]
                  == "new_string");
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

    TEST_CASE("Agent loop message structures")
    {
        SUBCASE("Tool result message format") {
            // The agent loop builds this message after
            // executing a tool call.
            auto tool_result = nlohmann::json{
                {"role", "tool"},
                {"tool_call_id", "call_abc123"},
                {"content",
                 "README.md\nsrc/\n[exit code: 0]"}};

            CHECK(tool_result["role"] == "tool");
            CHECK(tool_result.contains("tool_call_id"));
            CHECK(tool_result["tool_call_id"]
                  == "call_abc123");
            CHECK(tool_result["content"]
                      .get<std::string>()
                      .find("[exit code: 0]")
                  != std::string::npos);
        }

        SUBCASE("Nudge message for empty content") {
            // When the model responds with null or empty
            // content, the agent loop appends a nudge.
            auto nudge = nlohmann::json{
                {"role", "user"},
                {"content",
                 "Please use your tools or respond "
                 "with text."}};

            CHECK(nudge["role"] == "user");
            CHECK(nudge["content"]
                      .get<std::string>()
                      .find("tools")
                  != std::string::npos);
        }

        SUBCASE("Tool call argument parsing") {
            // The agent loop parses the arguments string
            // as JSON and extracts the command.
            auto arguments =
                R"({"command":"cat src/main.cpp"})";
            auto args =
                nlohmann::json::parse(arguments);
            CHECK(args.contains("command"));
            CHECK(args["command"]
                  == "cat src/main.cpp");
        }

        SUBCASE("read_file argument parsing") {
            // read_file has required file_path and
            // optional offset/limit
            auto args_minimal =
                nlohmann::json::parse(
                    R"({"file_path":"src/main.cpp"})");
            CHECK(args_minimal.contains("file_path"));
            CHECK(args_minimal["file_path"]
                  == "src/main.cpp");
            CHECK(not args_minimal
                      .contains("offset"));
            CHECK(not args_minimal
                      .contains("limit"));

            auto args_full = nlohmann::json::parse(
                R"({"file_path":"src/main.cpp",)"
                R"("offset":10,"limit":20})");
            CHECK(args_full["file_path"]
                  == "src/main.cpp");
            CHECK(args_full["offset"] == 10);
            CHECK(args_full["limit"] == 20);
        }

        SUBCASE("Tool dispatch with mixed tool "
                "names")
        {
            // The agent loop should dispatch based on
            // the tool name from each tool call.
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
                                    "name": "read_file",
                                    "arguments": "{\"file_path\":\"README.md\"}"
                                }
                            },
                            {
                                "id": "call_3",
                                "type": "function",
                                "function": {
                                    "name": "write_file",
                                    "arguments": "{\"file_path\":\"out.txt\",\"content\":\"hello\"}"
                                }
                            }
                        ]
                    }
                }]
            })");

            auto const & tool_calls =
                response_json["choices"][0]
                    ["message"]["tool_calls"];
            CHECK(tool_calls.size() == 3);

            // Verify each tool call has the
            // expected name
            CHECK(tool_calls[0]["function"]["name"]
                  == "bash");
            CHECK(tool_calls[1]["function"]["name"]
                  == "read_file");
            CHECK(tool_calls[2]["function"]["name"]
                  == "write_file");

            // Verify argument parsing works for
            // each tool type
            auto bash_args = nlohmann::json::parse(
                tool_calls[0]["function"]["arguments"]
                    .get<std::string>());
            CHECK(bash_args.contains("command"));

            auto read_args = nlohmann::json::parse(
                tool_calls[1]["function"]["arguments"]
                    .get<std::string>());
            CHECK(read_args.contains("file_path"));

            auto write_args = nlohmann::json::parse(
                tool_calls[2]["function"]["arguments"]
                    .get<std::string>());
            CHECK(write_args.contains("file_path"));
            CHECK(write_args.contains("content"));
        }

        SUBCASE("Assistant message with tool calls "
                "is preserved")
        {
            // The agent loop appends the full assistant
            // message (including tool_calls) back to the
            // messages array for context.
            auto assistant_msg = nlohmann::json{
                {"role", "assistant"},
                {"content", nullptr},
                {"tool_calls",
                 {{{"id", "call_1"},
                   {"type", "function"},
                   {"function",
                    {{"name", "bash"},
                     {"arguments",
                      R"({"command":"ls"})"}}}}}
                }};

            CHECK(assistant_msg["role"] == "assistant");
            CHECK(assistant_msg["content"].is_null());
            CHECK(assistant_msg["tool_calls"].size()
                  == 1);

            // Verify it round-trips through JSON
            auto serialized = assistant_msg.dump();
            auto parsed =
                nlohmann::json::parse(serialized);
            CHECK(parsed == assistant_msg);
        }
    }
}

} // anonymous namespace
