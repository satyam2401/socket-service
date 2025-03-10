//
// Created by Satyam Saurabh on 03/03/25.
//

#include "GlobalMaps.h"

ConcurrentHashMap<std::string, std::vector<std::shared_ptr<SocketConnection>>> symbolConnectionMap(2000);
ConcurrentHashMap<std::shared_ptr<SocketConnection>, std::unordered_set<std::string>> connectionSymbolMap(10'000);
ConcurrentHashMap<std::string, bool> streamStatusMap(2000);