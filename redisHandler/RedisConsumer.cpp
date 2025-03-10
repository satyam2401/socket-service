//
// Created by Satyam Saurabh on 02/03/25.
//

#include <iostream>
#include <string>
#include <thread>
#include <future>
#include <mutex>
#include <hiredis/hiredis.h>
#include <boost/json.hpp>

#include "RedisConsumer.h"
#include "../utils/GlobalMaps.h"
#include "../server/WebSocketSession.h"

redisContext* RedisConsumer::redisCtx = nullptr;
std::thread RedisConsumer::ioThread;

void RedisConsumer::initialize(const std::string& redisAddr) {
    try {
        redisCtx = redisConnect("127.0.0.1", 6379);
        if (redisCtx == nullptr || redisCtx->err) {
            if (redisCtx) {
                std::cerr << "Redis connection error: " << redisCtx->errstr << std::endl;
                redisFree(redisCtx);
            } else {
                std::cerr << "Redis connection error: Can't allocate Redis context" << std::endl;
            }
            redisCtx = nullptr;
            return;
        }

        std::cout << "Redis initialized at " << redisAddr << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error initializing Redis: " << e.what() << std::endl;
    }
}

void RedisConsumer::consumeStream(const std::string& symbol) {
    std::cout << "Starting Redis Stream consumption for symbol: " << symbol << std::endl;
    std::string startID = "$";  // Start from the latest message

    if (!redisCtx) {
        std::cerr << "Redis client is not initialized.\n";
        return;
    }

    while (true) {
        std::cout << "startId - " << startID << std::endl;

        auto* reply = (redisReply*) redisCommand(redisCtx, "XREAD BLOCK 0 COUNT 1 STREAMS %s %s", symbol.c_str(), startID.c_str());
        if (!reply) {
            std::cerr << "Error reading from Redis stream: " << redisCtx->errstr << std::endl;
            continue;
        }

        if (reply->type == REDIS_REPLY_NIL || reply->elements == 0) {
            std::cerr << "No messages received.\n";
            freeReplyObject(reply);
            continue;
        }

        boost::json::object clientData;
        try {
            std::string messageID;
            std::string payload;

            if (reply->type == REDIS_REPLY_ARRAY && reply->elements > 0) {
                auto stream = reply->element[0];
                if (stream->type == REDIS_REPLY_ARRAY && stream->elements > 1) {
                    auto messages = stream->element[1];

                    if (messages->type == REDIS_REPLY_ARRAY && messages->elements > 0) {
                        auto message = messages->element[0];

                        if (message->type == REDIS_REPLY_ARRAY && message->elements > 1) {
                            messageID = message->element[0]->str;
                            for (size_t i = 1; i < message->elements; i += 2) {
                                std::string key = message->element[i]->str;
                                std::string value = message->element[i + 1]->str;
                                if (key == "payload") {
                                    payload = value;
                                }
                            }
                        }
                    }
                }
            }

            if (payload.empty()) {
                std::cerr << "No payload found in message.\n";
                freeReplyObject(reply);
                continue;
            }

            boost::json::value streamData = boost::json::parse(payload);
            clientData["data"] = streamData;
            clientData["type"] = "marketfeed";

            // Update startID for next iteration
            if (!messageID.empty()) {
                startID = messageID;
            }

        } catch (const std::exception& e) {
            std::cerr << "Error parsing message payload: " << e.what() << std::endl;
            freeReplyObject(reply);
            continue;
        }

        freeReplyObject(reply);

        auto connectionListOpt = symbolConnectionMap.find(symbol);
        if (connectionListOpt) {
            auto connectionList = connectionListOpt.value();
            std::vector<std::future<void>> tasks;

            /*
             * While broadcast, for each connection we are using a separate thread to send message
             * to the client.
             * This helps us in two ways:
             *   1. For faster execution of broadcasting logic
             *   2. If broadcast to one connection fails, it will not impact broadcasting to other connections.
             */
            for (const auto& conn : connectionList) {
                tasks.push_back(std::async(std::launch::async, [conn, clientData, symbol] {
                    std::lock_guard<std::mutex> lock(conn->mutex);
                    boost::system::error_code ec;
                    conn->conn->write(boost::asio::buffer(boost::json::serialize(clientData)), ec);
                    if (ec) {
                        std::cerr << "Error sending data to client " << conn->connId
                                  << " for stream " << symbol << ": " << ec.message() << std::endl;
                        WebSocketSession::handleDisconnection(conn);
                        return;
                    }
                }));
            }

            /*
             * This will allow us to send one tick to all the connections subscribed to it before
             * moving to the next tick data, keeping the frequency of the ticks the same for all clients.
             */
            for (auto& task : tasks) {
                task.get();
            }
        } else {
            std::cout << "conn list not found for symbol - " << symbol << ". Closing stream connection" << std::endl;
            streamStatusMap.insert(symbol, false);
        }
    }
}

void RedisConsumer::shutdown() {
    if (redisCtx) {
        redisFree(redisCtx);
        redisCtx = nullptr;
    }
    if (ioThread.joinable()) {
        ioThread.join();
    }
    std::cout << "Redis connection shut down." << std::endl;
}
