#include "network.hpp"
#include <curl/curl.h>
#include <chrono>
#include <fstream>
#include <sstream>
#include <algorithm>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    std::string* s = static_cast<std::string*>(userp);
    s->append(static_cast<char*>(contents), total);
    return total;
}

void NetworkEngine::init() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void NetworkEngine::cleanup() {
    curl_global_cleanup();
}

std::string NetworkEngine::normalizeUrl(const std::string& input) {
    std::string trimmed = input;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

    if (trimmed.empty()) return "about:home";

    if (trimmed.rfind("about:", 0) == 0 ||
        trimmed.rfind("http://", 0) == 0 ||
        trimmed.rfind("https://", 0) == 0 ||
        trimmed.rfind("file://", 0) == 0) {
        return trimmed;
    }

    // If it has spaces or no dot, treat as search query
    if (trimmed.find(' ') != std::string::npos || trimmed.find('.') == std::string::npos) {
        return "https://html.duckduckgo.com/html/?q=" + trimmed;
    }

    return "https://" + trimmed;
}

HttpResponse NetworkEngine::generateAboutPage(const std::string& page) {
    HttpResponse res;
    res.statusCode = 200;
    res.contentType = "text/html";
    res.success = true;
    res.elapsedMs = 1.0;

    if (page == "about:blank") {
        res.finalUrl = "about:blank";
        res.body = "<!DOCTYPE html><html><head><title>Blank Tab</title></head><body></body></html>";
        return res;
    }

    if (page == "about:mkcorpo") {
        res.finalUrl = "about:mkcorpo";
        res.body = R"(<!DOCTYPE html>
<html>
<head>
    <title>About MKCorpo - MK-Browse</title>
</head>
<body style="font-family: sans-serif; background-color: #0f172a; color: #f8fafc; padding: 40px; margin: 0 auto; max-width: 800px;">
    <div style="text-align: center; margin-bottom: 30px;">
        <h1 style="color: #38bdf8; font-size: 32px; margin-bottom: 8px;">MKCorpo Technology Lab</h1>
        <p style="color: #94a3b8; font-size: 16px;">Next-Generation Lightweight Computing & Systems Architecture</p>
    </div>
    <hr style="border: 1px solid #334155; margin: 20px 0;">
    <div style="background-color: #1e293b; padding: 24px; border: 1px solid #334155;">
        <h2 style="color: #e2e8f0; font-size: 22px; margin-top: 0;">MK-Browse Engine Principles</h2>
        <p style="color: #cbd5e1; line-height: 1.6;">
            MK-Browse is an ultra-lightweight web browser crafted in pure modern C++17.
            It features <b>ZERO Chromium, ZERO WebKit, and ZERO Electron dependencies</b>.
        </p>
        <ul>
            <li style="color: #38bdf8;"><b>Memory Footprint:</b> Consumes under 20MB of RAM.</li>
            <li style="color: #38bdf8;"><b>Instant Startup:</b> Launches in sub-15ms.</li>
            <li style="color: #38bdf8;"><b>Custom Engine:</b> Built-in HTML Lexer, DOM builder, and Cairo/Pango typography renderer.</li>
            <li style="color: #38bdf8;"><b>OmniRoute Integration:</b> Designed in coordination with OmniRoute AI architecture.</li>
        </ul>
        <p style="margin-top: 20px;">
            <a href="about:home" style="color: #38bdf8; font-weight: bold;">Back to Speed Dial</a> | 
            <a href="https://github.com/ureniam" style="color: #38bdf8; font-weight: bold;">MKCorpo GitHub</a>
        </p>
    </div>
</body>
</html>)";
        return res;
    }

    // Default: about:home
    res.finalUrl = "about:home";
    res.body = R"(<!DOCTYPE html>
<html>
<head>
    <title>MKCorpo - Speed Dial</title>
