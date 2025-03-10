//
// Created by Satyam Saurabh on 06/03/25.
//

#ifndef SOCKETSERVICE_CLIENTREQUEST_H
#define SOCKETSERVICE_CLIENTREQUEST_H

#include <vector>
#include <string>
#include <boost/json.hpp>

class ClientRequest {
public:
    std::string action;
    std::vector<std::string> value;
    std::string userId;

    explicit ClientRequest(const boost::json::value& json) {
        if (json.is_object()) {
            const auto& obj = json.as_object();
            if (obj.contains("action") && obj.at("action").is_string()) {
                action = obj.at("action").as_string().c_str();
            }
            if (obj.contains("value") && obj.at("value").is_array()) {
                for (const auto& val : obj.at("value").as_array()) {
                    if (val.is_string()) {
                        value.push_back(val.as_string().c_str());
                    }
                }
            }
            if (obj.contains("userId") && obj.at("userId").is_string()) {
                userId = obj.at("userId").as_string().c_str();
            }
        }
    }
};


#endif //SOCKETSERVICE_CLIENTREQUEST_H
