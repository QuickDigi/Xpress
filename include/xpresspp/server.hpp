#pragma once
#include "app.hpp"
#include "httplib.h"
#include <string>
#include <iostream>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <iomanip>
#include <ctime>
#include <random>

inline std::unordered_map<std::string, std::string> parse_cookies(const std::string &cookieHeader)
{
    std::unordered_map<std::string, std::string> cookies;
    if (cookieHeader.empty())
        return cookies;

    std::istringstream stream(cookieHeader);
    std::string pair;

    while (std::getline(stream, pair, ';'))
    {
        pair.erase(pair.begin(), std::find_if(pair.begin(), pair.end(), [](unsigned char ch)
                                              { return !std::isspace(ch); }));

        auto eqPos = pair.find('=');
        if (eqPos != std::string::npos)
        {
            cookies[pair.substr(0, eqPos)] = pair.substr(eqPos + 1);
        }
    }
    return cookies;
}

inline std::unordered_map<std::string, std::string> extract_params(
    const std::string &routePath,
    const std::string &actualPath)
{
    std::unordered_map<std::string, std::string> params;

    std::istringstream r(routePath);
    std::istringstream a(actualPath);

    std::string rp, ap;
    while (std::getline(r, rp, '/') && std::getline(a, ap, '/'))
    {
        if (!rp.empty() && rp[0] == ':')
        {
            params[rp.substr(1)] = ap;
        }
    }
    return params;
}

namespace xpresspp
{
    // 🔥 Server Configuration
    struct ServerConfig
    {
        // Basic
        std::string host = "0.0.0.0";
        int port = 3000;
        int threadPoolSize = 8;

        // Timeouts
        int readTimeout = 30; // seconds
        int writeTimeout = 30;
        int keepAliveTimeout = 60;

        // Limits
        size_t maxRequestSize = 10 * 1024 * 1024; // 10MB
        size_t maxHeaderSize = 8 * 1024;          // 8KB
        int maxConnections = 1000;

        // Features
        bool enableLogging = true;
        bool enableMetrics = true;
        bool enableCORS = false;
        bool enableCompression = true;
        bool trustProxy = false;

        // SSL/TLS
        bool enableSSL = false;
        std::string sslCertPath = "";
        std::string sslKeyPath = "";

        // Performance
        bool reuseAddress = true;
        bool reusePort = false;
        bool tcpNoDelay = true;
    };

    // 🔥 Request Statistics
    struct RequestStats
    {
        std::atomic<uint64_t> totalRequests{0};
        std::atomic<uint64_t> successRequests{0};
        std::atomic<uint64_t> errorRequests{0};
        std::atomic<uint64_t> activeConnections{0};

        std::unordered_map<int, uint64_t> statusCodes;
        std::unordered_map<std::string, uint64_t> methodCounts;
        std::unordered_map<std::string, uint64_t> pathCounts;

        double avgResponseTime = 0.0;
        std::mutex statsMutex;

        void recordRequest(const std::string &method, const std::string &path,
                           int status, double duration)
        {
            std::lock_guard<std::mutex> lock(statsMutex);

            totalRequests++;

            if (status >= 200 && status < 400)
                successRequests++;
            else
                errorRequests++;

            statusCodes[status]++;
            methodCounts[method]++;
            pathCounts[path]++;

            // Update average response time (simple moving average)
            avgResponseTime = (avgResponseTime * (totalRequests - 1) + duration) / totalRequests;
        }
    };

    class Server
    {
    public:
        Server(App &app, const std::string &hostIp = "0.0.0.0", int port = 3000)
            : app_(app), config_()
        {
            config_.host = hostIp;
            config_.port = port;
            startTime_ = std::chrono::system_clock::now();
        }

        // 🔥 Constructor with config
        Server(App &app, const ServerConfig &config)
            : app_(app), config_(config)
        {
            startTime_ = std::chrono::system_clock::now();
        }

