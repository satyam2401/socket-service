//
// Created by Satyam Saurabh on 02/03/25.
//

#ifndef SOCKETSERVICE_SOCKETCONNECTION_H
#define SOCKETSERVICE_SOCKETCONNECTION_H

#include <string>
#include <mutex>
#include <boost/beast.hpp>
#include <boost/asio.hpp>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

class SocketConnection {
public:
    std::string connId;
    std::shared_ptr<websocket::stream<tcp::socket>> conn;
    std::mutex mutex;

    SocketConnection(std::string id, std::shared_ptr<websocket::stream<tcp::socket>> ws)
        : connId(std::move(id)), conn(std::move(ws)) {}

    // deleting copy constructors to avoid accidental copying
    SocketConnection(const SocketConnection&) = delete;
    SocketConnection operator=(const SocketConnection&) = delete;
};


#endif //SOCKETSERVICE_SOCKETCONNECTION_H