</head>
<body style="background-color: #0b0f19; color: #e2e8f0; padding: 30px; font-family: sans-serif;">
    <div style="text-align: center; margin-bottom: 25px;">
        <h1 style="font-size: 34px; color: #38bdf8; margin-bottom: 6px;">MK-Browse</h1>
        <p style="color: #94a3b8; font-size: 15px;">Ultra-Lightweight C++ Engine &bull; Zero Chromium &bull; MKCorpo</p>
    </div>

    <div style="background-color: #161e2e; padding: 20px; border: 1px solid #2d3748; margin-bottom: 25px;">
        <h3 style="color: #f1f5f9; margin-top: 0;">Quick Navigation / Bookmarks</h3>
        <p>
            <a href="https://html.duckduckgo.com/html/" style="color: #60a5fa; font-weight: bold;">DuckDuckGo Search</a> &bull;
            <a href="https://news.ycombinator.com" style="color: #60a5fa; font-weight: bold;">Hacker News</a> &bull;
            <a href="https://en.wikipedia.org" style="color: #60a5fa; font-weight: bold;">Wikipedia</a> &bull;
            <a href="https://github.com/ureniam" style="color: #60a5fa; font-weight: bold;">MKCorpo GitHub</a> &bull;
            <a href="about:mkcorpo" style="color: #38bdf8; font-weight: bold;">About Engine</a>
        </p>
    </div>

    <div style="background-color: #161e2e; padding: 20px; border: 1px solid #2d3748;">
        <h3 style="color: #f1f5f9; margin-top: 0;">Browser Telemetry</h3>
        <ul>
            <li><b>Engine:</b> MK-Browse C++17 Core (Non-Chromium)</li>
            <li><b>Network Stack:</b> libcurl / OpenSSL Async Transport</li>
            <li><b>Graphics:</b> Cairo 2D Vector Rendering + Pango Typography</li>
            <li><b>Security:</b> Native sandbox, no bloated tracking scripts</li>
        </ul>
    </div>
</body>
</html>)";
    return res;
}

HttpResponse NetworkEngine::fetch(const std::string& url) {
    if (url.rfind("about:", 0) == 0) {
        return generateAboutPage(url);
    }

    if (url.rfind("file://", 0) == 0) {
        HttpResponse res;
        std::string path = url.substr(7);
        auto tStart = std::chrono::high_resolution_clock::now();
        std::ifstream file(path);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            res.body = buffer.str();
            res.statusCode = 200;
            res.success = true;
            res.contentType = "text/html";
            res.finalUrl = url;
        } else {
            res.statusCode = 404;
            res.success = false;
            res.errorMessage = "File not found: " + path;
            res.body = "<h1>404 Not Found</h1><p>Cannot open file: " + path + "</p>";
        }
        auto tEnd = std::chrono::high_resolution_clock::now();
        res.elapsedMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
        return res;
    }

    HttpResponse res;
    CURL* curl = curl_easy_init();
    if (!curl) {
        res.errorMessage = "Failed to initialize libcurl";
        res.statusCode = 500;
        return res;
    }

    auto tStart = std::chrono::high_resolution_clock::now();

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 8L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &res.body);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "MK-Browse/1.0 (MKCorpo; Linux x86_64; UltraLight)");
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, ""); // Support gzip/deflate automatically
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    CURLcode curlCode = curl_easy_perform(curl);
    auto tEnd = std::chrono::high_resolution_clock::now();
    res.elapsedMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();

    if (curlCode == CURLE_OK) {
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        res.statusCode = static_cast<int>(httpCode);

        char* ct = nullptr;
        if (curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct) == CURLE_OK && ct) {
            res.contentType = ct;
        }

        char* effUrl = nullptr;
        if (curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effUrl) == CURLE_OK && effUrl) {
            res.finalUrl = effUrl;
        } else {
            res.finalUrl = url;
        }
        res.success = (res.statusCode >= 200 && res.statusCode < 400);
    } else {
        res.statusCode = 0;
        res.success = false;
        res.errorMessage = curl_easy_strerror(curlCode);
        res.finalUrl = url;
        res.body = "<!DOCTYPE html><html><head><title>Connection Error</title></head>"
                   "<body style='font-family:sans-serif; padding:40px; background:#111; color:#eee;'>"
                   "<h1 style='color:#f87171;'>Connection Failed</h1>"
                   "<p>Unable to reach <b>" + url + "</b></p>"
                   "<p style='color:#94a3b8;'>Error: " + res.errorMessage + "</p>"
                   "<p><a href='about:home' style='color:#38bdf8;'>Go Home</a></p>"
                   "</body></html>";
    }

    curl_easy_cleanup(curl);
    return res;
}
