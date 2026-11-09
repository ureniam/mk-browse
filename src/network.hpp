#pragma once
#include <string>
#include <vector>

struct HttpResponse {
    int statusCode = 0;
    std::string contentType;
    std::string body;
    double elapsedMs = 0.0;
    std::string finalUrl;
    std::string errorMessage;
    bool success = false;
};

class NetworkEngine {
public:
    static void init();
    static void cleanup();

    static HttpResponse fetch(const std::string& url);
    static std::string normalizeUrl(const std::string& input);

private:
    static HttpResponse generateAboutPage(const std::string& page);
};
