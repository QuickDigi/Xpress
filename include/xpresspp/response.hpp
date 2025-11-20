#pragma once
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <filesystem>

namespace xpresspp
{
    using json = nlohmann::json;

    class Response
    {
    public:
        Response()
            : statusCode(200), ended(false), contentType("text/plain; charset=utf-8"),
              compressionEnabled(false), streamingMode(false) {}

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
        bool sendFile(const std::string &path, const std::string &mime = "")
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

            // Auto-detect MIME type
            std::string mimeType = mime.empty() ? getMimeType(path) : mime;
            type(mimeType);

            // Set cache headers for static files
            setCacheHeaders(3600); // 1 hour default

            // Set Content-Length
            setHeader("Content-Length", std::to_string(body.size()));

            return true;
        }

        bool download(const std::string &path, const std::string &filename = "")
        {
            if (!sendFile(path))
                return false;

            std::string name = filename.empty() ? std::filesystem::path(path).filename().string() : filename;
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

        std::string getHeader(const std::string &key) const
        {
            auto it = headers.find(key);
            return it != headers.end() ? it->second : "";
        }

        bool hasHeader(const std::string &key) const
        {
            return headers.find(key) != headers.end();
        }

        void removeHeader(const std::string &key)
        {
            headers.erase(key);
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

        // 🔥 Advanced cookie with structured options
        struct CookieOptions
        {
            int maxAge = -1; // seconds
            std::string domain = "";
            std::string path = "/";
            bool secure = false;
            bool httpOnly = true;
            std::string sameSite = "Lax"; // Strict, Lax, None
        };

        void cookie(const std::string &name, const std::string &value, const CookieOptions &opts)
        {
            std::string cookieStr = name + "=" + value;

            if (opts.maxAge >= 0)
                cookieStr += "; Max-Age=" + std::to_string(opts.maxAge);

            if (!opts.domain.empty())
                cookieStr += "; Domain=" + opts.domain;

            if (!opts.path.empty())
                cookieStr += "; Path=" + opts.path;

            if (opts.secure)
                cookieStr += "; Secure";

            if (opts.httpOnly)
                cookieStr += "; HttpOnly";

            if (!opts.sameSite.empty())
                cookieStr += "; SameSite=" + opts.sameSite;

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
            body = "Redirecting to: " + url;
            type("text/plain; charset=utf-8");
        }

        void redirectBack(const std::string &defaultUrl = "/")
        {
            // This would need Request ref to get Referer
            redirect(defaultUrl);
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

        // ==========================================
        // 🔥 ENTERPRISE FEATURES
        // ==========================================

        // 🔥 Chainable status
        Response &statusChain(int code)
        {
            statusCode = code;
            return *this;
        }

        // 🔥 JSON with status code
        void json(int code, const nlohmann::json &data)
        {
            status(code);
            json(data);
        }

        // 🔥 Error responses with consistent format
        void error(int code, const std::string &message, const std::string &details = "")
        {
            status(code);
            nlohmann::json err = {
                {"error", true},
                {"status", code},
                {"message", message}};

            if (!details.empty())
                err["details"] = details;

            err["timestamp"] = getCurrentTimestamp();
            json(err);
        }

        // 🔥 Success response with data
        void success(const nlohmann::json &data, const std::string &message = "Success")
        {
            json({{"success", true},
                  {"message", message},
                  {"data", data}});
        }

        // 🔥 Paginated response
        void paginate(const nlohmann::json &items, int page, int limit, int total)
        {
            int totalPages = (total + limit - 1) / limit;
            json({{"success", true},
                  {"data", items},
                  {"pagination", {{"page", page}, {"limit", limit}, {"total", total}, {"totalPages", totalPages}, {"hasNext", page < totalPages}, {"hasPrev", page > 1}}}});
        }

        // 🔥 CORS Headers
        void cors(const std::string &origin = "*")
        {
            setHeader("Access-Control-Allow-Origin", origin);
            setHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            setHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
            setHeader("Access-Control-Allow-Credentials", "true");
        }

        void corsPreFlight()
        {
            cors();
            status(204);
            end();
        }

        // 🔥 Security Headers
        void securityHeaders()
        {
            setHeader("X-Content-Type-Options", "nosniff");
            setHeader("X-Frame-Options", "DENY");
            setHeader("X-XSS-Protection", "1; mode=block");
            setHeader("Strict-Transport-Security", "max-age=31536000; includeSubDomains");
            setHeader("Referrer-Policy", "strict-origin-when-cross-origin");
        }

        // 🔥 CSP (Content Security Policy)
        void csp(const std::string &policy)
        {
            setHeader("Content-Security-Policy", policy);
        }

        // 🔥 Cache Control
        void noCache()
        {
            setHeader("Cache-Control", "no-store, no-cache, must-revalidate, proxy-revalidate");
            setHeader("Pragma", "no-cache");
            setHeader("Expires", "0");
        }

        void cache(int seconds)
        {
            setHeader("Cache-Control", "public, max-age=" + std::to_string(seconds));
        }

        void setCacheHeaders(int maxAge, const std::string &etag = "")
        {
            setHeader("Cache-Control", "public, max-age=" + std::to_string(maxAge));

            if (!etag.empty())
                setHeader("ETag", "\"" + etag + "\"");

            // Last-Modified
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::tm tm = *std::gmtime(&now_c);

            std::ostringstream oss;
            oss << std::put_time(&tm, "%a, %d %b %Y %H:%M:%S GMT");
            setHeader("Last-Modified", oss.str());
        }

        // 🔥 ETag support
        void etag(const std::string &tag, bool weak = false)
        {
            std::string prefix = weak ? "W/" : "";
            setHeader("ETag", prefix + "\"" + tag + "\"");
        }

        // 🔥 Content negotiation
        void vary(const std::string &header)
        {
            append("Vary", header);
        }

        // 🔥 Rate limit headers
        void rateLimit(int limit, int remaining, int reset)
        {
            setHeader("X-RateLimit-Limit", std::to_string(limit));
            setHeader("X-RateLimit-Remaining", std::to_string(remaining));
            setHeader("X-RateLimit-Reset", std::to_string(reset));
        }

        // 🔥 Compression hint
        void enableCompression(bool enable = true)
        {
            compressionEnabled = enable;
        }

        // 🔥 Streaming response
        void stream(bool enable = true)
        {
            streamingMode = enable;
            if (enable)
                setHeader("Transfer-Encoding", "chunked");
        }

        // 🔥 Server timing API
        void addTiming(const std::string &name, double duration, const std::string &description = "")
        {
            std::string timing = name + ";dur=" + std::to_string(duration);
            if (!description.empty())
                timing += ";desc=\"" + description + "\"";

            append("Server-Timing", timing);
        }

        // 🔥 JSON-LD (Structured data)
        void jsonLD(const nlohmann::json &data)
        {
            std::string script = "<script type=\"application/ld+json\">" +
                                 data.dump() +
                                 "</script>";

            // Should be added to HTML head
            body += script;
        }

        // 🔥 XML Response
        void xml(const std::string &data)
        {
            type("application/xml; charset=utf-8");
            body = data;
        }

        // 🔥 Plain text
        void text(const std::string &data)
        {
            type("text/plain; charset=utf-8");
            body = data;
        }

        // 🔥 CSV Response
        void csv(const std::string &data, const std::string &filename = "data.csv")
        {
            type("text/csv; charset=utf-8");
            setHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
            body = data;
        }

        // 🔥 SSE (Server-Sent Events)
        void sse(const std::string &data, const std::string &event = "", const std::string &id = "")
        {
            type("text/event-stream");
            setHeader("Cache-Control", "no-cache");
            setHeader("Connection", "keep-alive");

            std::string sseData;
            if (!event.empty())
                sseData += "event: " + event + "\n";
            if (!id.empty())
                sseData += "id: " + id + "\n";

            sseData += "data: " + data + "\n\n";
            body = sseData;
        }

        // 🔥 Not Modified (304)
        void notModified()
        {
            status(304);
            body.clear();
            end();
        }

        // 🔥 Format response based on Accept header
        void format(const std::unordered_map<std::string, std::function<void()>> &formats,
                    const std::string &defaultFormat = "json")
        {
            // This would need Request ref to check Accept header
            // For now, use default
            if (formats.find(defaultFormat) != formats.end())
                formats.at(defaultFormat)();
        }

        // 🔥 Attachment header
        void attachment(const std::string &filename = "")
        {
            if (filename.empty())
                setHeader("Content-Disposition", "attachment");
            else
                setHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
        }

        // 🔥 Get response size
        size_t getSize() const
        {
            return body.size();
        }

        // 🔥 Check if response is sent
        bool isSent() const
        {
            return ended;
        }

        // 🔥 Reset response
        void reset()
        {
            body.clear();
            statusCode = 200;
            headers.clear();
            contentType = "text/plain; charset=utf-8";
            ended = false;
        }

        // 🔥 Render template (placeholder for template engine integration)
        void render(const std::string &view, const nlohmann::json &data = {})
        {
            // TODO: Integrate with template engine
            html("<html><body>View: " + view + "</body></html>");
        }

        // 🔥 JSONP support
        void jsonp(const nlohmann::json &data, const std::string &callback = "callback")
        {
            type("application/javascript; charset=utf-8");
            body = callback + "(" + data.dump() + ");";
        }

        // 🔥 Set multiple headers at once
        void setHeaders(const std::unordered_map<std::string, std::string> &headerMap)
        {
            for (const auto &[key, value] : headerMap)
                setHeader(key, value);
        }

        // 🔥 Location header (for 201 Created)
        void location(const std::string &url)
        {
            setHeader("Location", url);
        }

        // 🔥 Retry-After header
        void retryAfter(int seconds)
        {
            setHeader("Retry-After", std::to_string(seconds));
        }

        // 🔥 API versioning header
        void apiVersion(const std::string &version)
        {
            setHeader("X-API-Version", version);
        }

        // 🔥 Request ID for tracing
        void requestId(const std::string &id)
        {
            setHeader("X-Request-ID", id);
        }

        // 🔥 Get compression status
        bool isCompressionEnabled() const { return compressionEnabled; }
        bool isStreaming() const { return streamingMode; }

    private:
        std::string body;
        int statusCode;
        std::unordered_map<std::string, std::string> headers;
        std::string contentType;
        bool ended;
        bool compressionEnabled;
        bool streamingMode;

        // 🔥 MIME type detection
        std::string getMimeType(const std::string &path)
        {
            std::string ext = std::filesystem::path(path).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            static const std::unordered_map<std::string, std::string> mimeTypes = {
                // Text
                {".html", "text/html"},
                {".htm", "text/html"},
                {".css", "text/css"},
                {".js", "application/javascript"},
                {".json", "application/json"},
                {".xml", "application/xml"},
                {".txt", "text/plain"},
                {".csv", "text/csv"},

                // Images
                {".jpg", "image/jpeg"},
                {".jpeg", "image/jpeg"},
                {".png", "image/png"},
                {".gif", "image/gif"},
                {".svg", "image/svg+xml"},
                {".ico", "image/x-icon"},
                {".webp", "image/webp"},

                // Fonts
                {".woff", "font/woff"},
                {".woff2", "font/woff2"},
                {".ttf", "font/ttf"},
                {".otf", "font/otf"},

                // Audio/Video
                {".mp3", "audio/mpeg"},
                {".mp4", "video/mp4"},
                {".webm", "video/webm"},
                {".ogg", "audio/ogg"},

                // Documents
                {".pdf", "application/pdf"},
                {".zip", "application/zip"},
                {".tar", "application/x-tar"},
                {".gz", "application/gzip"}};

            auto it = mimeTypes.find(ext);
            return it != mimeTypes.end() ? it->second : "application/octet-stream";
        }

        // 🔥 Get current timestamp
        std::string getCurrentTimestamp()
        {
            auto now = std::chrono::system_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::tm tm = *std::gmtime(&now_c);

            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
            oss << "." << std::setfill('0') << std::setw(3) << ms.count() << "Z";
            return oss.str();
        }

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
            case 307:
                return "Temporary Redirect";
            case 308:
                return "Permanent Redirect";

            // 4xx
            case 400:
                return "Bad Request";
            case 401:
                return "Unauthorized";
            case 402:
                return "Payment Required";
            case 403:
                return "Forbidden";
            case 404:
                return "Not Found";
            case 405:
                return "Method Not Allowed";
            case 406:
                return "Not Acceptable";
            case 408:
                return "Request Timeout";
            case 409:
                return "Conflict";
            case 410:
                return "Gone";
            case 413:
                return "Payload Too Large";
            case 415:
                return "Unsupported Media Type";
            case 422:
                return "Unprocessable Entity";
            case 429:
                return "Too Many Requests";

            // 5xx
            case 500:
                return "Internal Server Error";
            case 501:
                return "Not Implemented";
            case 502:
                return "Bad Gateway";
            case 503:
                return "Service Unavailable";
            case 504:
                return "Gateway Timeout";
            }

            return "Unknown";
        }
    };
}