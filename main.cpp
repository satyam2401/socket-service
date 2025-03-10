#include <iostream>
#include "redisHandler/RedisConsumer.h"
#include "server/WebSocketServer.h"

using namespace std;

int main() {
//    replace it with your actual redis endpoint
    RedisConsumer::initialize("127.0.0.1:6379");

    try {
        asio::io_context ioContext;
        WebSocketServer server(ioContext, 8000);
        server.start();
        ioContext.run();
    } catch (const std::exception& e) {
        std::cerr << "Server Error: " << e.what() << std::endl;
    }

    RedisConsumer::shutdown();

    return 0;
}
