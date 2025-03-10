//
// Created by Satyam Saurabh on 21/02/25.
//

#ifndef SOCKETSERVICE_CONCURRENTHASHMAP_H
#define SOCKETSERVICE_CONCURRENTHASHMAP_H

#include "list"
#include "shared_mutex"

template<typename K, typename V>
class ConcurrentHashMap {
public:
    explicit ConcurrentHashMap(size_t size = 16);
    ~ConcurrentHashMap();

    void insert(const K &key, const V &value);
    bool remove(const K &key);
    std::optional<V> find(const K &key) const;

private:
    std::vector<std::list<std::pair<K, V>>> buckets;
    mutable std::vector<std::shared_mutex> locks;
    mutable std::shared_mutex global_mutex;
    size_t hash(const K& key) const;
    std::atomic<size_t>  size = 0;
    const double load_factor = 0.75;

    void rehash();
};

template<typename K, typename V>
ConcurrentHashMap<K, V>::ConcurrentHashMap(size_t size) : buckets(size), locks(size) {}

template<typename K, typename V>
ConcurrentHashMap<K, V>::~ConcurrentHashMap() = default;

template<typename K, typename V>
size_t ConcurrentHashMap<K, V>::hash(const K &key) const {
    size_t bucket_count = buckets.size();
    return bucket_count > 0 ? std::hash<K>{}(key) % bucket_count : 0;
}

template<typename K, typename V>
void ConcurrentHashMap<K, V>::insert(const K &key, const V &value) {
    size_t index = hash(key);
    std::unique_lock<std::shared_mutex> lock(locks[index]);
    auto &bucket = buckets[index];
    for(auto &pair: bucket){
        if(pair.first == key){
            pair.second = value;
            return;
        }
    }
    bucket.emplace_back(key, value);
    size.fetch_add(1, std::memory_order_relaxed);

    if(size.load(std::memory_order_relaxed) >= load_factor * buckets.size()){
        lock.unlock();
        std::unique_lock<std::shared_mutex> global_lock(global_mutex);
        rehash();
    }
}


template<typename K, typename V>
std::optional<V> ConcurrentHashMap<K, V>::find(const K &key) const {
    // to prevent access during resizing
    std::shared_lock<std::shared_mutex> global_lock(global_mutex);

    size_t index = hash(key);
    std::shared_lock<std::shared_mutex> lock(locks[index]);
    for(const auto& pair: buckets[index]){
        if(pair.first == key){
            return pair.second;
        }
    }
    return std::nullopt;
}

template<typename K, typename V>
bool ConcurrentHashMap<K, V>::remove(const K &key) {
    // to prevent access during resizing
    std::shared_lock<std::shared_mutex> global_lock(global_mutex);

    size_t index = hash(key);
    std::unique_lock<std::shared_mutex> lock(locks[index]);
    auto& bucket = buckets[index];
    for(auto it = bucket.begin(); it != bucket.end(); ++it){
        if(it->first == key){
            bucket.erase(it);
            size.fetch_sub(1, std::memory_order_relaxed);
            return true;
        }
    }
    return false;
}

template<typename K, typename V>
void ConcurrentHashMap<K, V>::rehash() {
    std::unique_lock<std::shared_mutex> global_lock(global_mutex);

    size_t new_bucket_count = buckets.size() * 2;
    std::vector<std::list<std::pair<K, V>>> new_buckets(new_bucket_count);
    std::vector<std::shared_mutex> new_locks(new_bucket_count);

    for (const auto& bucket : buckets) {
        for (const auto& pair : bucket) {
            size_t new_index = std::hash<K>{}(pair.first) % new_bucket_count;
            new_buckets[new_index].emplace_back(pair);
        }
    }

    buckets = std::move(new_buckets);
    locks = std::move(new_locks);
}


#endif //SOCKETSERVICE_CONCURRENTHASHMAP_H
