//
// Created by Satyam Saurabh on 24/02/25.
//

#ifndef SOCKETSERVICE_WEBSOCKETSERVER_H
#define SOCKETSERVICE_WEBSOCKETSERVER_H

#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

class WebSocketServer {
public:
    WebSocketServer(asio::io_context& ioc, short port);
    void start();

private:
    tcp::acceptor acceptor_;

    void acceptConnection();
    void handleRequest(tcp::socket socket);
};


#endif //SOCKETSERVICE_WEBSOCKETSERVER_H
