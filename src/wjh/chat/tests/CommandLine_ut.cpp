// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_ASSERTS_RETURN_VALUES
#include "wjh/chat/CommandLine.hpp"

#include "testing/doctest.hpp"

namespace {
using namespace wjh::chat;

TEST_SUITE("CommandLine")
{
    TEST_CASE("No arguments")
    {
        char const * args[] = {"chat_app"};
        auto result = parse_args(args);

        REQUIRE(result.has_value());
        CHECK_FALSE(result->model.has_value());
        CHECK_FALSE(result->system_prompt.has_value());
        CHECK_FALSE(result->max_tokens.has_value());
        CHECK_FALSE(result->temperature.has_value());
        CHECK(result->show_config == ShowConfig{false});
        CHECK(result->help == ShowHelp{false});
    }

    TEST_CASE("Help flag (-h)")
    {
        char const * args[] = {"chat_app", "-h"};
        auto result = parse_args(args);

        REQUIRE(result.has_value());
        CHECK(result->help == ShowHelp{true});
    }

    TEST_CASE("Help flag (--help)")
    {
        char const * args[] = {"chat_app", "--help"};
        auto result = parse_args(args);

        REQUIRE(result.has_value());
        CHECK(result->help == ShowHelp{true});
    }

    TEST_CASE("Model flag (-m)")
    {
        char const * args[] = {"chat_app", "-m", "openai/gpt-4"};
        auto result = parse_args(args);

        REQUIRE(result.has_value());
        REQUIRE(result->model.has_value());
        CHECK(*result->model == ModelId{"openai/gpt-4"});
    }

    TEST_CASE("Model flag (--model)")
    {
        char const * args[] = {"chat_app", "--model", "openai/gpt-4"};
        auto result = parse_args(args);

        REQUIRE(result.has_value());
        REQUIRE(result->model.has_value());
        CHECK(*result->model == ModelId{"openai/gpt-4"});
    }

    TEST_CASE("System prompt flag (-s)")
    {
        char const * args[] = {"chat_app", "-s", "You are a helpful assistant"};
        auto result = parse_args(args);

        REQUIRE(result.has_value());
        REQUIRE(result->system_prompt.has_value());
        CHECK(
            *result->system_prompt ==
            SystemPrompt{"You are a helpful assistant"});
    }

    TEST_CASE("Max tokens flag (-t)")
    {
        char const * args[] = {"chat_app", "-t", "2048"};
        auto result = parse_args(args);

        REQUIRE(result.has_value());
        REQUIRE(result->max_tokens.has_value());
        CHECK(*result->max_tokens == MaxTokens{2048u});
    }

    TEST_CASE("Show config flag")
    {
        char const * args[] = {"chat_app", "--show-config"};
        auto result = parse_args(args);

        REQUIRE(result.has_value());
        CHECK(result->show_config == ShowConfig{true});
    }

    TEST_CASE("Multiple flags")
    {
        char const * args[] =
            {"chat_app", "-m", "openai/gpt-4", "-t", "1024", "--show-config"};
        auto result = parse_args(args);

        REQUIRE(result.has_value());
        REQUIRE(result->model.has_value());
        CHECK(*result->model == ModelId{"openai/gpt-4"});
        REQUIRE(result->max_tokens.has_value());
        CHECK(*result->max_tokens == MaxTokens{1024u});
        CHECK(result->show_config == ShowConfig{true});
    }

    TEST_CASE("Missing argument for -m")
    {
        char const * args[] = {"chat_app", "-m"};
        auto result = parse_args(args);

        CHECK_FALSE(result.has_value());
    }

    TEST_CASE("Missing argument for -t")
    {
        char const * args[] = {"chat_app", "-t"};
        auto result = parse_args(args);

        CHECK_FALSE(result.has_value());
    }

    TEST_CASE("Invalid number for -t")
    {
        char const * args[] = {"chat_app", "-t", "not_a_number"};
        auto result = parse_args(args);

        CHECK_FALSE(result.has_value());
    }

    TEST_CASE("Temperature flag (--temperature)")
    {
        char const * args[] =
            {"chat_app", "--temperature", "0.7"};
        auto result = parse_args(args);

        REQUIRE(result.has_value());
        REQUIRE(result->temperature.has_value());
        CHECK(atlas::undress(*result->temperature)
              == doctest::Approx(0.7f));
    }

    TEST_CASE("Temperature zero is valid")
    {
        char const * args[] =
            {"chat_app", "--temperature", "0"};
        auto result = parse_args(args);

        REQUIRE(result.has_value());
        REQUIRE(result->temperature.has_value());
        CHECK(atlas::undress(*result->temperature)
              == doctest::Approx(0.0f));
    }

    TEST_CASE("Missing argument for --temperature")
    {
        char const * args[] = {"chat_app", "--temperature"};
        auto result = parse_args(args);

        CHECK_FALSE(result.has_value());
    }

    TEST_CASE("Invalid value for --temperature")
    {
        char const * args[] =
            {"chat_app", "--temperature", "hot"};
        auto result = parse_args(args);

        CHECK_FALSE(result.has_value());
    }

    TEST_CASE("Temperature with other flags")
    {
        char const * args[] = {"chat_app",
            "-m", "openai/gpt-4",
            "--temperature", "0.5",
            "-t", "1024"};
        auto result = parse_args(args);

        REQUIRE(result.has_value());
        REQUIRE(result->model.has_value());
        CHECK(*result->model == ModelId{"openai/gpt-4"});
        REQUIRE(result->temperature.has_value());
        CHECK(atlas::undress(*result->temperature)
              == doctest::Approx(0.5f));
        REQUIRE(result->max_tokens.has_value());
        CHECK(*result->max_tokens == MaxTokens{1024u});
    }

    TEST_CASE("Unknown argument")
    {
        char const * args[] = {"chat_app", "--unknown"};
        auto result = parse_args(args);

        CHECK_FALSE(result.has_value());
    }

    TEST_CASE("help_text contains program name")
    {
        auto text = help_text(ProgramName{"my_chat"});
        CHECK(atlas::undress(text).find("my_chat") != std::string::npos);
    }
}

} // anonymous namespace