        // 🔥 Set configuration
        void setConfig(const ServerConfig &config)
        {
            config_ = config;
        }

        ServerConfig &config()
        {
            return config_;
        }

        // 🔥 Get statistics
        const RequestStats &getStats() const
        {
            return stats_;
        }

        // 🔥 Enhanced run with features
        void run()
        {
            httplib::Server svr;

            // ========================================
            // 🔥 Server Configuration
            // ========================================

            // Timeouts
            svr.set_read_timeout(config_.readTimeout, 0);
            svr.set_write_timeout(config_.writeTimeout, 0);
            svr.set_keep_alive_timeout(config_.keepAliveTimeout);

            // Limits
            svr.set_payload_max_length(config_.maxRequestSize);

            // Thread pool
            svr.new_task_queue = [this]
            {
                return new httplib::ThreadPool(config_.threadPoolSize);
            };

            // ========================================
            // 🔥 Global Middleware (Logger, CORS, etc.)
            // ========================================

            // Logger middleware
            if (config_.enableLogging)
            {
                svr.set_logger([this](const httplib::Request &req, const httplib::Response &res)
                               { logRequest(req, res); });
            }

            // Pre-routing middleware (CORS, etc.)
            svr.set_pre_routing_handler([this](const httplib::Request &req, httplib::Response &res)
                                        {
                stats_.activeConnections++;
                
                // CORS
                if (config_.enableCORS)
                {
                    res.set_header("Access-Control-Allow-Origin", "*");
                    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS, PATCH");
                    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
                    res.set_header("Access-Control-Allow-Credentials", "true");
                    
                    if (req.method == "OPTIONS")
                    {
                        res.status = 204;
                        return httplib::Server::HandlerResponse::Handled;
                    }
                }
                
                return httplib::Server::HandlerResponse::Unhandled; });

            // Post-routing handler (cleanup)
            svr.set_post_routing_handler([this](const httplib::Request &, httplib::Response &)
                                         { stats_.activeConnections--; });

            // ========================================
            // 🔥 Error Handlers
            // ========================================

            svr.set_error_handler([](const httplib::Request &, httplib::Response &res)
                                  {
                nlohmann::json error = {
                    {"error", true},
                    {"status", res.status},
                    {"message", getStatusMessage(res.status)}
                };
                
                res.set_content(error.dump(), "application/json"); });

            svr.set_exception_handler([this](const httplib::Request &req, httplib::Response &res, std::exception_ptr ep)
                                      {
                std::string error_msg = "Internal Server Error";
                
                try
                {
                    std::rethrow_exception(ep);
                }
                catch (std::exception &e)
                {
                    error_msg = e.what();
                    std::cerr << "[Exception] " << req.method << " " << req.path 
                             << " - " << error_msg << std::endl;
                }
                catch (...)
                {
                    std::cerr << "[Unknown Exception] " << req.method << " " << req.path << std::endl;
                }
                
                nlohmann::json errorJson = {
                    {"error", true},
                    {"status", 500},
                    {"message", error_msg}
                };
                
                res.status = 500;
                res.set_content(errorJson.dump(), "application/json"); });

            // ========================================
            // 🔥 Register Routes
            // ========================================

            for (auto &route : app_.getRoutes())
            {
                auto handler = [&, route](const httplib::Request &req, httplib::Response &res)
                {
                    auto startTime = std::chrono::high_resolution_clock::now();

                    try
                    {
                        Request xreq;

                        // ========================================
                        // 🔥 Build Request Object
                        // ========================================

                        // Basic info
                        xreq.method = req.method;
                        xreq.url = req.path;
                        xreq.path = req.path;
                        xreq.originalUrl = req.path;
                        xreq.protocol = req.has_header("X-Forwarded-Proto")
                                            ? req.get_header_value("X-Forwarded-Proto")
                                            : "http";

                        // Generate unique request ID
                        xreq.requestId = generateRequestId();
                        xreq.startTime = std::chrono::system_clock::now();

                        // Query string
                        if (!req.params.empty())
                        {
                            std::ostringstream qs;
                            bool first = true;
                            for (const auto &p : req.params)
                            {
                                if (!first)
                                    qs << '&';
                                first = false;
                                qs << p.first << '=' << p.second;
                            }
                            xreq.parseQuery(qs.str());
                        }

                        // Body
                        xreq.body = req.body;
                        xreq.contentLength = req.body.size();

                        // Route params
                        xreq.params = extract_params(route.path, req.path);

                        // Cookies
                        auto ck = req.headers.find("Cookie");
                        if (ck != req.headers.end())
                        {
                            xreq.cookies = parse_cookies(ck->second);
                        }

                        // Headers
                        for (auto &h : req.headers)
                            xreq.headers[h.first] = h.second;

                        // Extract common headers
                        xreq.userAgent = req.get_header_value("User-Agent");
                        xreq.referer = req.get_header_value("Referer");
                        xreq.hostname = req.get_header_value("Host");

                        // IP handling (with proxy support)
                        if (config_.trustProxy)
                        {
                            xreq.ip = req.get_header_value("X-Forwarded-For");
                            if (xreq.ip.empty())
                                xreq.ip = req.get_header_value("X-Real-IP");
                            if (xreq.ip.empty())
                                xreq.ip = req.remote_addr;

                            xreq.parseForwardedIPs();
                        }
                        else
                        {
                            xreq.ip = req.remote_addr;
                        }

                        // HTTPS detection
                        xreq.secure = (xreq.protocol == "https") ||
                                      req.get_header_value("X-Forwarded-Ssl") == "on";

                        // Parse body
                        xreq.parseBody();

                        // Parse cookies
                        xreq.parseCookies();

                        // ========================================
                        // 🔥 Create Response Object
                        // ========================================

                        Response xres;
                        xres.requestId(xreq.requestId);

                        // Add server header
                        xres.setHeader("X-Powered-By", "Xpress++");

                        // Security headers (if enabled globally)
                        if (config_.enableCORS)
                            xres.cors();

                        // ========================================
                        // 🔥 Execute Handler
                        // ========================================

                        route.handler(xreq, xres);

                        // ========================================
                        // 🔥 Build HTTP Response
                        // ========================================

                        res.status = xres.getStatus();

                        for (auto &h : xres.getHeaders())
                            res.set_header(h.first.c_str(), h.second.c_str());

                        res.set_content(xres.getBody(), xres.getContentType().c_str());

                        // ========================================
                        // 🔥 Record Metrics
                        // ========================================

                        if (config_.enableMetrics)
                        {
                            auto endTime = std::chrono::high_resolution_clock::now();
                            auto duration = std::chrono::duration<double, std::milli>(endTime - startTime).count();

                            stats_.recordRequest(req.method, req.path, res.status, duration);

                            // Add timing header
                            res.set_header("X-Response-Time", std::to_string(duration) + "ms");
                        }
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "[Server Error] " << req.method << " " << req.path
                                  << " - " << e.what() << std::endl;

                        nlohmann::json errorJson = {
                            {"error", true},
                            {"status", 500},
                            {"message", "Internal Server Error"},
                            {"details", e.what()}};

                        res.status = 500;
                        res.set_content(errorJson.dump(), "application/json");

                        if (config_.enableMetrics)
                            stats_.errorRequests++;
                    }
                };

                // ========================================
                // 🔥 Register Route by Method
                // ========================================

                if (route.method == "GET")
                    svr.Get(route.path.c_str(), handler);
                else if (route.method == "POST")
                    svr.Post(route.path.c_str(), handler);
                else if (route.method == "PUT")
                    svr.Put(route.path.c_str(), handler);
                else if (route.method == "PATCH")
                    svr.Patch(route.path.c_str(), handler);
                else if (route.method == "DELETE")
                    svr.Delete(route.path.c_str(), handler);
                else if (route.method == "OPTIONS")
                    svr.Options(route.path.c_str(), handler);
                else if (route.method == "ALL")
                {
                    svr.Get(route.path.c_str(), handler);
                    svr.Post(route.path.c_str(), handler);
                    svr.Put(route.path.c_str(), handler);
                    svr.Patch(route.path.c_str(), handler);
                    svr.Delete(route.path.c_str(), handler);
                    svr.Options(route.path.c_str(), handler);
                }
            }

            // ========================================
            // 🔥 Built-in Routes (Health, Metrics, etc.)
            // ========================================

            // Health check endpoint
            svr.Get("/health", [this](const httplib::Request &, httplib::Response &res)
                    {
                auto uptime = getUptime();
                
                nlohmann::json health = {
                    {"status", "healthy"},
                    {"uptime", uptime},
                    {"timestamp", getCurrentTimestamp()},
                    {"activeConnections", stats_.activeConnections.load()}
                };
                
                res.set_content(health.dump(), "application/json"); });

            // Metrics endpoint
            if (config_.enableMetrics)
            {
                svr.Get("/metrics", [this](const httplib::Request &, httplib::Response &res)
                        {
                    nlohmann::json metrics = getMetricsJSON();
                    res.set_content(metrics.dump(), "application/json"); });
            }

            // ========================================
            // 🔥 Start Server
            // ========================================

            printStartupBanner();

            if (config_.enableSSL && !config_.sslCertPath.empty())
            {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
                std::cout << "🔒 SSL/TLS enabled\n";
                svr.listen(config_.host.c_str(), config_.port,
                           config_.sslCertPath.c_str(), config_.sslKeyPath.c_str());
#else
                std::cerr << "⚠️  SSL/TLS requested but cpp-httplib was built without OpenSSL support; falling back to HTTP\n";
                svr.listen(config_.host.c_str(), config_.port);
#endif
            }
            else
            {
                svr.listen(config_.host.c_str(), config_.port);
            }
        }

