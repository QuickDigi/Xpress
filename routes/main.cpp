#include <xpresspp/app.hpp>
#include <xpresspp/server.hpp>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>

using json = nlohmann::json;
using namespace xpresspp;

int main()
{
#ifdef _WIN32
        system("chcp 65001 > nul");
#endif
        App app;

        // ============================================
        // 🎯 BASIC ROUTES
        // ============================================

        app.get("/", [](Request &req, Response &res)
                { res.html(R"(
            <!DOCTYPE html>
            <html>
            <head>
                <title>Xpress++ Demo</title>
                <style>
                    body { font-family: Arial; max-width: 1000px; margin: 50px auto; padding: 20px; }
                    h1 { color: #4CAF50; }
                    .endpoint { background: #f5f5f5; padding: 15px; margin: 10px 0; border-radius: 5px; }
                    .method { 
                        display: inline-block; 
                        padding: 5px 10px; 
                        border-radius: 3px; 
                        color: white;
                        font-weight: bold;
                        margin-right: 10px;
                    }
                    .get { background: #61AFFE; }
                    .post { background: #49CC90; }
                    .put { background: #FCA130; }
                    .delete { background: #F93E3E; }
                </style>
            </head>
            <body>
                <h1>🚀 Xpress++ Feature Demo</h1>
                <p>Welcome! Try these endpoints:</p>
                
                <h2>📊 Basic Features</h2>
                <div class="endpoint">
                    <span class="method get">GET</span>
                    <a href="/json">/json</a> - JSON Response
                </div>
                <div class="endpoint">
                    <span class="method get">GET</span>
                    <a href="/user/123">/user/:id</a> - URL Parameters
                </div>
                <div class="endpoint">
                    <span class="method get">GET</span>
                    <a href="/search?q=hello&page=2">/search?q=hello</a> - Query Strings
                </div>
                
                <h2>🔐 Authentication & Security</h2>
                <div class="endpoint">
                    <span class="method get">GET</span>
                    <a href="/auth/bearer">/auth/bearer</a> - Bearer Token Auth
                </div>
                <div class="endpoint">
                    <span class="method get">GET</span>
                    <a href="/secure">/secure</a> - Security Headers Demo
                </div>
                
                <h2>📱 Content Negotiation</h2>
                <div class="endpoint">
                    <span class="method get">GET</span>
                    <a href="/device-info">/device-info</a> - Mobile Detection
                </div>
                <div class="endpoint">
                    <span class="method get">GET</span>
                    <a href="/data.csv">/data.csv</a> - CSV Download
                </div>
                
                <h2>⚡ Performance & Caching</h2>
                <div class="endpoint">
                    <span class="method get">GET</span>
                    <a href="/cached">/cached</a> - Cache Headers
                </div>
                <div class="endpoint">
                    <span class="method get">GET</span>
                    <a href="/etag-demo">/etag-demo</a> - ETag Support
                </div>
                
                <h2>📊 API Features</h2>
                <div class="endpoint">
                    <span class="method get">GET</span>
                    <a href="/api/users?page=1&limit=10">/api/users</a> - Paginated Response
                </div>
                <div class="endpoint">
                    <span class="method post">POST</span>
                    /api/validate - JSON Validation
                </div>
                <div class="endpoint">
                    <span class="method get">GET</span>
                    <a href="/api/error-demo">/api/error-demo</a> - Error Handling
                </div>
                
                <h2>🍪 Cookies</h2>
                <div class="endpoint">
                    <span class="method get">GET</span>
                    <a href="/cookie/set">/cookie/set</a> - Set Cookie
                </div>
                <div class="endpoint">
                    <span class="method get">GET</span>
                    <a href="/cookie/read">/cookie/read</a> - Read Cookie
                </div>
                
                <h2>📈 Monitoring</h2>
                <div class="endpoint">
                    <span class="method get">GET</span>
                    <a href="/health">/health</a> - Health Check
                </div>
                <div class="endpoint">
                    <span class="method get">GET</span>
                    <a href="/metrics">/metrics</a> - Server Metrics
                </div>
                <div class="endpoint">
                    <span class="method get">GET</span>
                    <a href="/request-info">/request-info</a> - Request Details
                </div>
            </body>
            </html>
        )"); });

        // ============================================
        // 🔐 AUTHENTICATION & SECURITY
        // ============================================

        app.get("/auth/bearer", [](Request &req, Response &res)
                {
        std::string token = req.getBearerToken();
        
        if (token.empty())
        {
            res.error(401, "Missing Authorization token");
            return;
        }
        
        // Validate token (demo)
        if (token != "secret-token-123")
        {
            res.error(403, "Invalid token");
            return;
        }
        
        res.success({
            {"user", "john_doe"},
            {"role", "admin"},
            {"authenticated", true}
        }, "Authentication successful"); });

        app.get("/auth/check", [](Request &req, Response &res)
                {
        bool authenticated = req.isAuthenticated();
        
        res.json({
            {"authenticated", authenticated},
            {"hasToken", !req.getBearerToken().empty()},
            {"hasCookie", !req.getCookie("session").empty()}
        }); });

        app.get("/secure", [](Request &req, Response &res)
                {
        res.securityHeaders();
        res.csp("default-src 'self'; script-src 'self' 'unsafe-inline'");
        
        res.json({
            {"message", "This response has security headers"},
            {"headers_set", {
                "X-Content-Type-Options",
                "X-Frame-Options",
                "X-XSS-Protection",
                "Strict-Transport-Security",
                "Content-Security-Policy"
            }}
        }); });

        // ============================================
        // 📱 CONTENT NEGOTIATION & DEVICE DETECTION
        // ============================================

        app.get("/device-info", [](Request &req, Response &res)
                { res.json({{"isMobile", req.isMobile()},
                            {"isXHR", req.isXHR()},
                            {"userAgent", req.getHeader("User-Agent")},
                            {"accepts_json", req.accepts("application/json")},
                            {"accepts_html", req.accepts("text/html")},
                            {"contentType", req.contentType()}}); });

        app.get("/data.csv", [](Request &req, Response &res)
                {
        std::string csvData = 
            "Name,Age,Email\n"
            "John Doe,30,john@example.com\n"
            "Jane Smith,25,jane@example.com\n"
            "Bob Johnson,35,bob@example.com\n";
        
        res.csv(csvData, "users.csv"); });

        app.get("/data.xml", [](Request &req, Response &res)
                {
        std::string xmlData = 
            "<?xml version=\"2.0\" encoding=\"UTF-8\"?>\n"
            "<users>\n"
            "  <user><name>John</name><age>30</age></user>\n"
            "  <user><name>Jane</name><age>25</age></user>\n"
            "</users>";
        
        res.xml(xmlData); });

        // ============================================
        // ⚡ PERFORMANCE & CACHING
        // ============================================

        app.get("/cached", [](Request &req, Response &res)
                {
        // Cache for 1 hour
        res.cache(3600);
        res.vary("Accept-Encoding");
        
        res.json({
            {"message", "This response is cached"},
            {"timestamp", std::time(nullptr)},
            {"cache_max_age", "3600 seconds"}
        }); });

        app.get("/no-cache", [](Request &req, Response &res)
                {
        res.noCache();
        
        res.json({
            {"message", "This response should not be cached"},
            {"timestamp", std::time(nullptr)}
        }); });

        app.get("/etag-demo", [](Request &req, Response &res)
                {
        std::string etag = "v1.0.0";
        
        // Check if client has fresh copy
        if (req.isFresh(etag))
        {
            res.notModified();
            return;
        }
        
        res.etag(etag);
        res.cache(3600);
        
        res.json({
            {"message", "Content with ETag support"},
            {"version", etag},
            {"data", "Some expensive computed data"}
        }); });

        // ============================================
        // 📊 API FEATURES
        // ============================================

        app.get("/api/users", [](Request &req, Response &res)
                {
        int page = std::stoi(req.getQuery("page", "1"));
        int limit = std::stoi(req.getQuery("limit", "10"));
        
        // Simulate data
        json users = json::array();
        for (int i = 0; i < limit; i++)
        {
            users.push_back({
                {"id", (page - 1) * limit + i + 1},
                {"name", "User " + std::to_string(i + 1)},
                {"email", "user" + std::to_string(i + 1) + "@example.com"}
            });
        }
        
        // Total users (simulated)
        int total = 100;
        
        res.paginate(users, page, limit, total); });

        app.post("/api/validate", [](Request &req, Response &res)
                 {
        // Validate required fields
        std::vector<std::string> required = {"name", "email", "age"};
        
        if (!req.validateJSON(required))
        {
            res.error(400, "Missing required fields", "Required: name, email, age");
            return;
        }
        
        std::string name = req.getJSON<std::string>("name");
        std::string email = req.getJSON<std::string>("email");
        int age = req.getJSON<int>("age", 0);
        
        if (age < 18)
        {
            res.error(400, "Age must be 18 or older");
            return;
        }
        
        res.status(201);
        res.success({
            {"name", name},
            {"email", email},
            {"age", age}
        }, "User created successfully"); });

        app.get("/api/error-demo", [](Request &req, Response &res)
                { res.error(500, "Something went wrong", "This is a demo error response"); });

        app.get("/api/rate-limit", [](Request &req, Response &res)
                {
        // Simulate rate limiting
        res.rateLimit(100, 75, std::time(nullptr) + 3600);
        
        res.json({
            {"message", "Rate limit headers set"},
            {"limit", 100},
            {"remaining", 75},
            {"reset_in", "1 hour"}
        }); });

        // ============================================
        // 🍪 COOKIE MANAGEMENT
        // ============================================

        app.get("/cookie/set", [](Request &req, Response &res)
                {
        // Simple cookie
        res.cookie("simple", "value123");
        
        // Advanced cookie with options
        Response::CookieOptions opts;
        opts.maxAge = 3600;        // 1 hour
        opts.httpOnly = true;
        opts.secure = false;       // Set true in production with HTTPS
        opts.sameSite = "Lax";
        opts.path = "/";
        
        res.cookie("session", "abc-def-ghi", opts);
        
        res.json({
            {"message", "Cookies set successfully"},
            {"cookies", {"simple", "session"}}
        }); });

        app.get("/cookie/read", [](Request &req, Response &res)
                {
        std::string simple = req.getCookie("simple", "not found");
        std::string session = req.getCookie("session", "not found");
        
        res.json({
            {"simple", simple},
            {"session", session},
            {"all_cookies", req.cookies}
        }); });

        app.get("/cookie/clear", [](Request &req, Response &res)
                {
        res.clearCookie("simple");
        res.clearCookie("session");
        
        res.send("Cookies cleared!"); });

        // ============================================
        // 🌐 REQUEST INFORMATION
        // ============================================

        app.get("/request-info", [](Request &req, Response &res)
                { res.json({{"method", req.method},
                            {"path", req.path},
                            {"url", req.url},
                            {"fullUrl", req.fullUrl()},
                            {"baseUrl", req.baseUrl()},
                            {"protocol", req.protocol},
                            {"secure", req.secure},
                            {"ip", req.ip},
                            {"realIP", req.getRealIP()},
                            {"hostname", req.hostname},
                            {"subdomain", req.getSubdomain()},
                            {"userAgent", req.getHeader("User-Agent")},
                            {"referer", req.referer},
                            {"requestId", req.requestId},
                            {"duration_ms", req.getDuration()},
                            {"isMobile", req.isMobile()},
                            {"isXHR", req.isXHR()},
                            {"isAuthenticated", req.isAuthenticated()},
                            {"headers", req.headers},
                            {"query", req.query}}); });

        app.get("/debug", [](Request &req, Response &res)
                { res.text(req.debug()); });

        // ============================================
        // 🔄 REDIRECTS & STATUS
        // ============================================

        app.get("/redirect/temp", [](Request &req, Response &res)
                { res.redirect("/", 302); });

        app.get("/redirect/permanent", [](Request &req, Response &res)
                { res.redirect("/", 301); });

        app.get("/status/:code", [](Request &req, Response &res)
                {
        int code = std::stoi(req.getParam("code", "200"));
        res.sendStatus(code); });

        // ============================================
        // 📡 CORS & HEADERS
        // ============================================

        app.get("/cors", [](Request &req, Response &res)
                {
        res.cors();
        res.json({{"message", "CORS enabled for this response"}}); });

        app.options("/cors", [](Request &req, Response &res)
                    { res.corsPreFlight(); });

        app.get("/custom-headers", [](Request &req, Response &res)
                {
        res.setHeaders({
            {"X-Custom-Header", "CustomValue"},
            {"X-API-Version", "1.0.0"},
            {"X-Request-ID", req.requestId}
        });
        
        res.apiVersion("v1.0.0");
        res.requestId(req.requestId);
        
        res.json({{"message", "Custom headers set"}}); });

        // ============================================
        // 📂 FILE OPERATIONS
        // ============================================

        app.get("/file/text", [](Request &req, Response &res)
                {
        // Create demo file content
        std::string content = "Hello from Xpress++!\nThis is a text file.";
        res.text(content);
        res.attachment("demo.txt"); });

        app.get("/file/json-download", [](Request &req, Response &res)
                {
        json data = {
            {"users", json::array({
                {{"id", 1}, {"name", "John"}},
                {{"id", 2}, {"name", "Jane"}}
            })}
        };
        
        res.json(data);
        res.attachment("users.json"); });

        // ============================================
        // 🎭 SPECIAL RESPONSES
        // ============================================

        app.get("/sse", [](Request &req, Response &res)
                {
        // Server-Sent Events demo
        res.sse("First message", "update", "1"); });

        app.get("/jsonp", [](Request &req, Response &res)
                {
        std::string callback = req.getQuery("callback", "callback");
        
        json data = {
            {"message", "JSONP response"},
            {"timestamp", std::time(nullptr)}
        };
        
        res.jsonp(data, callback); });

        // ============================================
        // 🧪 ADVANCED FEATURES
        // ============================================

        app.post("/upload-simulation", [](Request &req, Response &res)
                 { res.json({{"message", "File upload received"},
                             {"body_size", req.body.size()},
                             {"content_length", req.contentLength},
                             {"content_type", req.contentType()}}); });

        app.get("/timing", [](Request &req, Response &res)
                {
        // Simulate processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        res.addTiming("db", 45.5, "Database query");
        res.addTiming("cache", 10.2, "Cache lookup");
        res.addTiming("render", 20.1, "Template render");
        
        res.json({
            {"message", "Check Server-Timing header"},
            {"total_duration", req.getDuration()}
        }); });

        app.get("/all-data", [](Request &req, Response &res)
                {
        // Get all data (params + query + body)
        json allData = req.getAllData();
        
        res.json({
            {"combined_data", allData},
            {"params", req.params},
            {"query", req.query}
        }); });

        // ============================================
        // 🎯 PATTERN MATCHING
        // ============================================

        app.get("/pattern/:type/:id", [](Request &req, Response &res)
                {
        std::string type = req.getParam("type");
        std::string id = req.getParam("id");
        
        res.json({
            {"type", type},
            {"id", id},
            {"pattern", "/pattern/:type/:id"}
        }); });

        // ---------------------------
        // 🔥 JSON Response
        // ---------------------------
        app.get("/json", [](Request &req, Response &res)
                { res.json({
                      {"message", "Hello JSON"},
                      {"status", true},
                      {"framework", "Xpress++"},
                  }); });

        // ---------------------------
        // 🔥 Params Example
        // /user/123
        // ---------------------------
        app.get("/user/:id", [](Request &req, Response &res)
                {
        std::string id = req.getParam("id");
        res.json({
            {"user_id", id},
            {"info", "Dynamic param works!"}
        }); });

        // ---------------------------
        // 🔥 Query Example
        // /search?q=hello&page=2
        // ---------------------------
        app.get("/search", [](Request &req, Response &res)
                {
        std::string q = req.getQuery("q", "none");
        std::string page = req.getQuery("page", "1");

        res.json({
            {"query", q},
            {"page", page}
        }); });

        // ---------------------------
        // 🔥 Cookies Example
        // ---------------------------
        app.get("/set-cookie", [](Request &req, Response &res)
                {
        res.cookie("session_id", "xyz123", "HttpOnly; Max-Age=3600");
        res.send("Cookie has been set!"); });

        app.get("/read-cookie", [](Request &req, Response &res)
                {
        auto session = req.getCookie("session_id", "none");
        res.send("Session ID: " + session); });

        // ---------------------------
        // 🔥 File Response
        // ---------------------------
        app.get("/file", [](Request &req, Response &res)
                {
        if (!res.sendFile("test.txt", "text/plain"))
            res.sendStatus(404); });

        // ---------------------------
        // 🔥 Download File
        // ---------------------------
        app.get("/download", [](Request &req, Response &res)
                {
        if (!res.download("test.txt", "downloaded.txt"))
            res.sendStatus(404); });

        // ---------------------------
        // 🔥 POST JSON Body
        // ---------------------------
        app.post("/post-json", [](Request &req, Response &res)
                 {
        auto body = req.jsonBody;
        res.json({
            {"received", body},
            {"status", "OK"}
        }); });

        // ---------------------------
        // 🔥 HTML Response
        // ---------------------------
        app.get("/html", [](Request &req, Response &res)
                { res.html("<h1 style='color:green'>Hello Xpress++!</h1>"); });

        // ---------------------------
        // 🔥 Redirect
        // ---------------------------
        app.get("/go", [](Request &req, Response &res)
                { res.redirect("/"); });

        // ---------------------------
        // 🔥 ALL Methods Route
        // ---------------------------
        app.all("/any", [](Request &req, Response &res)
                { res.json({{"method", req.method},
                            {"msg", "This route accepts ANY method!"}}); });

        // ---------------------------
        // 🔥 404 Example (custom)
        // ---------------------------
        app.get("/404", [](Request &req, Response &res)
                { res.sendStatus(404); });

        // ============================================
        // 🚀 SERVER CONFIGURATION & START
        // ============================================

        ServerConfig config;
        config.host = "localhost";
        config.port = 5000;
        config.threadPoolSize = 8;
        config.enableLogging = true;
        config.enableMetrics = true;
        config.enableCORS = true;
        config.trustProxy = true;
        config.readTimeout = 30;
        config.writeTimeout = 30;
        config.maxRequestSize = 5 * 1024 * 1024; // 5MB

        Server server(app, config);

        std::cout << "\n";
        std::cout << "💡 TIP: Open http://localhost:5000 in your browser\n";
        std::cout << "📚 All endpoints are documented on the homepage\n";
        std::cout << "\n";

        server.run();

        return 0;
}