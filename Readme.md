# Socket Service (C++)

## Overview
This socket service attempts to build a high-performance, concurrent WebSocket-based service for real-time market data streaming. It efficiently manages client connections, handles subscriptions to market symbols, and processes incoming data from Redis streams.

The primary goal of this service is to:
- Develop a scalable and concurrent WebSocket server for real-time financial data streaming.
- Efficiently manage multiple client connections and their subscription preferences.
- Integrate Redis Streams for real-time market data ingestion.
- Implement an optimized connection management system using a custom concurrent hashmap.

To handle high-frequency WebSocket connections efficiently, this project implements a custom concurrent hashmap. This hashmap is used in the global maps for connection management, ensuring:
- Thread-safe operations for storing and retrieving WebSocket connections.
- Efficient subscription tracking per symbol.

## Modules

### 1. Server Module
The `server` module manages the WebSocket connections, client interactions, and messaging.

#### 1.1 WebSocket Server
- Listens for incoming WebSocket connections.
- Creates a `WebSocketSession` for each new client.
- Manages the lifecycle of client connections.

#### 1.2 WebSocket Session
Each client connection is handled by a `WebSocketSession`, which provides the following functionalities:

##### Subscribe
- Clients can subscribe to one or multiple market symbols.
- The session stores subscribed symbols in global connection maps.

##### Unsubscribe
- Clients can unsubscribe from specific symbols.
- The service removes their session from the global maps.

##### Handle Disconnection
- If a client disconnects (intentionally or due to a network failure), the session:
    - Removes the client from active subscriptions.
    - Cleans up the global connection map.

### 2. Redis Handler Module
The `redisHandler` module is responsible for fetching real-time market data from Redis Streams.

#### 2.1 Redis Setup
- Uses hiredis to connect to a Redis instance.
- Fetches data from Redis Streams to distribute real-time updates.

#### 2.2 Consuming Stream
- The `consumeStream()` method listens for market data updates in Redis.
- Each new tick is:
    - Mapped to subscribed WebSocket clients.
    - Broadcast to all relevant connections.

## Next Steps
1. Implement a lock-free concurrent hashmap to further reduce contention and improve scalability under high loads.
2. Introduce message compression techniques, such as gzip or LZ4, to reduce bandwidth usage and improve transmission efficiency
3. Add client-side code to populate Redis with dummy tick data for end-to-end flow.
4. Optimize data broadcasting to further reduce latency and improve scalability.
5. Explore alternative serialization methods for lower payload sizes.
6. Improve logging and monitoring to track performance metrics in real-time.