        // ========================================
        // 🔥 Graceful Shutdown
        // ========================================

        void shutdown()
        {
            std::cout << "\n🛑 Shutting down server gracefully...\n";

            // Wait for active connections to complete
            int maxWait = 10; // seconds
            int waited = 0;

            while (stats_.activeConnections > 0 && waited < maxWait)
            {
                std::cout << "⏳ Waiting for " << stats_.activeConnections
                          << " active connections...\n";
                std::this_thread::sleep_for(std::chrono::seconds(1));
                waited++;
            }

            printShutdownStats();
            std::cout << "✅ Server stopped\n";
        }

    private:
        App &app_;
        ServerConfig config_;
        RequestStats stats_;
        std::chrono::system_clock::time_point startTime_;

        // ========================================
        // 🔥 Helper Methods
        // ========================================

        void printStartupBanner()
        {
            std::cout << "\n";
            std::cout << "╔════════════════════════════════════════╗\n";
            std::cout << "║         🚀 Xpress++ Server v2.0        ║\n";
            std::cout << "╚════════════════════════════════════════╝\n";
            std::cout << "\n";
            std::cout << "📡 Protocol:  " << (config_.enableSSL ? "HTTPS" : "HTTP") << "\n";
            std::cout << "🌐 Host:      " << config_.host << "\n";
            std::cout << "🔌 Port:      " << config_.port << "\n";
            std::cout << "👥 Threads:   " << config_.threadPoolSize << "\n";
            std::cout << "📊 Logging:   " << (config_.enableLogging ? "✓" : "✗") << "\n";
            std::cout << "📈 Metrics:   " << (config_.enableMetrics ? "✓" : "✗") << "\n";
            std::cout << "🔐 CORS:      " << (config_.enableCORS ? "✓" : "✗") << "\n";
            std::cout << "\n";

            std::cout << "📍 Endpoints:\n";
            std::cout << "   • " << (config_.enableSSL ? "https://" : "http://")
                      << config_.host << ":" << config_.port << "\n";
            std::cout << "   • Health: /health\n";
            if (config_.enableMetrics)
                std::cout << "   • Metrics: /metrics\n";
            std::cout << "\n";

            std::cout << "🎯 Registered Routes: " << app_.getRoutes().size() << "\n";
            for (auto &route : app_.getRoutes())
            {
                std::cout << "   " << std::setw(7) << std::left << route.method
                          << " " << route.path << "\n";
            }

            std::cout << "\n";
            std::cout << "✨ Server is ready and listening...\n";
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
        }

