// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_CHAT_E1F2A3B4C5D6478890ABCDEF12345678
#define WJH_CHAT_E1F2A3B4C5D6478890ABCDEF12345678

#include "wjh/chat/Config.hpp"
#include "wjh/chat/TokenUsage.hpp"
#include "wjh/chat/client/IClient.hpp"
#include "wjh/chat/conversation/Conversation.hpp"

#include <istream>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace wjh::chat {

/**
 * Process exit code.
 */
enum class ExitCode : int
{
    success = 0,
    error = 1
};

/**
 * Result of handling a REPL command.
 */
enum class CommandResult
{
    handled, ///< Command was recognized and processed.
    exit, ///< User requested exit.
    unrecognized ///< Input is not a command.
};

/**
 * Chat loop with NVI extension points.
 *
 * The public run() method is a template method that defines the
 * overall loop structure. Six private virtual do_* functions
 * provide customization points for derived classes.
 */
class ChatLoop
{
public:
    ChatLoop(
        Config config,
        std::unique_ptr<client::IClient> client,
        std::istream & in,
        std::ostream & out);

    virtual ~ChatLoop();

    ChatLoop(ChatLoop const &) = delete;
    ChatLoop & operator = (ChatLoop const &) = delete;
    ChatLoop(ChatLoop &&) = delete;
    ChatLoop & operator = (ChatLoop &&) = delete;

    /**
     * Run the chat loop (template method).
     */
    [[nodiscard]]
    ExitCode run();

protected:
    /// @name Accessors for derived classes
    /// @{
    [[nodiscard]]
    Config const & config() const
    {
        return config_;
    }

    [[nodiscard]]
    client::IClient & client()
    {
        return *client_;
    }

    [[nodiscard]]
    conversation::Conversation & conversation()
    {
        return conversation_;
    }

    [[nodiscard]]
    std::istream & in()
    {
        return in_;
    }

    [[nodiscard]]
    std::ostream & out()
    {
        return out_;
    }

    /// @}

    /**
     * Handle built-in commands (/exit, /quit, /clear, /help).
     *
     * Derived classes can call this as a fallback after checking
     * their own commands in do_handle_command().
     */
    [[nodiscard]]
    CommandResult handle_builtin_command(std::string_view cmd);

private:
    /// @name NVI extension points
    /// @{

    /**
     * Display the welcome banner.
     * Default: prints model name and help hint.
     */
    virtual void do_display_welcome();

    /**
     * Read one line of user input.
     * Default: prints "You> " prompt and calls getline.
     * @return Input line, or nullopt on EOF.
     */
    virtual std::optional<std::string> do_read_input();

    /**
     * Handle a potential REPL command.
     * Default: delegates to handle_builtin_command().
     */
    virtual CommandResult do_handle_command(std::string_view cmd);

    /**
     * Process user input: send to LLM and handle result.
     * Default: sends message, dispatches to
     * do_display_response() or do_handle_error().
     */
    virtual void do_process_input(UserInput input);

    /**
     * Display an assistant response.
     * Default: prints "Assistant> {text}".
     */
    virtual void do_display_response(AssistantResponse const & response);

    /**
     * Handle an error from the LLM client.
     * Default: prints to cerr, pops the failed message.
     */
    virtual void do_handle_error(std::string const & error);

    /// @}

    Config config_;
    std::unique_ptr<client::IClient> client_;
    conversation::Conversation conversation_;
    std::vector<TokenUsage> usage_history_;
    std::istream & in_;
    std::ostream & out_;
};

/**
 * Production entry point.
 *
 * Parses args, loads config, creates real OpenRouterClient, runs loop.
 */
[[nodiscard]]
ExitCode run(int argc, char * argv[]);

/**
 * Testable entry point with injected dependencies.
 *
 * @param config Resolved configuration
 * @param client LLM client (owned)
 * @param in Input stream (e.g., std::cin or stringstream)
 * @param out Output stream (e.g., std::cout or stringstream)
 */
[[nodiscard]]
ExitCode run(
    Config const & config,
    std::unique_ptr<client::IClient> client,
    std::istream & in,
    std::ostream & out);

} // namespace wjh::chat

#endif // WJH_CHAT_E1F2A3B4C5D6478890ABCDEF12345678
