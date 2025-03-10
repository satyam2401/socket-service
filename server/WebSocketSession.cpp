//
// Created by Satyam Saurabh on 24/02/25.
//

#include "WebSocketSession.h"
#include "../utils/GlobalMaps.h"
#include "../redisHandler/RedisConsumer.h"
#include "../model/ClientRequest.h"

#include <iostream>
#include <boost/json.hpp>

WebSocketSession::WebSocketSession(tcp::socket socket)
        : ws_(std::move(socket)),
          connection_(std::make_shared<SocketConnection>("default_id", std::make_shared<websocket::stream<tcp::socket>>(std::move(ws_)))) {}

void WebSocketSession::start() {
    ws_.async_accept([self = shared_from_this()](boost::system::error_code ec) {
        if (!ec) {
            std::cout << "WebSocket session started!" << std::endl;
            // Start heartbeat mechanism once when the connection is established on a separate thread
            std::thread(&WebSocketSession::manageHeartbeat, self).detach();
            self->readMessage();
        }
    });
}

void WebSocketSession::readMessage() {
    auto buffer = std::make_shared<boost::beast::flat_buffer>();
    ws_.async_read(*buffer, [self = shared_from_this(), buffer, this](boost::system::error_code ec, std::size_t) {
        if (!ec) {
            std::string msg = boost::beast::buffers_to_string(buffer->data());
            self->handleMessage(msg);

            // Update atomic heartbeat timestamp and notify waiting thread
            self->lastHeartbeatReceived.store(std::chrono::steady_clock::now(), std::memory_order_relaxed);
            self->heartbeatCV.notify_one();

            self->readMessage(); // continue reading message
        } else {
            std::cout << "WebSocket read error: " << ec.message() << "\n";
            WebSocketSession::handleDisconnection(connection_);  // Cleanup on error or disconnect
        }
    });
}

void WebSocketSession::handleMessage(const std::string& message) {
    boost::json::value parsed;
    try {
        parsed = boost::json::parse(message);
    } catch (...) {
        ws_.async_write(boost::asio::buffer("Invalid JSON format"), [](boost::system::error_code, std::size_t) {});
        return;
    }
    ClientRequest request(parsed);

    if (request.userId.empty()) {
        ws_.async_write(boost::asio::buffer("Missing userId"), [](boost::system::error_code, std::size_t) {});
        return;
    }

    if (request.action == "subscribe") {
        subscribe(request.value);
    } else if (request.action == "unsubscribe") {
        unsubscribe(request.value);
    } else {
        ws_.async_write(boost::asio::buffer("Unknown action"), [](boost::system::error_code, std::size_t) {});
    }
}

void WebSocketSession::manageHeartbeat() {
    const std::chrono::seconds heartbeatInterval(5);       // Send heartbeat every 5 sec
    const std::chrono::seconds heartbeatTimeout(20);       // Disconnect if inactive for 20 sec

    lastHeartbeatReceived = std::chrono::steady_clock::now();

    while (true) {
        {
            std::unique_lock<std::mutex> lock(connection_->mutex);

            // Wait for a new message or timeout check (whichever happens first)
            heartbeatCV.wait_for(lock, heartbeatInterval, [&] {
                return std::chrono::steady_clock::now() - lastHeartbeatReceived.load() < heartbeatTimeout;
            });
        }

        // Check if last received message is too old
        if (std::chrono::steady_clock::now() - lastHeartbeatReceived.load() >= heartbeatTimeout) {
            std::cerr << "Heartbeat timeout, closing connection.\n";
            handleDisconnection(connection_);
            return;
        }

        // Send heartbeat
        {
            std::lock_guard<std::mutex> lock(connection_->mutex);
            boost::system::error_code ec;
            ws_.write(boost::asio::buffer(R"({"type":"heartbeat"})"), ec);
            if (ec) {
                std::cerr << "Error writing heartbeat: " << ec.message() << std::endl;
                handleDisconnection(connection_);
                return;
            }
        }
    }
}


