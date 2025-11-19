#pragma once
#include <functional>
#include <string>
#include <vector>
#include <iostream>
#include "request.hpp"
#include "response.hpp"

namespace xpresspp
{

    using Handler = std::function<void(Request &, Response &)>;

    struct Route
    {
        std::string method;
        std::string path;
        Handler handler;
    };

    class App
    {
    public:
        const std::vector<Route> &getRoutes() const
        {
            return routes;
        }

        App();

        // Routes
        void get(const std::string &path, Handler handler);
        void post(const std::string &path, Handler handler);
        void put(const std::string &path, Handler handler);
        void patch(const std::string &path, Handler handler);
        void del(const std::string &path, Handler handler);
        void all(const std::string &path, Handler handler);
        void options(const std::string &path, Handler handler);

        // Start server
        void listen(int port, std::function<void()> callback);

    private:
        std::vector<Route> routes;

        void addRoute(const std::string &method, const std::string &path, Handler handler);
        void handleRequest(const std::string &method, const std::string &path);
    };
}
