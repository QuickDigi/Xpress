#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <chrono>
#include <regex>

namespace xpresspp
{
    class Request
    {
    public:
        using json = nlohmann::json;

        std::string method;
        std::string url;
        std::string path;
        std::string body;
        std::string protocol;        // HTTP/1.1, HTTP/2
        std::string hostname;        // من Host header
        std::string originalUrl;     // الـ URL الأصلي قبل أي تعديل

        std::unordered_map<std::string, std::string> params;
        std::unordered_map<std::string, std::string> query;
        std::unordered_map<std::string, std::string> cookies;
        std::unordered_map<std::string, std::string> headers;

        std::string ip;
        std::vector<std::string> ips;  // X-Forwarded-For chain
        std::string userAgent;
        std::string referer;

        json jsonBody;

        // 🔥 Request metadata
        std::chrono::system_clock::time_point startTime;
        std::string requestId;  // Unique ID for tracking
        size_t contentLength = 0;

        // 🔥 Security & validation
        bool secure = false;    // HTTPS?
        std::string subdomains; // subdomain parsing

        // Constructor
        Request() : startTime(std::chrono::system_clock::now()) {}

        // -----------------------------
        // 🔥 Normalize URL (/user/?id=1)
        // -----------------------------
        static std::string cleanURL(const std::string &url)
        {
            if (url.empty()) return url;
            if (url.size() > 1 && url.back() == '/')
                return url.substr(0, url.size() - 1);
            return url;
        }

        // -------------------------------------------
        // 🔥 Parse query string (a=1&b=2&c=3)
        // -------------------------------------------
        void parseQuery(const std::string &queryStr)
        {
            query.clear();
            std::istringstream ss(queryStr);
            std::string pair;

            while (std::getline(ss, pair, '&'))
            {
                auto eq = pair.find('=');
                if (eq != std::string::npos)
                {
                    std::string key   = urlDecode(pair.substr(0, eq));
                    std::string value = urlDecode(pair.substr(eq + 1));
                    query[key] = value;
                }
            }
        }

        // ----------------------------------------------
        // 🔥 Automatic body parsing (JSON / x-www-form)
        // ----------------------------------------------
        void parseBody()
        {
            jsonBody = json::object();
            auto ctype = getHeader("Content-Type");

            // ---------------------------
            // JSON
            // ---------------------------
            if (ctype.find("application/json") != std::string::npos)
            {
                if (!body.empty() && (body[0] == '{' || body[0] == '['))
                {
                    try
                    {
                        jsonBody = json::parse(body);
                    }
                    catch (...)
                    {
                        jsonBody = json::object();
                    }
                }
                return;
            }

            // ---------------------------
            // URL-encoded (a=1&b=2)
            // ---------------------------
            if (ctype.find("application/x-www-form-urlencoded") != std::string::npos)
            {
                parseQuery(body);
                for (auto &[k, v] : query)
                {
                    jsonBody[k] = v;
                }
                return;
            }
        }

        // ---------------------------------------
        // 🔥 URL Decode
        // ---------------------------------------
        static std::string urlDecode(const std::string &src)
        {
            std::string out;
            out.reserve(src.size());

            for (size_t i = 0; i < src.size(); i++)
            {
                if (src[i] == '%' && i + 2 < src.size())
                {
                    std::string hex = src.substr(i + 1, 2);
                    char ch = static_cast<char>(strtol(hex.c_str(), nullptr, 16));
                    out += ch;
                    i += 2;
                }
                else if (src[i] == '+')
                {
                    out += ' ';
                }
                else
                {
                    out += src[i];
                }
            }
            return out;
        }

        // ---------------------------------------
        // 🔥 Param / Query / Cookie / Header Getters
        // ---------------------------------------
        std::string getParam(const std::string &key, const std::string &def = "") const
        {
            auto it = params.find(key);
            return it != params.end() ? it->second : def;
        }

        std::string getQuery(const std::string &key, const std::string &def = "") const
        {
            auto it = query.find(key);
            return it != query.end() ? it->second : def;
        }

