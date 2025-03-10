cmake_minimum_required(VERSION 3.29)
project(SocketService)

set(CMAKE_CXX_STANDARD 20)

# Set Boost paths (ensure you have Boost installed via Homebrew)
set(BOOST_ROOT "/opt/homebrew/opt/boost")
set(Boost_INCLUDE_DIR "/opt/homebrew/include")
set(Boost_LIBRARY_DIR "/opt/homebrew/lib")

# Find required Boost components
find_package(Boost REQUIRED COMPONENTS system thread json)

# Ensure OpenSSL is found
find_package(OpenSSL REQUIRED)

# Find and link Hiredis
find_package(PkgConfig REQUIRED)
pkg_check_modules(HIREDIS REQUIRED hiredis)

include_directories(${HIREDIS_INCLUDE_DIRS})
link_directories(${HIREDIS_LIBRARY_DIRS})
add_definitions(${HIREDIS_CFLAGS_OTHER})

# Include directories
include_directories(${Boost_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/model)
include_directories(${CMAKE_SOURCE_DIR}/server)
include_directories(${CMAKE_SOURCE_DIR}/redisHandler)

# Add executable
add_executable(SocketService
        main.cpp
        server/WebSocketServer.cpp
        server/WebSocketSession.cpp
        utils/GlobalMaps.cpp
        redisHandler/RedisConsumer.cpp
)

# Link necessary libraries
target_link_libraries(SocketService
        Boost::system
        Boost::thread
        Boost::json
        OpenSSL::SSL
        OpenSSL::Crypto
        ${HIREDIS_LIBRARIES}  # Link Hiredis
        pthread
)
