//
// Created by Satyam Saurabh on 03/03/25.
//

#ifndef SOCKETSERVICE_GLOBALMAPS_H
#define SOCKETSERVICE_GLOBALMAPS_H

#include <unordered_set>

#include "ConcurrentHashMap.h"
#include "../model/SocketConnection.h"

/*
 * To do an efficient connection management, these maps are created:
 * 1. symbolConnectionMap: key -> symbol | value -> list of connection object
 *    This map is used while broadcasting a symbol tick to all connections subscribed to it.
 * 2. connectionSymbolMap: key -> connection object | value -> list of symbol
 *    This map is used for storing list of symbols for a connection.
 */

extern ConcurrentHashMap<std::string, std::vector<std::shared_ptr<SocketConnection>>> symbolConnectionMap;

extern ConcurrentHashMap<std::shared_ptr<SocketConnection>, std::unordered_set<std::string>> connectionSymbolMap;

extern ConcurrentHashMap<std::string, bool> streamStatusMap;

#endif //SOCKETSERVICE_GLOBALMAPS_H