        void printShutdownStats()
        {
            std::cout << "\n📊 Final Statistics:\n";
            std::cout << "   Total Requests:   " << stats_.totalRequests << "\n";
            std::cout << "   Success:          " << stats_.successRequests << "\n";
            std::cout << "   Errors:           " << stats_.errorRequests << "\n";
            std::cout << "   Avg Response:     " << std::fixed << std::setprecision(2)
                      << stats_.avgResponseTime << "ms\n";
            std::cout << "   Uptime:           " << getUptime() << "\n\n";
        }

        void logRequest(const httplib::Request &req, const httplib::Response &res)
        {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);

            std::cout << std::put_time(std::localtime(&time), "[%Y-%m-%d %H:%M:%S]")
                      << " " << std::setw(7) << std::left << req.method
                      << " " << std::setw(40) << std::left << req.path
                      << " " << getStatusColor(res.status) << res.status << "\033[0m"
                      << " - " << req.remote_addr
                      << "\n";
        }

        std::string getStatusColor(int status)
        {
            if (status >= 200 && status < 300)
                return "\033[32m"; // Green
            if (status >= 300 && status < 400)
                return "\033[36m"; // Cyan
            if (status >= 400 && status < 500)
                return "\033[33m"; // Yellow
            if (status >= 500)
                return "\033[31m"; // Red
            return "\033[0m";      // Reset
        }

