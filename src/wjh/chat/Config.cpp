// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "wjh/chat/Config.hpp"

#include "wjh/chat/json_convert.hpp"

#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>

#include <dotenv.h>

namespace wjh::chat {

namespace {

void
load_env_if_exists(std::filesystem::path const & path)
{
    if (std::filesystem::exists(path)) {
        dotenv::init(path.c_str());
    }
}

std::optional<std::string>
get_env(char const * name)
{
    auto const * value = std::getenv(name);
    if (value and *value != '\0') {
        return std::string{value};
    }
    return std::nullopt;
}

} // anonymous namespace

void
load_env_files()
{
    // Load in reverse precedence order (lowest first)
    // so that higher-precedence files override

    // 3. User preferences
    auto const * home = std::getenv("HOME");
    if (home) {
        load_env_if_exists(
            std::filesystem::path{home} / ".config/aipp101_chat/.env");
    }

    // 2. Project config
    load_env_if_exists(".env");

    // 1. Local overrides (highest precedence)
    load_env_if_exists(".env.local");
}

Result<Config>
resolve_config(CommandLineArgs const & args)
{
    Config config{
        .api_key = ApiKey{"placeholder"},
        .model = ModelId{"anthropic/claude-sonnet-4"},
        .max_tokens = MaxTokens{4096u},
        .system_prompt = std::nullopt,
        .temperature = std::nullopt,
        .show_config = args.show_config};

    // Resolve API key (required)
    if (auto env = get_env("OPENROUTER_API_KEY")) {
        config.api_key = ApiKey{std::move(*env)};
    } else if (not args.show_config) {
        return make_error(
            "OPENROUTER_API_KEY not set. "
            "Set it in .env or export it as an environment variable.");
    }

    // Resolve model: CLI > env > default
    if (args.model) {
        config.model = *args.model;
    } else if (auto env = get_env("LLM_MODEL")) {
        config.model = ModelId{std::move(*env)};
    }

    // Resolve max tokens: CLI > env > default
    if (args.max_tokens) {
        config.max_tokens = *args.max_tokens;
    } else if (auto env = get_env("MAX_TOKENS")) {
        std::uint32_t val = 0;
        auto [ptr, ec] =
            std::from_chars(env->data(), env->data() + env->size(), val);
        if (ec != std::errc{} or ptr != env->data() + env->size()) {
            return make_error("Invalid MAX_TOKENS value: '{}'", *env);
        }
        config.max_tokens = MaxTokens{val};
    }

    // Resolve system prompt: CLI > env > none
    if (args.system_prompt) {
        config.system_prompt = *args.system_prompt;
    } else if (auto env = get_env("SYSTEM_PROMPT")) {
        config.system_prompt = SystemPrompt{std::move(*env)};
    }

    // Resolve temperature: CLI > env > none
    if (args.temperature) {
        config.temperature = *args.temperature;
    } else if (auto env = get_env("TEMPERATURE")) {
        char * end = nullptr;
        float val = std::strtof(env->c_str(), &end);
        if (end != env->c_str() + env->size()) {
            return make_error(
                "Invalid TEMPERATURE value: '{}'", *env);
        }
        config.temperature = Temperature{val};
    }

    return config;
}

void
print_config(Config const & config, std::ostream & out)
{
    out << "Configuration:\n"
        << "  Model:      " << config.model << "\n"
        << "  Max tokens: " << config.max_tokens << "\n"
        << "  API key:    " << config.api_key.substr(0u, 12u) << "...\n";
    if (config.temperature) {
        out << "  Temperature: " << *config.temperature << "\n";
    }
    if (config.system_prompt) {
        out << "  System:     " << *config.system_prompt << "\n";
    }
}

void
append_agents_file(
    Config & config,
    std::filesystem::path const & dir)
{
    auto const path = dir / "AGENTS.md";
    if (not std::filesystem::exists(path)) {
        return;
    }

    std::ifstream file(path);
    if (not file) {
        return;
    }

    // GCC 13 at -O3 inlines through istreambuf_iterator into
    // streambuf and emits a false -Wnull-dereference warning.
#if defined(__GNUC__) and not defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif
    std::string content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
#if defined(__GNUC__) and not defined(__clang__)
#pragma GCC diagnostic pop
#endif

    if (content.empty()) {
        return;
    }

    static constexpr auto wrapper_prefix =
        "<system-reminder>"
        "As you answer the user's questions, "
        "you can use the following context.\n\n"
        "Codebase and user instructions are shown "
        "below. Be sure to adhere to these "
        "instructions.\n\n"
        "IMPORTANT: These instructions OVERRIDE "
        "any default behavior and you MUST follow "
        "them as written.\n\n";
    static constexpr auto wrapper_suffix = "\n</system-reminder>";

    auto wrapped = std::string{wrapper_prefix} + content + wrapper_suffix;

    if (config.system_prompt) {
        config.system_prompt = SystemPrompt{
            std::format("{}\n{}", *config.system_prompt, wrapped)};
    } else {
        config.system_prompt = SystemPrompt{std::move(wrapped)};
    }
}

} // namespace wjh::chat
