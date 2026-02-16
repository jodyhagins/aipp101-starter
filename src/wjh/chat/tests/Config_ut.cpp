// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_ASSERTS_RETURN_VALUES
#include "wjh/chat/Config.hpp"

#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>

#include <unistd.h>

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

// RAII helper that creates a temporary directory and
// removes it (with contents) on destruction.
struct TempDir
{
    std::filesystem::path path_;

    TempDir()
    : path_(std::filesystem::temp_directory_path()
          / "wjh_chat_test_XXXXXX")
    {
        // Create a unique directory name
        auto tmpl = path_.string();
        auto * result = mkdtemp(tmpl.data());
        REQUIRE(result != nullptr);
        path_ = result;
    }

    ~TempDir()
    {
        std::filesystem::remove_all(path_);
    }

    void write_file(
        std::string const & name,
        std::string const & content) const
    {
        std::ofstream out(path_ / name);
        out << content;
    }

    TempDir(TempDir const &) = delete;
    TempDir & operator = (TempDir const &) = delete;
};

Config
make_test_config()
{
    return Config{
        .api_key = ApiKey{"sk-test-key"},
        .model = ModelId{"test/model"},
        .max_tokens = MaxTokens{1024u},
        .system_prompt = std::nullopt,
        .temperature = std::nullopt,
        .show_config = ShowConfig{false}};
}

