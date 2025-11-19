#include <xpresspp/app.hpp>
#include <xpresspp/server.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace xpresspp;

int main()
{
        App app;

        // ---------------------------
        // 🔥 Basic GET
        // ---------------------------
        app.get("/", [](Request &req, Response &res)
                { res.send("Nice, it's working! 🚀"); });

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

        // ---------------------------
        // SERVER START
        // ---------------------------
        Server server(app, "localhost", 5000);
        server.run();

        return 0;
}
