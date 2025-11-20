![Version](https://img.shields.io/badge/version-v2.0.0-blue?style=for-the-badge)
![License](https://img.shields.io/badge/license-MIT-green?style=for-the-badge)
![Build](https://img.shields.io/badge/build-passing-brightgreen?style=for-the-badge)
![Platform](https://img.shields.io/badge/platform-C++17-orange?style=for-the-badge)
![Status](https://img.shields.io/badge/status-stable-success?style=for-the-badge)
![Contributions](https://img.shields.io/badge/contributions-welcome-yellow?style=for-the-badge)
![Issues](https://img.shields.io/badge/issues-0-lightgrey?style=for-the-badge)
![Stars](https://img.shields.io/github/stars/QuickDigi/Xpress?style=for-the-badge)
![Forks](https://img.shields.io/github/forks/QuickDigi/Xpress?style=for-the-badge)

# 🚀 Xpress++ v2.0.0

**Modern C++ Web Framework — Fast, Lightweight & Developer-Friendly**

Xpress++ is a high-performance C++ web framework inspired by the simplicity of Express.js and the efficiency of modern C++ networking.
Built for speed, safety, and ease of use — without compromising advanced features.

---

## ✨ What's New in v2.0.0

🆕 Rewritten core with faster routing engine
🆕 New Request & Response API
🆕 Advanced Caching (ETag, Cache-Control, No-Cache)
🆕 Cookie Manager (set, read, clear, options)
🆕 Bearer Authentication Helpers
🆕 Mobile Detection + Content Negotiation
🆕 CSV, XML, JSONP, SSE support
🆕 File responses (inline + download)
🆕 Metrics + Server-Timing
🆕 Pagination helper
🆕 Pattern-matching routes
🆕 Unified error / success responses
🆕 Static security headers + CSP
🆕 Rate-limit headers
🆕 Full request introspection
🆕 Better redirect & status helpers
🆕 Trusted proxies support
🆕 New ServerConfig System

---

## 📦 Installation

```bash
git clone https://github.com/QuickDigi/Xpress.git
cd Xpress
mkdir build && cd build
cmake ..
make -j8
```

Or include it as a **header-only dependency** in your project.

---

## 🧩 Quick Example

```cpp
#include <xpresspp/app.hpp>
#include <xpresspp/server.hpp>

using namespace xpresspp;

int main() {
    App app;

    app.get("/", [](Request& req, Response& res){
        res.json({{"message", "Hello from Xpress++!"}});
    });

    ServerConfig config;
    config.port = 5000;

    Server server(app, config);
    server.run();
}
```

---

## 🌐 Features Overview

### 🔥 Core Features

- Super-fast routing
- Params, query, full URL parsing
- Pattern-based routes (`/user/:id`)
- JSON / HTML / Text responses
- `app.all("*")` for wildcard routes
- Request body parser (JSON)

### 🔐 Authentication

- `req.getBearerToken()`
- `req.isAuthenticated()`
- Token & cookie-based checks

### 🔏 Security

- Auto security headers
- CSP support (`res.csp()`)
- Proxy trust support
- CORS (`res.cors()`, `res.corsPreFlight()`)

### ⚡ Performance

- ETag support
- Cache-Control helpers
- Freshness validation
- Response compression-friendly

### 🧠 Content Negotiation

- `req.accepts("application/json")`
- Mobile detection
- XHR detection
- CSV, XML, JSONP support

### 🍪 Cookies

- Set/read/clear cookies
- Options: maxAge, secure, httpOnly, sameSite

### 📁 File Handling

- Inline file serving
- File download
- Attachments

### 📊 Monitoring

- Server metrics
- Server-Timing header
- Request duration
- Health endpoint

### 📦 API Helpers

- Pagination (`res.paginate()`)
- Error/Success formatters
- Rate-limit headers

---

## 📡 Advanced Endpoints in v2.0.0

### ✔ Bearer Authentication

```cpp
app.get("/auth/bearer", [](Request& req, Response& res){
    if (req.getBearerToken() != "secret-token-123")
        return res.error(403, "Invalid token");

    res.success({{"user","admin"}}, "Authenticated");
});
```

### ✔ Cookie Example

```cpp
res.cookie("session", "abc123", {
    .maxAge = 3600,
    .httpOnly = true,
    .secure = false,
    .sameSite = "Lax"
});
```

### ✔ File Download

```cpp
res.download("file.txt", "download.txt");
```

### ✔ SSE (Server-Sent Events)

```cpp
res.sse("Hello!", "update", "1");
```

---

## 📂 Project Structure (Recommended)

```
/src
  /routes
  /controllers
  /middleware
  main.cpp

/include/xpresspp
  app.hpp
  server.hpp
  request.hpp
  response.hpp

/examples
/docs
```

---

## 🛠 Server Configuration

```cpp
ServerConfig cfg;
cfg.host = "localhost";
cfg.port = 5000;
cfg.threadPoolSize = 8;
cfg.enableCORS = true;
cfg.enableMetrics = true;
cfg.maxRequestSize = 5 * 1024 * 1024; // 5MB
```

---

## 📈 Benchmark (v2.0.0)

| Test             | Result     |
| ---------------- | ---------- |
| JSON response    | ~52k req/s |
| HTML response    | ~48k req/s |
| File serving     | ~41k req/s |
| Routing (params) | ~62k req/s |

_Benchmarks run on AMD A8 R7 (same as dev-machine)._

---

## 📜 License

MIT — completely free for personal & commercial use.

---

## ❤️ Maintained by

**QuickDigi** — Egyptian tech innovator 🇪🇬
Developer: **Mohammed Mostafa Brawh**

---

## ⭐ Support

If you like Xpress++ →
**Star the repo ⭐**
It keeps the project alive!
