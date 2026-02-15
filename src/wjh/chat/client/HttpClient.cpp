// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "wjh/chat/client/HttpClient.hpp"

#include "wjh/chat/json_convert.hpp"

#include <httplib.h>

namespace wjh::chat::client {

HttpClient::
HttpClient(Hostname host, PortNumber port)
: host_(std::move(host))
, port_(port)
{ }

Result<HttpResponse>
HttpClient::
post(HttpPath const & path, HttpBody const & body, HttpHeaders const & headers)
{
    httplib::SSLClient client(json_value(host_), json_value(port_));
    client.set_connection_timeout(json_value(connection_timeout_), 0);
    client.set_read_timeout(json_value(read_timeout_), 0);
    client.enable_server_certificate_verification(true);

    httplib::Headers http_headers;
    for (auto const & [key, value] : headers) {
        http_headers.emplace(key, value);
    }

    auto result = client.Post(
        json_value(path),
        http_headers,
        json_value(body),
        "application/json");

    if (not result) {
        auto err = result.error();
        return make_error("HTTP request failed: {}", httplib::to_string(err));
    }

    HttpResponse response;
    response.status = HttpStatusCode{result->status};
    response.body = HttpBody{result->body};

    for (auto const & [key, value] : result->headers) {
        response.headers.add(HeaderName{key}, HeaderValue{value});
    }

    return response;
}

void
HttpClient::
set_connection_timeout(TimeoutSeconds seconds)
{
    connection_timeout_ = seconds;
}

void
HttpClient::
set_read_timeout(TimeoutSeconds seconds)
{
    read_timeout_ = seconds;
}

} // namespace wjh::chat::client
