#pragma once
#include "app.hpp"
#include "httplib.h"
#include <string>
#include <iostream>
#include <unordered_map>
#include <sstream>
#include <algorithm>

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
    class Server
    {
    public:
        Server(App &app, const std::string &hostIp, int port)
            : app_(app), hostIp_(hostIp), port_(port) {}

        void run()
        {
            httplib::Server svr;

            for (auto &route : app_.getRoutes())
            {
                // --------------------
                // 🔥 Basic handler
                // --------------------
                auto handler = [&, route](const httplib::Request &req, httplib::Response &res)
                {
                    try
                    {
                        Request xreq;

                        // Method
                        xreq.method = req.method;

                        // URL + PATH
                        xreq.url = req.path;
                        xreq.path = req.path;

                        // Build query string from parsed params (httplib::Request does not have query_string)
                        if (!req.params.empty())
                        {
                            std::ostringstream qs;
                            bool first = true;
                            for (const auto &p : req.params)
                            {
                                if (!first) qs << '&';
                                first = false;
                                qs << p.first << '=' << p.second;
                            }
                            xreq.parseQuery(qs.str());
                        }

                        // Body
                        xreq.body = req.body;

                        // Params extraction
                        xreq.params = extract_params(route.path, req.path);

                        // Cookies
                        auto ck = req.headers.find("Cookie");
                        if (ck != req.headers.end())
                            xreq.cookies = parse_cookies(ck->second);

                        // Headers
                        for (auto &h : req.headers)
                            xreq.headers[h.first] = h.second;

                        // IP
                        xreq.ip = req.remote_addr;

                        // Parse body (json/urlencoded)
                        xreq.parseBody();

                        Response xres;

                        // Run handler
                        route.handler(xreq, xres);

                        // Return response
                        res.status = xres.getStatus();

                        for (auto &h : xres.getHeaders())
                            res.set_header(h.first.c_str(), h.second.c_str());

                        res.set_content(xres.getBody(), xres.getContentType().c_str());
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "[Server Error] " << e.what() << std::endl;
                        res.status = 500;
                        res.set_content("Internal Server Error", "text/plain");
                    }
                };

                // ------------------------
                // 🔥 Attach route methods
                // ------------------------
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

                // Express-style "ALL"
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

            std::cout << "🚀 Xpress++ listening on http://"
                      << hostIp_ << ":" << port_ << std::endl;

            svr.listen(hostIp_.c_str(), port_);
        }

    private:
        App &app_;
        std::string hostIp_;
        int port_;
    };
}
