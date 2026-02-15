// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_CHAT_0AEA43995B8347128A834C0E8EBFACEE
#define WJH_CHAT_0AEA43995B8347128A834C0E8EBFACEE

#include "wjh/chat/Result.hpp"
#include "wjh/chat/client/types.hpp"

#include <initializer_list>
#include <map>
#include <string>
#include <utility>

namespace wjh::chat::client {

/**
 * Semantic type for HTTP header key-value pairs.
 *
 * Encapsulates the raw string map so it cannot be confused with
 * other map-of-string types at call sites.
 */
class HttpHeaders
{
public:
    HttpHeaders() = default;

    explicit HttpHeaders(
        std::initializer_list<std::pair<HeaderName, HeaderValue>> init)
    {
        for (auto const & [k, v] : init) {
            add(k, v);
        }
    }

    HttpHeaders(HttpHeaders const &) = default;
    HttpHeaders(HttpHeaders &&) noexcept = default;
    HttpHeaders & operator = (HttpHeaders const &) = default;
    HttpHeaders & operator = (HttpHeaders &&) noexcept = default;

    /// Insert a header. If the key already exists, it is not replaced
    /// (first-write-wins semantics, matching std::map::emplace).
    void add(HeaderName key, HeaderValue value)
    {
        headers_.emplace(
            atlas::undress(std::move(key)),
            atlas::undress(std::move(value)));
    }

    using const_iterator = std::map<std::string, std::string>::const_iterator;

    [[nodiscard]]
    const_iterator begin() const
    {
        return headers_.begin();
    }

    [[nodiscard]]
    const_iterator end() const
    {
        return headers_.end();
    }

    [[nodiscard]]
    bool empty() const
    {
        return headers_.empty();
    }

private:
    std::map<std::string, std::string> headers_;
};

/**
 * HTTP response containing status, headers, and body.
 */
struct HttpResponse
{
    HttpStatusCode status;
    HttpHeaders headers;
    HttpBody body;
};

/**
 * Simple HTTP client abstraction using cpp-httplib.
 *
 * This provides a basic interface for making HTTPS requests,
 * primarily for the OpenRouter API.
 */
class HttpClient
{
public:
    /**
     * Construct a client for the given host.
     * @param host The hostname (e.g., "openrouter.ai")
     * @param port The port (default 443 for HTTPS)
     */
    explicit HttpClient(Hostname host, PortNumber port = PortNumber{443});

    /**
     * Make a POST request.
     * @param path The request path
     * @param body The request body
     * @param headers Additional headers to include
     * @return Response or error message
     */
    [[nodiscard]]
    Result<HttpResponse> post(
        HttpPath const & path,
        HttpBody const & body,
        HttpHeaders const & headers);

    /**
     * Set connection timeout in seconds.
     */
    void set_connection_timeout(TimeoutSeconds seconds);

    /**
     * Set read timeout in seconds.
     */
    void set_read_timeout(TimeoutSeconds seconds);

private:
    Hostname host_;
    PortNumber port_;
    TimeoutSeconds connection_timeout_{30};
    TimeoutSeconds read_timeout_{120};
};

} // namespace wjh::chat::client

#endif // WJH_CHAT_0AEA43995B8347128A834C0E8EBFACEE
