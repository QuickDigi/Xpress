#pragma once
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

namespace xpresspp
{
    using json = nlohmann::json;

    class Response
    {
    public:
        Response()
            : statusCode(200), ended(false), contentType("text/plain; charset=utf-8") {}

        // ------------------------------
        // 🔥 BASIC SEND
        // ------------------------------
        void send(const std::string &data)
        {
            body = data;
            type("text/plain; charset=utf-8");
        }

        // Send raw bytes (e.g. images)
        void send(const std::vector<uint8_t> &buffer)
        {
            body.assign(buffer.begin(), buffer.end());
            // Keep content-type as-is
        }

        // ------------------------------
        // 🔥 JSON
        // ------------------------------
        void json(const nlohmann::json &data)
        {
            type("application/json; charset=utf-8");
            body = data.dump();
        }

        void json(const std::initializer_list<std::pair<std::string, nlohmann::json>> &list)
        {
            nlohmann::json j = nlohmann::json::object();
            for (auto &p : list)
                j[p.first] = p.second;
            json(j);
        }

        // ------------------------------
        // 🔥 HTML
        // ------------------------------
        void html(const std::string &data)
        {
            type("text/html; charset=utf-8");
            body = data;
        }

        // ------------------------------
        // 🔥 FILE SENDING
        // ------------------------------
        bool sendFile(const std::string &path, const std::string &mime = "application/octet-stream")
        {
            std::ifstream file(path, std::ios::binary);
            if (!file.is_open())
            {
                status(404);
                body = "File Not Found";
                return false;
            }

            std::ostringstream buffer;
            buffer << file.rdbuf();
            body = buffer.str();

            type(mime);
            return true;
        }

        bool download(const std::string &path, const std::string &filename = "")
        {
            if (!sendFile(path))
                return false;

            std::string name = filename.empty() ? path : filename;
            setHeader("Content-Disposition", "attachment; filename=\"" + name + "\"");
            return true;
        }

        // ------------------------------
        // 🔥 STATUS
        // ------------------------------
        void status(int code)
        {
            statusCode = code;
        }

        void sendStatus(int code)
        {
            statusCode = code;
            body = std::to_string(code) + " " + getStatusText(code);
            type("text/plain; charset=utf-8");
        }

        // ------------------------------
        // 🔥 HEADERS
        // ------------------------------
        void setHeader(const std::string &key, const std::string &value)
        {
            headers[key] = value;
        }

        void append(const std::string &key, const std::string &value)
        {
            if (headers.find(key) != headers.end())
                headers[key] += ", " + value;
            else
                headers[key] = value;
        }

        void type(const std::string &mime)
        {
            contentType = mime;
            headers["Content-Type"] = mime;
        }

        // ------------------------------
        // 🔥 COOKIES
        // ------------------------------
        void cookie(const std::string &name, const std::string &value, const std::string &options = "")
        {
            std::string cookieStr = name + "=" + value;
            if (!options.empty())
                cookieStr += "; " + options;

            append("Set-Cookie", cookieStr);
        }

        void clearCookie(const std::string &name)
        {
            cookie(name, "", "Expires=Thu, 01 Jan 1970 00:00:00 GMT; Max-Age=0");
        }

        // ------------------------------
        // 🔥 REDIRECT
        // ------------------------------
        void redirect(const std::string &url, int code = 302)
        {
            status(code);
            setHeader("Location", url);

            // body for non-browser tools
            body = "Redirecting to: " + url;
            type("text/plain; charset=utf-8");
        }

        // ------------------------------
        // 🔥 LINKS HEADER (HTTP/2 Friendly)
        // ------------------------------
        void links(const std::unordered_map<std::string, std::string> &linkMap)
        {
            std::string header;
            for (const auto &[rel, href] : linkMap)
            {
                if (!header.empty())
                    header += ", ";
                header += "<" + href + ">; rel=\"" + rel + "\"";
            }
            setHeader("Link", header);
        }

        // ------------------------------
        // 🔥 END
        // ------------------------------
        void end(const std::string &data = "")
        {
            if (!data.empty())
                body = data;

            ended = true;
        }

        // ------------------------------
        // 🔥 GETTERS
        // ------------------------------
        const std::string &getBody() const { return body; }
        int getStatus() const { return statusCode; }
        const std::unordered_map<std::string, std::string> &getHeaders() const { return headers; }
        const std::string &getContentType() const { return contentType; }
        bool isEnded() const { return ended; }

    private:
        std::string body;
        int statusCode;
        std::unordered_map<std::string, std::string> headers;
        std::string contentType;
        bool ended;

        // ------------------------------
        // 🔥 STATUS TEXT LOOKUP
        // ------------------------------
        std::string getStatusText(int code)
        {
            switch (code)
            {
            // 1xx
            case 100:
                return "Continue";
            case 101:
                return "Switching Protocols";
            case 102:
                return "Processing";
            case 103:
                return "Early Hints";

            // 2xx
            case 200:
                return "OK";
            case 201:
                return "Created";
            case 202:
                return "Accepted";
            case 203:
                return "Non-Authoritative Information";
            case 204:
                return "No Content";
            case 205:
                return "Reset Content";
            case 206:
                return "Partial Content";

            // 3xx
            case 300:
                return "Multiple Choices";
            case 301:
                return "Moved Permanently";
            case 302:
                return "Found";
            case 303:
                return "See Other";
            case 304:
                return "Not Modified";

            // 4xx
            case 400:
                return "Bad Request";
            case 401:
                return "Unauthorized";
            case 403:
                return "Forbidden";
            case 404:
                return "Not Found";
            case 405:
                return "Method Not Allowed";
            case 409:
                return "Conflict";

            // 5xx
            case 500:
                return "Internal Server Error";
            case 501:
                return "Not Implemented";
            case 503:
                return "Service Unavailable";
            }

            return "Unknown";
        }
    };
}