        std::string getCookie(const std::string &key, const std::string &def = "") const
        {
            auto it = cookies.find(key);
            return it != cookies.end() ? it->second : def;
        }

        // 🔥 Header name becomes case-insensitive
        std::string getHeader(const std::string &key, const std::string &def = "") const
        {
            std::string lowerKey = key;
            std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);

            for (auto &kv : headers)
            {
                std::string h = kv.first;
                std::transform(h.begin(), h.end(), h.begin(), ::tolower);

                if (h == lowerKey)
                    return kv.second;
            }

            return def;
        }

        // ==========================================
        // 🔥 ENTERPRISE FEATURES
        // ==========================================

        // 🔥 Check if request accepts specific content type
        bool accepts(const std::string &type) const
        {
            std::string accept = getHeader("Accept");
            return accept.find(type) != std::string::npos || accept.find("*/*") != std::string::npos;
        }

        // 🔥 Check if request is AJAX/XHR
        bool isXHR() const
        {
            return getHeader("X-Requested-With") == "XMLHttpRequest";
        }

        // 🔥 Check if request is from mobile device
        bool isMobile() const
        {
            std::string ua = getHeader("User-Agent");
            std::transform(ua.begin(), ua.end(), ua.begin(), ::tolower);
            return ua.find("mobile") != std::string::npos ||
                   ua.find("android") != std::string::npos ||
                   ua.find("iphone") != std::string::npos;
        }

        // 🔥 Get content type
        std::string contentType() const
        {
            std::string ct = getHeader("Content-Type");
            auto pos = ct.find(';');
            return pos != std::string::npos ? ct.substr(0, pos) : ct;
        }

        // 🔥 Check if content type matches
        bool is(const std::string &type) const
        {
            return contentType().find(type) != std::string::npos;
        }

        // 🔥 Get base URL (protocol + host)
        std::string baseUrl() const
        {
            std::string proto = secure ? "https://" : "http://";
            return proto + hostname;
        }

        // 🔥 Get full URL with query string
        std::string fullUrl() const
        {
            std::string result = baseUrl() + path;
            if (!query.empty())
            {
                result += "?";
                bool first = true;
                for (auto &[k, v] : query)
                {
                    if (!first) result += "&";
                    result += k + "=" + v;
                    first = false;
                }
            }
            return result;
        }

        // 🔥 Parse X-Forwarded-For chain
        void parseForwardedIPs()
        {
            std::string forwarded = getHeader("X-Forwarded-For");
            if (forwarded.empty()) return;

            std::istringstream ss(forwarded);
            std::string ip_str;
            while (std::getline(ss, ip_str, ','))
            {
                // Trim whitespace
                ip_str.erase(0, ip_str.find_first_not_of(" \t"));
                ip_str.erase(ip_str.find_last_not_of(" \t") + 1);
                if (!ip_str.empty())
                    ips.push_back(ip_str);
            }

            // First IP is the real client IP
            if (!ips.empty() && ip.empty())
                ip = ips[0];
        }

        // 🔥 Get real client IP (handles proxies)
        std::string getRealIP() const
        {
            // Try various proxy headers
            std::vector<std::string> ipHeaders = {
                "X-Real-IP",
                "X-Forwarded-For",
                "CF-Connecting-IP",      // Cloudflare
                "True-Client-IP",        // Akamai, Cloudflare
                "X-Client-IP"
            };

            for (auto &header : ipHeaders)
            {
                std::string headerIp = getHeader(header);
                if (!headerIp.empty())
                {
                    // X-Forwarded-For can have multiple IPs
                    auto comma = headerIp.find(',');
                    if (comma != std::string::npos)
                        return headerIp.substr(0, comma);
                    return headerIp;
                }
            }

            return ip;
        }

        // 🔥 Parse cookies from Cookie header
        void parseCookies()
        {
            std::string cookieHeader = getHeader("Cookie");
            if (cookieHeader.empty()) return;

            std::istringstream ss(cookieHeader);
            std::string pair;

            while (std::getline(ss, pair, ';'))
            {
                // Trim whitespace
                pair.erase(0, pair.find_first_not_of(" \t"));
                pair.erase(pair.find_last_not_of(" \t") + 1);

                auto eq = pair.find('=');
                if (eq != std::string::npos)
                {
                    std::string key = pair.substr(0, eq);
                    std::string value = pair.substr(eq + 1);
                    cookies[key] = value;
                }
            }
        }

