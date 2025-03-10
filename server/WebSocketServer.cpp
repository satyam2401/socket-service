//
// Created by Satyam Saurabh on 24/02/25.
//
#include <iostream>

#include "WebSocketServer.h"
#include "WebSocketSession.h"

WebSocketServer::WebSocketServer(asio::io_context& ioc, short port)
        : acceptor_(ioc, tcp::endpoint(tcp::v4(), port)) {}

void WebSocketServer::start() {
    acceptConnection();
}

void WebSocketServer::acceptConnection() {
    acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::cout << "New connection received. Checking request type...\n";
                    handleRequest(std::move(socket));
                }
                acceptConnection();  // Continue accepting new clients
            });
}

void WebSocketServer::handleRequest(tcp::socket socket) {
    auto buffer = std::make_shared<boost::beast::flat_buffer>();
    auto req = std::make_shared<http::request<http::string_body>>();

    auto socket_ptr = std::make_shared<tcp::socket>(std::move(socket));

    http::async_read(*socket_ptr, *buffer, *req, [this, socket_ptr, buffer, req](boost::system::error_code ec, std::size_t) {
        if (!ec) {
            if (req->target() == "/cpp/ws" && req->method() == http::verb::get) {
                auto session = std::make_shared<WebSocketSession>(std::move(*socket_ptr));
                session->start();
            } else {
                http::response<http::string_body> res(http::status::not_found, req->version());
                res.set(http::field::server, "Boost.Beast WebSocket Server");
                res.set(http::field::content_type, "text/plain");
                res.body() = "Not Found";
                res.prepare_payload();
                http::async_write(*socket_ptr, res, [](boost::system::error_code, std::size_t) {});
            }
        }
    });
}
