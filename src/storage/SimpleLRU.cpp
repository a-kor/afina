// -*- compile-command: "cd ../.. && make runStorageTests && ./test/storage/runStorageTests" -*-
#include "SimpleLRU.h"
#include <memory>

namespace Afina {
namespace Backend {

// Makes *lru_index_it the most recently used.
void SimpleLRU::make_mru(const lru_index_t::iterator it) {
    _lru_list.push_back(std::move(*it->second));
    _lru_list.erase(it->second);
    it->second = --_lru_list.end();
}

// Deletes lru entry unless it's *updated_it.
void SimpleLRU::delete_front(const lru_list_t::iterator updated_it) {
    auto del_it = _lru_list.begin();
    if (del_it == _lru_list.end() || del_it == updated_it)
        throw std::runtime_error("not enough space to store a single entry");
    _cur_size -= (*del_it)->key.size() + (*del_it)->value.size();
    _lru_index.erase((*del_it)->key);
    _lru_list.erase(del_it);
}

// Deletes first elements until enough size available, *updated_it could not be deleted.
void SimpleLRU::delete_maybe(const std::size_t requested_size, const lru_list_t::iterator updated_it) {
    while (_max_size < _cur_size + requested_size) {
        delete_front(updated_it);
    }
}

// Sets value by iterator.
void SimpleLRU::set(const lru_index_t::iterator it, const std::string &value) {
    std::size_t requested_size = value.size() - (*it->second)->value.size();
    delete_maybe(requested_size, it->second);
    (*it->second)->value = value;
    make_mru(it);
    _cur_size += requested_size;
}

// Puts without existance check.
void SimpleLRU::put(const std::string &key, const std::string &value) {
    delete_maybe(key.size() + value.size());
    _lru_list.push_back(std::unique_ptr<lru_node>(new lru_node{key, value}));
    auto it = std::prev(_lru_list.end());
    _lru_index.insert(std::make_pair(std::ref((*it)->key), it));
    _cur_size += (key.size() + value.size());
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    auto it = _lru_index.find(key);
    if (it == _lru_index.end()) {
        put(key, value);
    } else {
        set(it, value);
    }
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    auto it = _lru_index.find(key);
    if (it == _lru_index.end()) {
        put(key, value);
        return true;
    } else {
        return false;
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    auto it = _lru_index.find(key);
    if (it == _lru_index.end()) {
        return false;
    } else {
        set(it, value);
        return true;
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    auto it = _lru_index.find(key);
    if (it == _lru_index.end()) {
        return false;
    } else {
        auto lru_list_it = it->second;
        _lru_index.erase(it);
        _lru_list.erase(lru_list_it);
        return true;
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) {
    auto it = _lru_index.find(key);
    if (it == _lru_index.end()) {
        return false;
    } else {
        auto lru_list_iterator = it->second;
        value = (*lru_list_iterator)->value;
        make_mru(it);
        return true;
    }
}
} // namespace Backend
} // namespace Afina