        // 🔥 Get request duration in milliseconds
        long long getDuration() const
        {
            auto now = std::chrono::system_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
        }

        // 🔥 Check if request has body
        bool hasBody() const
        {
            return !body.empty() || contentLength > 0;
        }

        // 🔥 Validate JSON body against expected fields
        bool validateJSON(const std::vector<std::string> &requiredFields) const
        {
            if (!jsonBody.is_object()) return false;

            for (auto &field : requiredFields)
            {
                if (!jsonBody.contains(field))
                    return false;
            }
            return true;
        }

        // 🔥 Get JSON field with type checking
        template<typename T>
        T getJSON(const std::string &key, const T &defaultValue = T()) const
        {
            try
            {
                if (jsonBody.contains(key))
                    return jsonBody[key].get<T>();
            }
            catch (...) {}
            return defaultValue;
        }

        // 🔥 Check if route matches pattern
        bool matchesRoute(const std::string &pattern) const
        {
            try
            {
                std::regex routeRegex(pattern);
                return std::regex_match(path, routeRegex);
            }
            catch (...)
            {
                return false;
            }
        }

        // 🔥 Extract subdomain
        std::string getSubdomain() const
        {
            auto pos = hostname.find('.');
            if (pos != std::string::npos && pos > 0)
            {
                std::string sub = hostname.substr(0, pos);
                // Exclude common cases
                if (sub != "www" && sub != "api")
                    return sub;
            }
            return "";
        }

        // 🔥 Check authentication
        bool isAuthenticated() const
        {
            return !getHeader("Authorization").empty() || 
                   !getCookie("token").empty() ||
                   !getCookie("session").empty();
        }

        // 🔥 Get Bearer token from Authorization header
        std::string getBearerToken() const
        {
            std::string auth = getHeader("Authorization");
            if (auth.find("Bearer ") == 0)
                return auth.substr(7);
            return "";
        }

        // 🔥 Get Basic Auth credentials
        std::pair<std::string, std::string> getBasicAuth() const
        {
            std::string auth = getHeader("Authorization");
            if (auth.find("Basic ") == 0)
            {
                // TODO: Decode base64 and split username:password
                return {"", ""};
            }
            return {"", ""};
        }

        // 🔥 Check if request is fresh (for caching)
        bool isFresh(const std::string &etag, const std::string &lastModified = "") const
        {
            std::string ifNoneMatch = getHeader("If-None-Match");
            if (!ifNoneMatch.empty() && ifNoneMatch == etag)
                return true;

            if (!lastModified.empty())
            {
                std::string ifModifiedSince = getHeader("If-Modified-Since");
                if (!ifModifiedSince.empty() && ifModifiedSince == lastModified)
                    return true;
            }

            return false;
        }

        // 🔥 Get query as JSON
        json getQueryJSON() const
        {
            json result = json::object();
            for (auto &[k, v] : query)
                result[k] = v;
            return result;
        }

        // 🔥 Get all data as JSON (params + query + body)
        json getAllData() const
        {
            json result = jsonBody.is_object() ? jsonBody : json::object();
            
            for (auto &[k, v] : params)
                result[k] = v;
            
            for (auto &[k, v] : query)
                if (!result.contains(k))
                    result[k] = v;
            
            return result;
        }

        // 🔥 Debug info
        std::string debug() const
        {
            std::ostringstream oss;
            oss << "=== Request Debug ===\n";
            oss << "Method: " << method << "\n";
            oss << "URL: " << fullUrl() << "\n";
            oss << "IP: " << getRealIP() << "\n";
            oss << "User-Agent: " << getHeader("User-Agent") << "\n";
            oss << "Content-Type: " << contentType() << "\n";
            oss << "Duration: " << getDuration() << "ms\n";
            oss << "===================\n";
            return oss.str();
        }
    };
}