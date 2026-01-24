#pragma once

/// C++ port of Zed's `http_client` crate.
/// Provides an abstract HTTP client interface for making network requests.

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace gpui {

/// HTTP methods supported by the client.
enum class HttpMethod {
    Get,
    Post,
    Put,
    Patch,
    Delete,
    Head,
    Options,
};

/// HTTP headers as a multimap of name -> value.
using HttpHeaders = std::map<std::string, std::string>;

/// An HTTP request to be sent.
struct HttpRequest {
    HttpMethod method = HttpMethod::Get;
    std::string url;
    HttpHeaders headers;
    std::vector<uint8_t> body;

    /// Builder methods for fluent construction.
    HttpRequest& set_method(HttpMethod m) { method = m; return *this; }
    HttpRequest& set_url(std::string u) { url = std::move(u); return *this; }
    HttpRequest& set_header(std::string name, std::string value) {
        headers[std::move(name)] = std::move(value);
        return *this;
    }
    HttpRequest& set_body(std::vector<uint8_t> b) { body = std::move(b); return *this; }
    HttpRequest& set_body(const std::string& b) {
        body.assign(b.begin(), b.end());
        return *this;
    }
};

/// An HTTP response received from the server.
struct HttpResponse {
    int status_code = 0;
    HttpHeaders headers;
    std::vector<uint8_t> body;

    bool is_success() const { return status_code >= 200 && status_code < 300; }

    std::string body_as_string() const {
        return std::string(body.begin(), body.end());
    }
};

/// Abstract HTTP client interface.
/// Implementations can use libcurl, WinHTTP, NSURLSession, etc.
class HttpClient {
public:
    virtual ~HttpClient() = default;

    /// Send a request synchronously and return the response.
    virtual HttpResponse send(const HttpRequest& request) = 0;

    /// Send a request asynchronously. The callback is invoked with the response.
    virtual void send_async(
        const HttpRequest& request,
        std::function<void(HttpResponse)> callback) {
        // Default implementation: blocking send on a separate concept
        // Real implementations should use async I/O.
        auto response = send(request);
        callback(std::move(response));
    }

    /// Convenience: GET request.
    HttpResponse get(const std::string& url) {
        HttpRequest req;
        req.method = HttpMethod::Get;
        req.url = url;
        return send(req);
    }

    /// Convenience: POST request with a body.
    HttpResponse post(const std::string& url, const std::string& body,
                      const std::string& content_type = "application/json") {
        HttpRequest req;
        req.method = HttpMethod::Post;
        req.url = url;
        req.set_header("Content-Type", content_type);
        req.set_body(body);
        return send(req);
    }
};

/// A no-op HTTP client for testing.
class NoopHttpClient : public HttpClient {
public:
    HttpResponse send(const HttpRequest&) override {
        return HttpResponse{200, {}, {}};
    }
};

/// Factory function to create the platform-appropriate HTTP client.
std::unique_ptr<HttpClient> create_http_client();

} // namespace gpui
