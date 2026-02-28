// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "wjh/chat/client/OpenRouterClient.hpp"

#include "wjh/chat/json_convert.hpp"
#include "wjh/chat/stdfmt.hpp"
#include "wjh/chat/conversation/Message.hpp"

#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {

constexpr bool DEBUG_COMMS = false;

void debug_json(
    std::string_view label,
    nlohmann::json const & json)
{
    if constexpr (DEBUG_COMMS) {
        wjh::chat::print(
            stderr,
            "\n=== {} ===\n{}\n",
            label,
            json.dump(2));
    }
}

nlohmann::json make_tools_json()
{
    auto bash_tool = nlohmann::json{
        {"type", "function"},
        {"function",
         {{"name", "bash"},
          {"description",
           "Execute a bash command. Use this to run "
           "shell commands, compile code, run tests, "
           "and other terminal operations."},
          {"parameters",
           {{"type", "object"},
            {"properties",
             {{"command",
               {{"type", "string"},
                {"description",
                 "The bash command to execute"}}}}},
            {"required", {"command"}}}}}}};

    auto read_file_tool = nlohmann::json{
        {"type", "function"},
        {"function",
         {{"name", "read_file"},
          {"description",
           "Read the contents of a file. Returns "
           "lines with line numbers. Use this "
           "instead of bash cat/head/tail."},
          {"parameters",
           {{"type", "object"},
            {"properties",
             {{"file_path",
               {{"type", "string"},
                {"description",
                 "Path to the file to read"}}},
              {"offset",
               {{"type", "integer"},
                {"description",
                 "1-indexed line number to start "
                 "from (optional)"}}},
              {"limit",
               {{"type", "integer"},
                {"description",
                 "Maximum number of lines to read "
                 "(optional)"}}}}},
            {"required", {"file_path"}}}}}}};

    auto write_file_tool = nlohmann::json{
        {"type", "function"},
        {"function",
         {{"name", "write_file"},
          {"description",
           "Write content to a file. Creates parent "
           "directories if needed. Use this instead "
           "of bash echo/cat with redirects."},
          {"parameters",
           {{"type", "object"},
            {"properties",
             {{"file_path",
               {{"type", "string"},
                {"description",
                 "Path to the file to write"}}},
              {"content",
               {{"type", "string"},
                {"description",
                 "The content to write to the "
                 "file"}}}}},
            {"required",
             {"file_path", "content"}}}}}}};

    auto edit_file_tool = nlohmann::json{
        {"type", "function"},
        {"function",
         {{"name", "edit_file"},
          {"description",
           "Make a targeted edit to a file by "
           "replacing an exact string. The old_string"
           " must appear exactly once in the file. "
           "Use this instead of bash sed."},
          {"parameters",
           {{"type", "object"},
            {"properties",
             {{"file_path",
               {{"type", "string"},
                {"description",
                 "Path to the file to edit"}}},
              {"old_string",
               {{"type", "string"},
                {"description",
                 "The exact string to find and "
                 "replace (must be unique)"}}},
              {"new_string",
               {{"type", "string"},
                {"description",
                 "The replacement string"}}}}},
            {"required",
             {"file_path", "old_string",
              "new_string"}}}}}}};

    return {bash_tool, read_file_tool,
            write_file_tool, edit_file_tool};
}

std::string execute_bash(std::string const & command)
{
    std::cerr << "\n[tool] bash: " << command
              << "\n[y/n]> " << std::flush;
    std::string answer;
    std::getline(std::cin, answer);
    if (answer.empty()
        or (answer[0] != 'y' and answer[0] != 'Y'))
    {
        return "Command skipped by user";
    }

    std::string full_cmd = command + " 2>&1";
    std::array<char, 4096> buffer;
    std::string result;

    auto * pipe = popen(full_cmd.c_str(), "r");
    if (not pipe) {
        return "Error: failed to execute command";
    }

    while (fgets(buffer.data(), buffer.size(), pipe)) {
        result += buffer.data();
        if (result.size() > 100'000) {
            result += "\n... [truncated at 100KB]";
            break;
        }
    }

    auto status = pclose(pipe);
    result +=
        "\n[exit code: "
        + std::to_string(WEXITSTATUS(status)) + "]";
    return result;
}

std::string execute_read_file(
    nlohmann::json const & args)
{
    auto path =
        args["file_path"].get<std::string>();

    std::ifstream file(path);
    if (not file.is_open()) {
        return "Error: Cannot open file: " + path;
    }

    int offset = 1;
    int limit = std::numeric_limits<int>::max();
    if (args.contains("offset")) {
        offset = args["offset"].get<int>();
    }
    if (args.contains("limit")) {
        limit = args["limit"].get<int>();
    }

    std::string result;
    std::string line;
    int line_num = 0;
    int lines_read = 0;

    while (std::getline(file, line)) {
        ++line_num;
        if (line_num < offset) {
            continue;
        }
        if (lines_read >= limit) {
            break;
        }
        result += std::format(
            "{:>6}\t{}\n", line_num, line);
        ++lines_read;
        if (result.size() > 100'000) {
            result += "\n... [truncated at 100KB]";
            break;
        }
    }

    if (result.empty()) {
        return "File is empty or offset is past end";
    }
    return result;
}

