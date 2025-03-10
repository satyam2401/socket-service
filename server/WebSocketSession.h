//
// Created by Satyam Saurabh on 24/02/25.
//

#ifndef WEBSOCKETSESSION_H
#define WEBSOCKETSESSION_H

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <unordered_set>
#include <string>

#include "../model/SocketConnection.h"

namespace websocket = boost::beast::websocket;
using tcp = boost::asio::ip::tcp;

class WebSocketSession : public std::enable_shared_from_this<WebSocketSession> {
public:
    explicit WebSocketSession(tcp::socket socket);
    void start();

    static void handleDisconnection(std::shared_ptr<SocketConnection> conn);
private:
    websocket::stream<tcp::socket> ws_;
    std::unordered_set<std::string> subscribedSymbols_;
    std::shared_ptr<SocketConnection> connection_;

    std::atomic<std::chrono::steady_clock::time_point> lastHeartbeatReceived;
    std::condition_variable heartbeatCV;

    void readMessage();
    void handleMessage(const std::string& message);
    void manageHeartbeat();
    void subscribe(const std::vector<std::string>& symbols);
    void unsubscribe(const std::vector<std::string>& symbols);
};

#endif // WEBSOCKETSESSION_H
