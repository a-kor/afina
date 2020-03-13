#ifndef AFINA_STORAGE_SIMPLE_LRU_H
#define AFINA_STORAGE_SIMPLE_LRU_H

#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <afina/Storage.h>

namespace Afina {
namespace Backend {

/**
 * # Map based implementation
 * That is NOT thread safe implementaiton!!
 */
class SimpleLRU : public Afina::Storage {
public:
    SimpleLRU(size_t max_size = 1024) : _max_size(max_size), _cur_size(0) {}

    ~SimpleLRU() {
        _lru_index.clear();
        _lru_list.clear();
    }

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value) override;

private:
    // LRU cache node
    using lru_node = struct lru_node {
        std::string key;
        std::string value;
    };

    // Maximum number of bytes could be stored in this cache.
    // i.e all (keys+values) must be less the _max_size
    std::size_t _max_size;

    // Current number of bytes stored in this cache.
    std::size_t _cur_size;

    // Main storage of lru_nodes, elements in this list ordered descending by "freshness": in the head
    // element that wasn't used for longest time.
    using lru_list_t = std::list<std::unique_ptr<lru_node>>;
    lru_list_t _lru_list;

    // Index of nodes from list above, allows fast random access to elements by lru_node#key
    // std::map<std::reference_wrapper<std::string>, std::reference_wrapper<lru_node>, std::less<std::string>>
    // _lru_index;
    using lru_index_t = std::map<std::reference_wrapper<std::string>, lru_list_t::iterator, std::less<std::string>>;
    lru_index_t _lru_index;

    // Makes *lru_index_it the most recently used.
    void raise(const lru_index_t::iterator lru_index_it);

    // Deletes lru entry unless it's *updated_it.
    void delete_front(const lru_list_t::iterator updated_it = lru_list_t::iterator());

    // Deletes first elements until enough size available, *updated_it could not be deleted.
    void delete_maybe(const std::size_t requested_size, const lru_list_t::iterator updated_it = lru_list_t::iterator());

    // Sets value by iterator.
    void set(const lru_index_t::iterator it, const std::string &value);

    // Puts without existance check.
    void put(const std::string &key, const std::string &value);
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_SIMPLE_LRU_H