std::string execute_write_file(
    nlohmann::json const & args)
{
    auto path =
        args["file_path"].get<std::string>();
    auto content =
        args["content"].get<std::string>();

    std::cerr
        << "\n[tool] write_file: " << path
        << " (" << content.size() << " bytes)"
        << "\n[y/n]> " << std::flush;
    std::string answer;
    std::getline(std::cin, answer);
    if (answer.empty()
        or (answer[0] != 'y' and answer[0] != 'Y'))
    {
        return "Write skipped by user";
    }

    auto parent =
        std::filesystem::path(path).parent_path();
    if (not parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(
            parent, ec);
        if (ec) {
            return "Error: Cannot create directory: "
                + parent.string();
        }
    }

    std::ofstream file(path);
    if (not file.is_open()) {
        return "Error: Cannot open file for "
               "writing: " + path;
    }

    file << content;
    if (not file.good()) {
        return "Error: Write failed";
    }

    return "Wrote " + std::to_string(content.size())
        + " bytes to " + path;
}

std::string execute_edit_file(
    nlohmann::json const & args)
{
    auto path =
        args["file_path"].get<std::string>();
    auto old_string =
        args["old_string"].get<std::string>();
    auto new_string =
        args["new_string"].get<std::string>();

    // Read the entire file
    std::ifstream file(path);
    if (not file.is_open()) {
        return "Error: Cannot open file: " + path;
    }
    std::string contents(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    file.close();

    // Check uniqueness before prompting
    std::size_t count = 0;
    std::size_t pos = 0;
    std::size_t found_pos = std::string::npos;
    while ((pos = contents.find(old_string, pos))
           != std::string::npos)
    {
        ++count;
        found_pos = pos;
        pos += old_string.size();
    }

    if (count == 0) {
        return "Error: old_string not found in "
            + path;
    }
    if (count > 1) {
        return "Error: old_string is not unique in "
            + path + " (found "
            + std::to_string(count)
            + " occurrences)";
    }

    // Show diff preview and prompt
    std::cerr
        << "\n[tool] edit_file: " << path
        << "\n--- old ---\n" << old_string
        << "\n--- new ---\n" << new_string
        << "\n[y/n]> " << std::flush;
    std::string answer;
    std::getline(std::cin, answer);
    if (answer.empty()
        or (answer[0] != 'y' and answer[0] != 'Y'))
    {
        return "Edit skipped by user";
    }

    // Apply the replacement
    contents.replace(
        found_pos, old_string.size(), new_string);

    // Write back
    std::ofstream out(path);
    if (not out.is_open()) {
        return "Error: Cannot write file: " + path;
    }
    out << contents;
    if (not out.good()) {
        return "Error: Write failed";
    }

    return "Applied edit to " + path;
}

std::string dispatch_tool(
    std::string const & name,
    nlohmann::json const & args)
{
    if (name == "bash") {
        return execute_bash(
            args["command"].get<std::string>());
    }
    if (name == "read_file") {
        return execute_read_file(args);
    }
    if (name == "write_file") {
        return execute_write_file(args);
    }
    if (name == "edit_file") {
        return execute_edit_file(args);
    }
    return "Error: unknown tool: " + name;
}

} // anonymous namespace