void WebSocketSession::subscribe(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        subscribedSymbols_.insert(symbol);
    }
    std::cout << "Subscribed to symbols: ";
    for (const auto& s : subscribedSymbols_) std::cout << s << " ";
    std::cout << std::endl;

    for(const auto& symbol: symbols){
        auto connectionListOpt = symbolConnectionMap.find(symbol);
        if(connectionListOpt) {
            auto& connectionList = connectionListOpt.value();
            bool connectionExists = false;
            for(const auto& conn: connectionList){
                if(conn->connId == connection_->connId){
                    connectionExists = true;
                    break;
                }
            }
            if(!connectionExists){
                connectionList.push_back(connection_);
                symbolConnectionMap.insert(symbol, connectionList);
            }
        } else {
            symbolConnectionMap.insert(symbol, std::vector<std::shared_ptr<SocketConnection>>{connection_});
        }
    }

    auto symbolListOpt= connectionSymbolMap.find(connection_);
    if(symbolListOpt){
        auto &symbolList = symbolListOpt.value();
        std::for_each(symbols.begin(), symbols.end(), [&](const std::string& symbol) {
            symbolList.insert(symbol);
        });
        connectionSymbolMap.insert(connection_, symbolList);
    } else {
        connectionSymbolMap.insert(connection_, std::unordered_set<std::string>(symbols.begin(), symbols.end()));
    }

    for(const auto& symbol: symbols){
        auto streamStatusOpt = streamStatusMap.find(symbol);
        if(streamStatusOpt){
            bool streamStatus = streamStatusOpt.value();
            if(streamStatus){
                std::cout<<"Already connected to stream for symbol - "<<symbol<<std::endl;
            } else {
                // Launch a detached thread to consume the stream
                std::thread([symbol]() {
                    RedisConsumer::consumeStream(symbol);
                }).detach();  // Runs independently in the background

                streamStatusMap.insert(symbol, true);
            }
        } else {
            // Launch a detached thread to consume the stream
            std::thread([symbol]() {
                RedisConsumer::consumeStream(symbol);
            }).detach();  // Runs independently in the background

            streamStatusMap.insert(symbol, true);
        }
    }

}


void WebSocketSession::unsubscribe(const std::vector<std::string>& symbols) {
    for (const auto& symbol : symbols) {
        subscribedSymbols_.erase(symbol);
    }
    std::cout << "Unsubscribed from symbols.\n";

    for (const auto& symbol : symbols) {
        auto connectionListOpt = symbolConnectionMap.find(symbol);
        if (connectionListOpt) {
            auto& connectionList = connectionListOpt.value();
            auto it = std::remove_if(connectionList.begin(), connectionList.end(),
                                     [&](const std::shared_ptr<SocketConnection>& conn) {
                                         return conn->connId == connection_->connId;
                                     });

            if (it != connectionList.end()) {
                connectionList.erase(it, connectionList.end());

                if (connectionList.empty()) {
                    symbolConnectionMap.remove(symbol);
                    streamStatusMap.insert(symbol, false);
                } else {
                    symbolConnectionMap.insert(symbol, connectionList);
                }
            }
        }
    }

    auto symbolListOpt = connectionSymbolMap.find(connection_);
    if (symbolListOpt) {
        auto& symbolList = symbolListOpt.value();

        std::unordered_set<std::string> updatedSymbolList;
        std::copy_if(symbolList.begin(), symbolList.end(),
                     std::inserter(updatedSymbolList, updatedSymbolList.end()),
                     [&](const std::string& symbol) {
                         return std::find(symbols.begin(), symbols.end(), symbol) == symbols.end();
                     });

        if (updatedSymbolList.empty()) {
            connectionSymbolMap.remove(connection_);
        } else {
            connectionSymbolMap.insert(connection_, updatedSymbolList);
        }
    }

}

void WebSocketSession::handleDisconnection(std::shared_ptr<SocketConnection> connection) {
    auto symbolListOpt = connectionSymbolMap.find(connection);
    if (symbolListOpt) {
        auto &symbolList = symbolListOpt.value();

        for (const auto &symbol: symbolList) {
            auto connectionListOpt = symbolConnectionMap.find(symbol);
            if (connectionListOpt) {
                auto &connectionList = connectionListOpt.value();

                auto it = std::find_if(connectionList.begin(), connectionList.end(),
                                       [&](const std::shared_ptr<SocketConnection> &conn) {
                                           return conn->connId == connection->connId;
                                       });

                if (it != connectionList.end()) {
                    connectionList.erase(it);

                    if (connectionList.empty()) {
                        symbolConnectionMap.remove(symbol);
                        streamStatusMap.insert(symbol, false);
                    } else {
                        symbolConnectionMap.insert(symbol, connectionList);
                    }
                }
            }
        }
    }
    connectionSymbolMap.remove(connection);
}

