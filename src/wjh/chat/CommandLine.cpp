// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "wjh/chat/CommandLine.hpp"

#include <charconv>
#include <cstdlib>

namespace wjh::chat {

Result<CommandLineArgs>
parse_args(std::span<char const * const> args)
{
    CommandLineArgs result;

    // Skip program name (args[0])
    for (std::size_t i = 1; i < args.size(); ++i) {
        std::string_view arg{args[i]};

        if (arg == "-h" or arg == "--help") {
            result.help = ShowHelp{true};
            return result;
        }

        if (arg == "--show-config") {
            result.show_config = ShowConfig{true};
            continue;
        }

        if (arg == "-m" or arg == "--model") {
            if (i + 1 >= args.size()) {
                return make_error("Missing argument for {}", arg);
            }
            result.model = ModelId{args[++i]};
            continue;
        }

        if (arg == "-s" or arg == "--system-prompt") {
            if (i + 1 >= args.size()) {
                return make_error("Missing argument for {}", arg);
            }
            result.system_prompt = SystemPrompt{args[++i]};
            continue;
        }

        if (arg == "-t" or arg == "--max-tokens") {
            if (i + 1 >= args.size()) {
                return make_error("Missing argument for {}", arg);
            }
            ++i;
            std::string_view val{args[i]};
            std::uint32_t tokens = 0;
            auto [ptr, ec] =
                std::from_chars(val.data(), val.data() + val.size(), tokens);
            if (ec != std::errc{} or ptr != val.data() + val.size()) {
                return make_error("Invalid number for --max-tokens: '{}'", val);
            }
            result.max_tokens = MaxTokens{tokens};
            continue;
        }

        if (arg == "--temperature") {
            if (i + 1 >= args.size()) {
                return make_error(
                    "Missing argument for {}", arg);
            }
            ++i;
            std::string arg_str{args[i]};
            char * end = nullptr;
            float temp = std::strtof(arg_str.c_str(), &end);
            if (end != arg_str.c_str() + arg_str.size()) {
                return make_error(
                    "Invalid number for --temperature:"
                    " '{}'",
                    args[i]);
            }
            result.temperature = Temperature{temp};
            continue;
        }

        return make_error("Unknown argument: '{}'", arg);
    }

    return result;
}

HelpText
help_text(ProgramName const & program_name)
{
    constexpr auto fmt = R"(Usage: {} [options]

AI++ 101 Chat Application

Options:
  -m, --model <id>           Model ID (default: anthropic/claude-sonnet-4)
  -s, --system-prompt <text>  System prompt
  -t, --max-tokens <n>        Max response tokens (default: 4096)
  --temperature <value>       LLM temperature (0.0-2.0)
  --show-config               Display resolved config and exit
  -h, --help                  Show this help message

Environment variables:
  OPENROUTER_API_KEY          API key (required)
  LLM_MODEL                   Model ID override
  MAX_TOKENS                  Max tokens override
  TEMPERATURE                 LLM temperature override
  SYSTEM_PROMPT               System prompt

REPL commands:
  /exit, /quit                Exit the chat
  /clear                      Clear conversation history
  /help                       Show REPL commands
)";
    return HelpText{std::format(fmt, program_name)};
}

} // namespace wjh::chat