namespace wjh::chat::client {

OpenRouterClient::
OpenRouterClient(OpenRouterClientConfig config)
: config_(std::move(config))
, http_client_(Hostname{"openrouter.ai"}, PortNumber{443})
{ }

nlohmann::json
OpenRouterClient::
convert_messages_to_openai(
    conversation::Conversation const & conversation) const
{
    auto messages = nlohmann::json::array();

    // Add system message if present
    auto const & system_prompt = config_.system_prompt
        ? config_.system_prompt
        : conversation.system_prompt();
    if (system_prompt) {
        messages.push_back(
            {{"role", "system"}, {"content", json_value(*system_prompt)}});
    }

    // Convert each message (simple role + content format)
    for (auto const & msg : conversation.messages()) {
        messages.push_back(to_json(msg));
    }

    return messages;
}

nlohmann::json
OpenRouterClient::
build_request(conversation::Conversation const & conversation) const
{
    auto request = nlohmann::json{
        {"model", json_value(config_.model)},
        {"max_tokens", json_value(config_.max_tokens)},
        {"messages", convert_messages_to_openai(conversation)}};

    if (config_.temperature) {
        request["temperature"] =
            json_value(*config_.temperature);
    }

    request["tools"] = make_tools_json();

    return request;
}

conversation::StopReason
OpenRouterClient::
map_stop_reason(FinishReason const & finish_reason)
{
    if (finish_reason == FinishReason{"stop"}) {
        return conversation::StopReason::end_turn;
    } else if (finish_reason == FinishReason{"length"}) {
        return conversation::StopReason::max_tokens;
    } else if (finish_reason == FinishReason{"content_filter"}) {
        return conversation::StopReason::stop_sequence;
    }

    // Default to end_turn for unknown reasons
    return conversation::StopReason(atlas::undress(finish_reason));
}

Result<ChatResponse>
OpenRouterClient::
parse_response(nlohmann::json const & json) const
{
    try {
        if (not json.contains("choices") or json["choices"].empty()) {
            return make_error("Response missing choices array");
        }

        auto const & choice = json["choices"][0];
        auto const & message = choice.at("message");

        // Extract token usage if present (needed by both
        // tool-call and text-content paths)
        std::optional<TokenUsage> usage;
        if (json.contains("usage")) {
            auto const & u = json["usage"];
            usage = TokenUsage{
                .prompt_tokens = PromptTokens{
                    u.value("prompt_tokens", 0u)},
                .completion_tokens = CompletionTokens{
                    u.value("completion_tokens", 0u)},
                .total_tokens = TotalTokens{
                    u.value("total_tokens", 0u)}};
        }

        // Check for tool calls
        if (message.contains("tool_calls")
            and not message["tool_calls"].empty())
        {
            std::string display;
            for (auto const & tc :
                 message["tool_calls"])
            {
                auto const & fn = tc["function"];
                display +=
                    "[Tool call] "
                    + fn["name"].get<std::string>()
                    + ": "
                    + fn["arguments"]
                          .get<std::string>()
                    + "\n";
            }
            return ChatResponse{
                .response = AssistantResponse{
                    std::move(display)},
                .usage = std::move(usage)};
        }

        // Extract text content
        if (not message.contains("content")
            or message["content"].is_null())
        {
            return make_error(
                "Response contains no text content");
        }

        auto text =
            message["content"].get<std::string>();

        return ChatResponse{
            .response =
                AssistantResponse{std::move(text)},
            .usage = std::move(usage)};
    } catch (nlohmann::json::exception const & e) {
        return make_error("Failed to parse API response: {}", e.what());
    }
}

Result<nlohmann::json>
OpenRouterClient::
send_api_request(nlohmann::json const & request)
{
    HttpHeaders headers{
        {HeaderName{"Authorization"},
         HeaderValue{
             "Bearer " + json_value(config_.api_key)}},
        {HeaderName{"Content-Type"},
         HeaderValue{"application/json"}}};

    auto result = http_client_.post(
        HttpPath{"/api/v1/chat/completions"},
        HttpBody{request.dump()},
        headers);
    if (not result) {
        return make_error("{}", result.error());
    }

    auto const & response = *result;

    if (response.status != HttpStatusCode{200}) {
        try {
            auto err = nlohmann::json::parse(
                json_value(response.body));
            if (err.contains("error")
                and err["error"].contains("message"))
            {
                return make_error(
                    "API error ({}): {}",
                    json_value(response.status),
                    err["error"]["message"]
                        .get<std::string>());
            }
        } catch (nlohmann::json::exception const &) {
        }
        return make_error(
            "API error ({}): {}",
            json_value(response.status),
            json_value(response.body));
    }

    try {
        return nlohmann::json::parse(
            json_value(response.body));
    } catch (nlohmann::json::parse_error const & e) {
        return make_error(
            "Failed to parse response JSON: {}",
            e.what());
    }
}

Result<ChatResponse>
OpenRouterClient::
do_send_message(
    conversation::Conversation const & conversation)
{
    auto messages =
        convert_messages_to_openai(conversation);
    auto const tools = make_tools_json();

    for (int i = 0; i < 20; ++i) {
        auto request = nlohmann::json{
            {"model", json_value(config_.model)},
            {"max_tokens",
             json_value(config_.max_tokens)},
            {"messages", messages},
            {"tools", tools}};

        if (config_.temperature) {
            request["temperature"] =
                json_value(*config_.temperature);
        }

        debug_json("request", request);

        auto result = send_api_request(request);
        if (not result) {
            return make_error("{}", result.error());
        }

        debug_json("response", *result);

        auto const & choice = (*result)["choices"][0];
        auto const & message = choice["message"];

        // Tool calls: execute and loop
        if (message.contains("tool_calls")
            and not message["tool_calls"].empty())
        {
            messages.push_back(message);

            for (auto const & tc :
                 message["tool_calls"])
            {
                auto name =
                    tc["function"]["name"]
                        .get<std::string>();
                auto args = nlohmann::json::parse(
                    tc["function"]["arguments"]
                        .get<std::string>());

                auto output =
                    dispatch_tool(name, args);
                std::cerr << output << std::endl;

                messages.push_back(
                    {{"role", "tool"},
                     {"tool_call_id", tc["id"]},
                     {"content", output}});
            }
            continue;
        }

        // Text content: return to user
        if (message.contains("content")
            and not message["content"].is_null()
            and not message["content"]
                        .get<std::string>()
                        .empty())
        {
            return parse_response(*result);
        }

        // Empty/null content: nudge the model
        if (message.contains("content")) {
            messages.push_back(message);
        }
        messages.push_back(
            {{"role", "user"},
             {"content",
              "Please use your tools or respond "
              "with text."}});
    }

    return make_error(
        "Agent loop exceeded 20 iterations");
}

} // namespace wjh::chat::client
