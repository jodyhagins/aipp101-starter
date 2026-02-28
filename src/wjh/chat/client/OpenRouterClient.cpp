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
    return {{
        {"type", "function"},
        {"function",
         {{"name", "bash"},
          {"description",
           "Execute a bash command. Use this to run "
           "shell commands, read/write files, compile "
           "code, run tests, etc."},
          {"parameters",
           {{"type", "object"},
            {"properties",
             {{"command",
               {{"type", "string"},
                {"description",
                 "The bash command to execute"}}}}},
            {"required", {"command"}}}}}}
    }};
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

Result<ChatResponse>
OpenRouterClient::
do_send_message(conversation::Conversation const & conversation)
{
    auto request = build_request(conversation);
    debug_json("request", request);
    auto request_body = request.dump();

    HttpHeaders headers{
        {HeaderName{"Authorization"},
         HeaderValue{"Bearer " + json_value(config_.api_key)}},
        {HeaderName{"Content-Type"}, HeaderValue{"application/json"}}};

    auto result = http_client_.post(
        HttpPath{"/api/v1/chat/completions"},
        HttpBody{request_body},
        headers);
    if (not result) {
        return make_error("{}", result.error());
    }

    auto const & response = *result;

    // Check for HTTP errors
    if (response.status != HttpStatusCode{200}) {
        try {
            auto error_json = nlohmann::json::parse(json_value(response.body));
            if (error_json.contains("error")) {
                auto const & error = error_json["error"];
                if (error.contains("message")) {
                    return make_error(
                        "API error ({}): {}",
                        json_value(response.status),
                        error["message"].get<std::string>());
                }
            }
        } catch (nlohmann::json::exception const &) {
            // Fall through to generic error
        }
        return make_error(
            "API error ({}): {}",
            json_value(response.status),
            json_value(response.body));
    }

    // Parse successful response
    nlohmann::json response_json;
    try {
        response_json =
            nlohmann::json::parse(json_value(response.body));
    } catch (nlohmann::json::parse_error const & e) {
        return make_error(
            "Failed to parse response JSON: {}",
            e.what());
    }

    debug_json("response", response_json);

    return parse_response(response_json);
}

} // namespace wjh::chat::client