        static std::string getStatusMessage(int status)
        {
            switch (status)
            {
            case 200:
                return "OK";
            case 201:
                return "Created";
            case 204:
                return "No Content";
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
            case 500:
                return "Internal Server Error";
            case 503:
                return "Service Unavailable";
            default:
                return "Unknown Status";
            }
        }

        std::string generateRequestId()
        {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<> dis(0, 15);

            const char *hex = "0123456789abcdef";
            std::string id;
            id.reserve(32);

            for (int i = 0; i < 32; i++)
                id += hex[dis(gen)];

            return id;
        }

        std::string getCurrentTimestamp()
        {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::ostringstream oss;
            oss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");
            return oss.str();
        }

        std::string getUptime()
        {
            auto now = std::chrono::system_clock::now();
            auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - startTime_).count();

            int days = uptime / 86400;
            int hours = (uptime % 86400) / 3600;
            int minutes = (uptime % 3600) / 60;
            int seconds = uptime % 60;

            std::ostringstream oss;
            if (days > 0)
                oss << days << "d ";
            if (hours > 0)
                oss << hours << "h ";
            if (minutes > 0)
                oss << minutes << "m ";
            oss << seconds << "s";

            return oss.str();
        }

        nlohmann::json getMetricsJSON()
        {
            std::lock_guard<std::mutex> lock(stats_.statsMutex);

            nlohmann::json metrics = {
                {"uptime", getUptime()},
                {"timestamp", getCurrentTimestamp()},
                {"requests", {{"total", stats_.totalRequests.load()}, {"success", stats_.successRequests.load()}, {"error", stats_.errorRequests.load()}, {"active", stats_.activeConnections.load()}}},
                {"performance", {{"avgResponseTime", stats_.avgResponseTime}}},
                {"statusCodes", stats_.statusCodes},
                {"methods", stats_.methodCounts},
                {"topPaths", stats_.pathCounts}};

            return metrics;
        }
    };
}