// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_ASSERTS_RETURN_VALUES
#include "wjh/chat/Config.hpp"

#include <cstdlib>

#include "testing/doctest.hpp"

namespace {
using namespace wjh::chat;

// RAII helper to set/unset environment variables for tests
struct EnvGuard
{
    std::string name;
    std::optional<std::string> old_value;

    EnvGuard(char const * n, char const * v)
    : name(n)
    {
        auto const * old = std::getenv(n);
        if (old) {
            old_value = old;
        }
        if (v) {
            setenv(n, v, 1);
        } else {
            unsetenv(n);
        }
    }

    ~EnvGuard()
    {
        if (old_value) {
            setenv(name.c_str(), old_value->c_str(), 1);
        } else {
            unsetenv(name.c_str());
        }
    }

    EnvGuard(EnvGuard const &) = delete;
    EnvGuard & operator = (EnvGuard const &) = delete;
};

TEST_SUITE("Config")
{
    TEST_CASE("resolve_config: missing API key returns error")
    {
        EnvGuard guard("OPENROUTER_API_KEY", nullptr);
        CommandLineArgs args;
        auto result = resolve_config(args);

        CHECK_FALSE(result.has_value());
        CHECK(result.error().find("OPENROUTER_API_KEY") != std::string::npos);
    }

    TEST_CASE("resolve_config: API key from environment")
    {
        EnvGuard guard("OPENROUTER_API_KEY", "sk-test-key-123");
        CommandLineArgs args;
        auto result = resolve_config(args);

        REQUIRE(result.has_value());
        CHECK(result->api_key == ApiKey{"sk-test-key-123"});
    }

    TEST_CASE("resolve_config: defaults")
    {
        EnvGuard key_guard("OPENROUTER_API_KEY", "sk-test");
        EnvGuard model_guard("LLM_MODEL", nullptr);
        EnvGuard tokens_guard("MAX_TOKENS", nullptr);
        EnvGuard prompt_guard("SYSTEM_PROMPT", nullptr);
        CommandLineArgs args;
        auto result = resolve_config(args);

        REQUIRE(result.has_value());
        CHECK(result->model == ModelId{"anthropic/claude-sonnet-4"});
        CHECK(result->max_tokens == MaxTokens{4096u});
        CHECK_FALSE(result->system_prompt.has_value());
    }

    TEST_CASE("resolve_config: env overrides defaults")
    {
        EnvGuard key_guard("OPENROUTER_API_KEY", "sk-test");
        EnvGuard model_guard("LLM_MODEL", "openai/gpt-4");
        EnvGuard tokens_guard("MAX_TOKENS", "2048");
        EnvGuard prompt_guard("SYSTEM_PROMPT", "Be helpful");
        CommandLineArgs args;
        auto result = resolve_config(args);

        REQUIRE(result.has_value());
        CHECK(result->model == ModelId{"openai/gpt-4"});
        CHECK(result->max_tokens == MaxTokens{2048u});
        REQUIRE(result->system_prompt.has_value());
        CHECK(*result->system_prompt == SystemPrompt{"Be helpful"});
    }

    TEST_CASE("resolve_config: CLI overrides env")
    {
        EnvGuard key_guard("OPENROUTER_API_KEY", "sk-test");
        EnvGuard model_guard("LLM_MODEL", "openai/gpt-4");
        CommandLineArgs args;
        args.model = ModelId{"anthropic/claude-3-opus"};
        args.max_tokens = MaxTokens{1024u};
        auto result = resolve_config(args);

        REQUIRE(result.has_value());
        CHECK(result->model == ModelId{"anthropic/claude-3-opus"});
        CHECK(result->max_tokens == MaxTokens{1024u});
    }

    TEST_CASE("resolve_config: show_config allows missing key")
    {
        EnvGuard guard("OPENROUTER_API_KEY", nullptr);
        CommandLineArgs args;
        args.show_config = ShowConfig{true};
        auto result = resolve_config(args);

        REQUIRE(result.has_value());
        CHECK(result->show_config == ShowConfig{true});
    }

    TEST_CASE("resolve_config: invalid MAX_TOKENS")
    {
        EnvGuard key_guard("OPENROUTER_API_KEY", "sk-test");
        EnvGuard tokens_guard("MAX_TOKENS", "not_a_number");
        CommandLineArgs args;
        auto result = resolve_config(args);

        CHECK_FALSE(result.has_value());
    }
}

} // anonymous namespace
