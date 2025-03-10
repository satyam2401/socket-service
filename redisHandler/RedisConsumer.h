//
// Created by Satyam Saurabh on 02/03/25.
//

#ifndef SOCKETSERVICE_REDISCONSUMER_H
#define SOCKETSERVICE_REDISCONSUMER_H

#include <string>
#include <memory>
#include <thread>
#include <hiredis/hiredis.h>

class RedisConsumer {
public:
    static void initialize(const std::string& redisAddr);
    static void consumeStream(const std::string& symbol);
    static void shutdown();

private:
    static redisContext* redisCtx;
    static std::thread ioThread;
};

#endif //SOCKETSERVICE_REDISCONSUMER_H