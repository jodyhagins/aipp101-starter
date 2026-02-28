// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_CHAT_32E18E993AE1467299AB1E45EEC3991D
#define WJH_CHAT_32E18E993AE1467299AB1E45EEC3991D

#include "wjh/chat/Result.hpp"
#include "wjh/chat/types.hpp"
#include "wjh/chat/client/HttpClient.hpp"
#include "wjh/chat/client/IClient.hpp"

#include <nlohmann/json.hpp>

#include <optional>

namespace wjh::chat::client {

/**
 * Configuration for the OpenRouter client.
 */
struct OpenRouterClientConfig
{
    ApiKey api_key;
    ModelId model;
    MaxTokens max_tokens;
    std::optional<SystemPrompt> system_prompt;
    std::optional<Temperature> temperature;
};

/**
 * Client for the OpenRouter API.
 *
 * OpenRouter provides access to multiple LLM providers (GPT-4, Claude,
 * Mistral, Llama, etc.) through a single OpenAI-compatible API.
 */
class OpenRouterClient
: public IClient
{
public:
    explicit OpenRouterClient(OpenRouterClientConfig config);

    /**
     * Get the current model being used.
     */
    [[nodiscard]]
    ModelId const & model() const
    {
        return config_.model;
    }

private:
    Result<ChatResponse> do_send_message(
        conversation::Conversation const & conversation) override;

    OpenRouterClientConfig config_;
    HttpClient http_client_;

    /**
     * Build request JSON in OpenAI format.
     */
    nlohmann::json build_request(
        conversation::Conversation const & conversation) const;

    /**
     * Parse response from OpenAI format to ChatResponse.
     */
    Result<ChatResponse> parse_response(
        nlohmann::json const & json) const;

    /**
     * Send a JSON request to the API and return parsed
     * response JSON.
     */
    Result<nlohmann::json> send_api_request(
        nlohmann::json const & request);

    /**
     * Convert messages to OpenAI format.
     */
    nlohmann::json convert_messages_to_openai(
        conversation::Conversation const & conversation) const;

    /**
     * Map OpenAI finish_reason to internal StopReason.
     */
    static conversation::StopReason map_stop_reason(
        FinishReason const & finish_reason);
};

} // namespace wjh::chat::client

#endif // WJH_CHAT_32E18E993AE1467299AB1E45EEC3991D
