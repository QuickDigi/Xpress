#include "xpresspp/app.hpp"

namespace xpresspp
{

    App::App() {}

    void App::addRoute(const std::string &method, const std::string &path, Handler handler)
    {
        routes.push_back({method, path, handler});
    }

    void App::get(const std::string &path, Handler handler)
    {
        addRoute("GET", path, handler);
    }

    void App::post(const std::string &path, Handler handler)
    {
        addRoute("POST", path, handler);
    }

    void App::put(const std::string &path, Handler handler)
    {
        addRoute("PUT", path, handler);
    }

    void App::patch(const std::string &path, Handler handler)
    {
        addRoute("PATCH", path, handler);
    }

    void App::del(const std::string &path, Handler handler)
    {
        addRoute("DELETE", path, handler);
    }

    void App::all(const std::string &path, Handler handler)
    {
        addRoute("ALL", path, handler);
    }

    void App::options(const std::string &path, Handler handler)
    {
        addRoute("OPTIONS", path, handler);
    }

    void App::listen(int port, std::function<void()> callback)
    {
        // TODO: socket/HTTP server
        std::cout << "🚀 Xpress++ listening on port " << port << std::endl;
        if (callback)
            callback();

        handleRequest("GET", "/");
    }

    void App::handleRequest(const std::string &method, const std::string &path)
    {
        for (auto &route : routes)
        {
            if (route.method == method && route.path == path)
            {
                Request req;
                Response res;
                route.handler(req, res);
                return;
            }
        }
        std::cout << "404 Not Found: " << path << std::endl;
    }
}
