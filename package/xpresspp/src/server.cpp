#include "xpresspp/server.hpp"
#include "xpresspp/app.hpp"

#include <asio.hpp>
#include <iostream>
#include <string>

using asio::ip::tcp;

namespace xpresspp
{
    Server::Server(App &app, int port)
        : app_(app), 
          io_context_(),
          acceptor_(io_context_, tcp::endpoint(tcp::v4(), port)) {}

    void Server::run()
    {
        std::cout << "🚀 Xpress++ listening on port "
                  << acceptor_.local_endpoint().port() << std::endl;

        while (true)
        {
            tcp::socket socket(io_context_);
            acceptor_.accept(socket);

            try
            {
                // Read HTTP request
                asio::streambuf buffer;
                asio::read_until(socket, buffer, "\r\n");

                std::istream request_stream(&buffer);
                std::string method, path, version;
                request_stream >> method >> path >> version;

                Request req;
                req.method = method;
                req.url = path;
                req.path = path;

                Response res;
                app_.handleRequest(method, path, req, res);

                std::string body = res.getBody();
                int status = res.getStatus();

                std::string http_response = 
                    "HTTP/1.1 " + std::to_string(status) + " OK\r\n"
                    "Content-Type: " + res.getContentType() + "\r\n"
                    "Content-Length: " + std::to_string(body.size()) + "\r\n"
                    "Connection: close\r\n\r\n" +
                    body;

                asio::write(socket, asio::buffer(http_response));
            }
            catch (std::exception &e)
            {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }
    }
}
