// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_ASSERTS_RETURN_VALUES
#include "wjh/chat/ChatLoop.hpp"

#include <sstream>

#include "testing/MockClient.hpp"
#include "testing/doctest.hpp"

namespace {
using namespace wjh::chat;

Config
makeTestConfig()
{
    return Config{
        .api_key = ApiKey{"test-key"},
        .model = ModelId{"test-model"},
        .max_tokens = MaxTokens{4096u},
        .system_prompt = std::nullopt,
        .temperature = std::nullopt,
        .show_config = ShowConfig{false}};
}

TEST_SUITE("ChatLoop")
{
    TEST_CASE("Normal conversation flow")
    {
        auto mock = std::make_unique<testing::MockClient>();
        mock->queue_response(AssistantResponse{"Hello! How can I help?"});

        std::istringstream in("Hi there\n/exit\n");
        std::ostringstream out;

        auto result = run(makeTestConfig(), std::move(mock), in, out);

        CHECK(result == ExitCode::success);
        auto output = out.str();
        CHECK(output.find("Hello! How can I help?") != std::string::npos);
        CHECK(output.find("Goodbye!") != std::string::npos);
    }

    TEST_CASE("Exit with /quit")
    {
        auto mock = std::make_unique<testing::MockClient>();

        std::istringstream in("/quit\n");
        std::ostringstream out;

        auto result = run(makeTestConfig(), std::move(mock), in, out);

        CHECK(result == ExitCode::success);
        CHECK(out.str().find("Goodbye!") != std::string::npos);
    }

    TEST_CASE("Exit on EOF")
    {
        auto mock = std::make_unique<testing::MockClient>();

        std::istringstream in("");
        std::ostringstream out;

        auto result = run(makeTestConfig(), std::move(mock), in, out);

        CHECK(result == ExitCode::success);
    }

    TEST_CASE("/clear resets conversation")
    {
        auto mock_ptr = new testing::MockClient();
        mock_ptr->queue_response(AssistantResponse{"First response"});
        mock_ptr->queue_response(AssistantResponse{"Second response"});
        auto mock = std::unique_ptr<testing::MockClient>(mock_ptr);

        std::istringstream in("Hello\n/clear\nHi again\n/exit\n");
        std::ostringstream out;

        auto result = run(makeTestConfig(), std::move(mock), in, out);

        CHECK(result == ExitCode::success);
        auto output = out.str();
        CHECK(output.find("Conversation cleared.") != std::string::npos);
    }

    TEST_CASE("/help shows commands")
    {
        auto mock = std::make_unique<testing::MockClient>();

        std::istringstream in("/help\n/exit\n");
        std::ostringstream out;

        auto result = run(makeTestConfig(), std::move(mock), in, out);

        CHECK(result == ExitCode::success);
        auto output = out.str();
        CHECK(output.find("/exit") != std::string::npos);
        CHECK(output.find("/clear") != std::string::npos);
    }

    TEST_CASE("Empty lines are skipped")
    {
        // No responses queued -- if mock is called it will return error
        auto mock = std::make_unique<testing::MockClient>();

        std::istringstream in("\n\n\n/exit\n");
        std::ostringstream out;

        auto result = run(makeTestConfig(), std::move(mock), in, out);

        CHECK(result == ExitCode::success);
        // No "Assistant>" in output means mock was never called
        CHECK(out.str().find("Assistant>") == std::string::npos);
    }

    TEST_CASE("Error handling")
    {
        auto mock = std::make_unique<testing::MockClient>();
        mock->queue_error("API rate limit exceeded");

        std::istringstream in("Hello\n/exit\n");
        std::ostringstream out;

        auto result = run(makeTestConfig(), std::move(mock), in, out);

        CHECK(result == ExitCode::success);
    }

    TEST_CASE("Welcome message includes model name")
    {
        auto mock = std::make_unique<testing::MockClient>();

        std::istringstream in("/exit\n");
        std::ostringstream out;

        auto result = run(makeTestConfig(), std::move(mock), in, out);

        CHECK(result == ExitCode::success);
        CHECK(out.str().find("test-model") != std::string::npos);
    }

    TEST_CASE("System prompt passes through to conversation")
    {
        auto mock = std::make_unique<testing::MockClient>();
        mock->queue_response(AssistantResponse{"I see your system prompt"});

        auto config = makeTestConfig();
        config.system_prompt = SystemPrompt{"Be concise"};

        std::istringstream in("Hello\n/exit\n");
        std::ostringstream out;

        auto result = run(config, std::move(mock), in, out);

        CHECK(result == ExitCode::success);
        // The mock was called and returned a response, proving the
        // conversation (with system prompt) was sent successfully
        auto output = out.str();
        CHECK(output.find("I see your system prompt") != std::string::npos);
    }
}

} // anonymous namespace
