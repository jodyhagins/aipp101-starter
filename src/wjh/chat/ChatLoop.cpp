// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "wjh/chat/ChatLoop.hpp"

#include "wjh/chat/CommandLine.hpp"
#include "wjh/chat/json_convert.hpp"
#include "wjh/chat/client/OpenRouterClient.hpp"

#include <string>

#include <iostream>

namespace wjh::chat {

// ------------------------------------------------------------------
// ChatLoop construction / destruction
// ------------------------------------------------------------------

ChatLoop::
ChatLoop(
    Config config,
    std::unique_ptr<client::IClient> client,
    std::istream & in,
    std::ostream & out)
: config_(std::move(config))
, client_(std::move(client))
, in_(in)
, out_(out)
{ }

ChatLoop::
~ChatLoop() = default;

// ------------------------------------------------------------------
// Template method
// ------------------------------------------------------------------

ExitCode
ChatLoop::
run()
{
    if (config_.system_prompt) {
        conversation_.set_system_prompt(*config_.system_prompt);
    }

    do_display_welcome();

    while (true) {
        auto line = do_read_input();
        if (not line) {
            break;
        }

        if (line->empty()) {
            continue;
        }

        auto cmd_result = do_handle_command(*line);
        if (cmd_result == CommandResult::exit) {
            break;
        }
        if (cmd_result == CommandResult::handled) {
            continue;
        }

        do_process_input(UserInput{std::move(*line)});
    }

    return ExitCode::success;
}

// ------------------------------------------------------------------
// Protected helpers
// ------------------------------------------------------------------

CommandResult
ChatLoop::
handle_builtin_command(std::string_view cmd)
{
    if (cmd == "/exit" or cmd == "/quit") {
        out_ << "Goodbye!\n";
        return CommandResult::exit;
    }

    if (cmd == "/clear") {
        conversation_.clear();
        out_ << "Conversation cleared.\n\n";
        return CommandResult::handled;
    }

    if (cmd == "/help") {
        out_ << "Commands:\n"
            << "  /exit, /quit  Exit the chat\n"
            << "  /clear        Clear conversation history\n"
            << "  /help         Show this help\n\n";
        return CommandResult::handled;
    }

    return CommandResult::unrecognized;
}

// ------------------------------------------------------------------
// Default NVI implementations
// ------------------------------------------------------------------

void
ChatLoop::
do_display_welcome()
{
    out_ << "AI++ 101 Chat (model: " << json_value(config_.model) << ")\n"
        << "Type /help for commands, /exit to quit.\n\n";
}

std::optional<std::string>
ChatLoop::
do_read_input()
{
    out_ << "You> " << std::flush;

    std::string line;
    if (not std::getline(in_, line)) {
        out_ << "\n";
        return std::nullopt;
    }
    return line;
}

CommandResult
ChatLoop::
do_handle_command(std::string_view cmd)
{
    return handle_builtin_command(cmd);
}

void
ChatLoop::
do_process_input(UserInput input)
{
    conversation_.add_message(input);
    auto result = client_->send_message(conversation_);

    if (not result) {
        do_handle_error(result.error());
        return;
    }

    auto const & response = *result;
    do_display_response(response);
    conversation_.add_message(response);
}

void
ChatLoop::
do_display_response(AssistantResponse const & response)
{
    out_ << "\nAssistant> " << json_value(response) << "\n\n";
}

void
ChatLoop::
do_handle_error(std::string const & error)
{
    std::cerr << "Error: " << error << "\n";
    conversation_.pop_back();
}

// ------------------------------------------------------------------
// Free function wrappers (backward compatibility)
// ------------------------------------------------------------------

ExitCode
run(int argc, char * argv[])
{
    auto args_result = parse_args(
        std::span<char const * const>(argv, static_cast<std::size_t>(argc)));
    if (not args_result) {
        std::cerr << "Error: " << args_result.error() << "\n";
        return ExitCode::error;
    }

    auto const & args = *args_result;

    if (args.help) {
        std::cout << help_text(ProgramName{argv[0]});
        return ExitCode::success;
    }

    load_env_files();

    auto config_result = resolve_config(args);
    if (not config_result) {
        std::cerr << "Error: " << config_result.error() << "\n";
        return ExitCode::error;
    }

    auto const & config = *config_result;

    if (config.show_config) {
        print_config(config, std::cout);
        return ExitCode::success;
    }

    auto client = std::make_unique<client::OpenRouterClient>(
        client::OpenRouterClientConfig{
            .api_key = config.api_key,
            .model = config.model,
            .max_tokens = config.max_tokens,
            .system_prompt = config.system_prompt});

    return run(config, std::move(client), std::cin, std::cout);
}

ExitCode
run(Config const & config,
    std::unique_ptr<client::IClient> client,
    std::istream & in,
    std::ostream & out)
{
    ChatLoop loop(config, std::move(client), in, out);
    return loop.run();
}

} // namespace wjh::chat