TEST_SUITE("Config")
{
    TEST_CASE("resolve_config: missing API key returns error")
    {
        EnvGuard guard("OPENROUTER_API_KEY", nullptr);
        CommandLineArgs args;
        auto result = resolve_config(args);

        CHECK_FALSE(result.has_value());
        CHECK(result.error().find("OPENROUTER_API_KEY")
              != std::string::npos);
    }

    TEST_CASE("resolve_config: API key from environment")
    {
        EnvGuard guard(
            "OPENROUTER_API_KEY", "sk-test-key-123");
        CommandLineArgs args;
        auto result = resolve_config(args);

        REQUIRE(result.has_value());
        CHECK(result->api_key
              == ApiKey{"sk-test-key-123"});
    }

    TEST_CASE("resolve_config: defaults")
    {
        EnvGuard key_guard(
            "OPENROUTER_API_KEY", "sk-test");
        EnvGuard model_guard("LLM_MODEL", nullptr);
        EnvGuard tokens_guard("MAX_TOKENS", nullptr);
        EnvGuard prompt_guard("SYSTEM_PROMPT", nullptr);
        EnvGuard temp_guard("TEMPERATURE", nullptr);
        CommandLineArgs args;
        auto result = resolve_config(args);

        REQUIRE(result.has_value());
        CHECK(result->model
              == ModelId{"anthropic/claude-sonnet-4"});
        CHECK(result->max_tokens == MaxTokens{4096u});
        CHECK_FALSE(result->system_prompt.has_value());
        CHECK_FALSE(result->temperature.has_value());
    }

    TEST_CASE("resolve_config: env overrides defaults")
    {
        EnvGuard key_guard(
            "OPENROUTER_API_KEY", "sk-test");
        EnvGuard model_guard("LLM_MODEL", "openai/gpt-4");
        EnvGuard tokens_guard("MAX_TOKENS", "2048");
        EnvGuard prompt_guard(
            "SYSTEM_PROMPT", "Be helpful");
        EnvGuard temp_guard("TEMPERATURE", nullptr);
        CommandLineArgs args;
        auto result = resolve_config(args);

        REQUIRE(result.has_value());
        CHECK(result->model == ModelId{"openai/gpt-4"});
        CHECK(result->max_tokens == MaxTokens{2048u});
        REQUIRE(result->system_prompt.has_value());
        CHECK(*result->system_prompt
              == SystemPrompt{"Be helpful"});
    }

    TEST_CASE("resolve_config: CLI overrides env")
    {
        EnvGuard key_guard(
            "OPENROUTER_API_KEY", "sk-test");
        EnvGuard model_guard("LLM_MODEL", "openai/gpt-4");
        CommandLineArgs args;
        args.model = ModelId{"anthropic/claude-3-opus"};
        args.max_tokens = MaxTokens{1024u};
        auto result = resolve_config(args);

        REQUIRE(result.has_value());
        CHECK(result->model
              == ModelId{"anthropic/claude-3-opus"});
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
        EnvGuard key_guard(
            "OPENROUTER_API_KEY", "sk-test");
        EnvGuard tokens_guard(
            "MAX_TOKENS", "not_a_number");
        CommandLineArgs args;
        auto result = resolve_config(args);

        CHECK_FALSE(result.has_value());
    }

    TEST_CASE("resolve_config: temperature from env")
    {
        EnvGuard key_guard(
            "OPENROUTER_API_KEY", "sk-test");
        EnvGuard temp_guard("TEMPERATURE", "0.5");
        CommandLineArgs args;
        auto result = resolve_config(args);

        REQUIRE(result.has_value());
        REQUIRE(result->temperature.has_value());
        CHECK(atlas::undress(*result->temperature)
              == doctest::Approx(0.5f));
    }

    TEST_CASE("resolve_config: temperature CLI overrides env")
    {
        EnvGuard key_guard(
            "OPENROUTER_API_KEY", "sk-test");
        EnvGuard temp_guard("TEMPERATURE", "0.5");
        CommandLineArgs args;
        args.temperature = Temperature{0.9f};
        auto result = resolve_config(args);

        REQUIRE(result.has_value());
        REQUIRE(result->temperature.has_value());
        CHECK(atlas::undress(*result->temperature)
              == doctest::Approx(0.9f));
    }

    TEST_CASE("resolve_config: invalid TEMPERATURE")
    {
        EnvGuard key_guard(
            "OPENROUTER_API_KEY", "sk-test");
        EnvGuard temp_guard(
            "TEMPERATURE", "not_a_number");
        CommandLineArgs args;
        auto result = resolve_config(args);

        CHECK_FALSE(result.has_value());
    }

    TEST_CASE("append_agents_file: no file leaves config "
              "unchanged")
    {
        TempDir dir;
        auto config = make_test_config();

        append_agents_file(config, dir.path_);

        CHECK_FALSE(config.system_prompt.has_value());
    }

    TEST_CASE("append_agents_file: no file preserves "
              "existing system prompt")
    {
        TempDir dir;
        auto config = make_test_config();
        config.system_prompt =
            SystemPrompt{"existing prompt"};

        append_agents_file(config, dir.path_);

        REQUIRE(config.system_prompt.has_value());
        CHECK(*config.system_prompt
              == SystemPrompt{"existing prompt"});
    }

    TEST_CASE("append_agents_file: sets system prompt "
              "from file content")
    {
        TempDir dir;
        dir.write_file(
            "AGENTS.md", "# Test Instructions\nDo X.");
        auto config = make_test_config();

        append_agents_file(config, dir.path_);

        REQUIRE(config.system_prompt.has_value());
        auto prompt =
            std::format("{}", *config.system_prompt);

        CHECK(prompt.starts_with("<system-reminder>"));
        CHECK(prompt.ends_with("</system-reminder>"));
        CHECK(prompt.find("# Test Instructions\nDo X.")
              != std::string::npos);
    }

    TEST_CASE("append_agents_file: appends to existing "
              "system prompt")
    {
        TempDir dir;
        dir.write_file("AGENTS.md", "Agent rules here.");
        auto config = make_test_config();
        config.system_prompt =
            SystemPrompt{"You are helpful."};

        append_agents_file(config, dir.path_);

        REQUIRE(config.system_prompt.has_value());
        auto prompt =
            std::format("{}", *config.system_prompt);

        // Existing prompt appears first
        CHECK(prompt.starts_with("You are helpful."));

        // Wrapped content appears after a newline
        auto wrapped_pos =
            prompt.find("<system-reminder>");
        REQUIRE(wrapped_pos != std::string::npos);
        CHECK(wrapped_pos > 0);
        CHECK(prompt[wrapped_pos - 1] == '\n');

        // File content is inside the wrapper
        CHECK(prompt.find("Agent rules here.")
              != std::string::npos);
        CHECK(prompt.ends_with("</system-reminder>"));
    }

    TEST_CASE("append_agents_file: empty file leaves "
              "config unchanged")
    {
        TempDir dir;
        dir.write_file("AGENTS.md", "");
        auto config = make_test_config();

        append_agents_file(config, dir.path_);

        CHECK_FALSE(config.system_prompt.has_value());
    }

    TEST_CASE("append_agents_file: wrapper tags have "
              "correct structure")
    {
        TempDir dir;
        dir.write_file("AGENTS.md", "payload");
        auto config = make_test_config();

        append_agents_file(config, dir.path_);

        REQUIRE(config.system_prompt.has_value());
        auto prompt =
            std::format("{}", *config.system_prompt);

        // Verify the prefix contains expected instruction
        auto payload_pos = prompt.find("payload");
        REQUIRE(payload_pos != std::string::npos);
        auto prefix = prompt.substr(0, payload_pos);

        CHECK(prefix.find(
            "As you answer the user's questions")
              != std::string::npos);
        CHECK(prefix.find(
            "Codebase and user instructions")
              != std::string::npos);
        CHECK(prefix.find(
            "IMPORTANT: These instructions OVERRIDE")
              != std::string::npos);

        // Suffix is "\n</system-reminder>" immediately
        // after the content
        auto suffix =
            prompt.substr(payload_pos + 7); // "payload"
        CHECK(suffix == "\n</system-reminder>");
    }
}

} // anonymous namespace
