# 🚀 Xpress++ — Modern C++ Web Framework  
A lightweight, expressive, and easy-to-use C++ web framework inspired by **Express.js**.

Xpress++ provides a clean routing system, JSON support, cookies, headers, file sending, 
redirects, middleware-ready architecture, and a powerful request/response API.

---

## ✨ Features
- ⚡ Simple routing (`GET`, `POST`, `PUT`, `DELETE`, `PATCH`, `OPTIONS`, `ALL`)
- 📦 JSON parsing & JSON responses
- 🔥 Dynamic route params (`/user/:id`)
- 🔍 Query parsing (`/search?q=hello&page=1`)
- 🍪 Cookies support
- 📂 File sending & downloading
- 🔁 Redirects
- 🧱 Clean Request / Response objects
- 🧬 Middleware-ready architecture
- 🧠 Easy, Express-like API
- 🛠 Built on top of `cpp-httplib`
- 🏎 Extremely fast, zero-dependency on runtime

---

## 📦 Installation

Clone the project:

```sh
git clone https://github.com/YourName/XpressPP.git
cd XpressPP
````

Build with g++:

```sh
g++ -std=c++17 routes/main.cpp xpress/src/*.cpp -Iinclude -lws2_32 -o server
```

Or using the provided build script:

```sh
start.bat
```

---

## 🧪 First Example (Hello World)

```cpp
#include <xpresspp/app.hpp>
#include <xpresspp/server.hpp>
using namespace xpresspp;

int main() {
    App app;

    app.get("/", [](Request &req, Response &res) {
        res.send("Nice, it's working! 🚀");
    });

    Server server(app, "localhost", 5000);
    server.run();
}
```

Run:

```
http://localhost:5000/
```

---

# 📚 Routing Examples

---

## 🔥 GET Route

```cpp
app.get("/", [](Request &req, Response &res) {
    res.send("Hello from Xpress++!");
});
```

---

## 🔥 Route Params

URL: `/user/42`

```cpp
app.get("/user/:id", [](Request &req, Response &res) {
    res.json({
        {"user_id", req.getParam("id")}
    });
});
```

---

## 🔍 Query Params

URL: `/search?q=test&page=10`

```cpp
app.get("/search", [](Request &req, Response &res) {
    res.json({
        {"query", req.getQuery("q")},
        {"page", req.getQuery("page")}
    });
});
```

---

## 📩 JSON POST Request

```cpp
app.post("/api/data", [](Request &req, Response &res) {
    res.json({
        {"received", req.jsonBody}
    });
});
```

---

## 🍪 Cookies

### Set Cookie:

```cpp
app.get("/set-cookie", [](Request &req, Response &res) {
    res.cookie("session_id", "ABC123", "HttpOnly; Max-Age=3600");
    res.send("Cookie set!");
});
```

### Read Cookie:

```cpp
app.get("/read-cookie", [](Request &req, Response &res) {
    res.send("Session: " + req.getCookie("session_id", "none"));
});
```

---

## 📂 Send File

```cpp
app.get("/file", [](Request &req, Response &res) {
    if (!res.sendFile("test.txt", "text/plain"))
        res.sendStatus(404);
});
```

---

## 📥 Download File

```cpp
app.get("/download", [](Request &req, Response &res) {
    res.download("test.txt", "myfile.txt");
});
```

---

## 🔁 Redirect

```cpp
app.get("/go-home", [](Request &req, Response &res) {
    res.redirect("/");
});
```

---

## 🔥 ALL Methods

```cpp
app.all("/any", [](Request &req, Response &res) {
    res.json({
        {"method", req.method},
        {"msg", "ALL methods allowed"}
    });
});
```

---

# 🏗 Project Structure

```
XpressPP/
│
├── include/
│   └── xpresspp/
│       ├── app.hpp
│       ├── server.hpp
│       ├── request.hpp
│       ├── response.hpp
│       ├── router.hpp
│       ├── utils.hpp
│       └── ...
│
├── xpress/
│   └── src/*.cpp
│
├── routes/
│   └── main.cpp
│
├── production/
│   └── server.exe
│
└── start.bat
```

---

# 🧠 Roadmap (Upcoming Features)

✔ Middlewares (`app.use()`)
✔ Static Files (`app.use("/public", static("public"))`)
✔ Express-style Router system
✔ Async handlers
✔ WebSocket support
✔ Error-handling middleware
✔ CORS middleware
✔ Logging middleware
✔ Template rendering (HTML + variables)

---

# 📜 License

MIT License © 2025 — Xpress++

---

# ⭐ Contributions

Pull Requests are welcome!
Feel free to fork the project & improve it.

```