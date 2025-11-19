#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <algorithm>
#include <nlohmann/json.hpp>

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

        std::unordered_map<std::string, std::string> params;
        std::unordered_map<std::string, std::string> query;
        std::unordered_map<std::string, std::string> cookies;
        std::unordered_map<std::string, std::string> headers;

        std::string ip;

        json jsonBody;

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
                return;
            }

            // multipart/form-data → هنعملها بعدين
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
    };
}
