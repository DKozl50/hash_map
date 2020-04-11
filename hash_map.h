#pragma once
#include <list>
#include <vector>
#include <algorithm>
#include <stdexcept>

template <class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
public:
    class iterator;
    class const_iterator;
    using element_type = std::pair<const KeyType, ValueType>;
    using bucket_type = std::list<element_type>;
    using table_type = std::vector<bucket_type>;

    explicit HashMap(Hash t_hasher = Hash()) : hasher_(t_hasher) {}

    template <class ForwardIterator>
    HashMap(ForwardIterator t_begin, ForwardIterator t_end, Hash t_hasher = Hash()) : hasher_(t_hasher)  {
        std::for_each(t_begin, t_end, [this](const element_type& elem) { insert(elem); });
    }

    HashMap(std::initializer_list<std::pair<KeyType, ValueType>> t_list, Hash t_hasher = Hash()) :
        HashMap(t_list.begin(), t_list.end(), t_hasher) {}

    HashMap& operator=(const HashMap &t_other) {
        if (this == &t_other) {
            return *this;
        }
        HashMap<KeyType, ValueType, Hash> tmp(t_other.begin(), t_other.end(), t_other.hash_function());
        swap(tmp);
        return *this;
    }

    bool empty() const {
        return filled_amount_ == 0;
    }

    size_t size() const {
        return filled_amount_;
    }

    iterator begin() {
        return iterator(cbegin());
    }

    iterator end() {
        return iterator(cend());
    }

    const_iterator begin() const {
        return cbegin();
    }

    const_iterator end() const {
        return cend();
    }

    const_iterator cbegin() const {
        auto first_nonempty_cell = hash_table_.begin();
        if (empty()) {
            return end();
        }
        while (first_nonempty_cell->empty()) {
            ++first_nonempty_cell;
        }
        return const_iterator(first_nonempty_cell->begin(), first_nonempty_cell, &hash_table_);
    }

    const_iterator cend() const {
        auto last_cell = hash_table_.end();
        auto last_elem = last_cell->end();
        return const_iterator(last_elem, last_cell, &hash_table_);
    }


    Hash hash_function() const {
        return hasher_;
    }

    iterator find(KeyType t_key) {
        return iterator(cfind(t_key));
    }

    const_iterator find(KeyType t_key) const {
        return cfind(t_key);
    }

    const_iterator cfind(KeyType t_key) const {
        const size_t bucket_index = apply_hash(t_key, avail_buckets_);
        auto elem_iter = std::find_if(hash_table_[bucket_index].begin(), hash_table_[bucket_index].end(),
              [t_key](const element_type& element){ return element.first == t_key; });
        if (elem_iter == hash_table_[bucket_index].end()) {
            return end();
        }
        return const_iterator(elem_iter, std::next(hash_table_.begin(), bucket_index), &hash_table_);
    }


    iterator insert(element_type t_element) {
        if (find(t_element.first) != end()) {
            return end();
        }
        ++filled_amount_;
        try_upscale();
        const size_t bucket_index = apply_hash(t_element.first, avail_buckets_);
        hash_table_[bucket_index].push_front(t_element);
        return iterator(hash_table_[bucket_index].begin(), std::next(hash_table_.begin(), bucket_index), &hash_table_);
    }

    ValueType& operator[](KeyType t_key) {
        iterator found_iterator = find(t_key);
        if (found_iterator == end()) {
            iterator placed_iterator = insert({t_key, {}});
            return placed_iterator->second;
        }
        return found_iterator->second;
    }

    const ValueType& at(KeyType t_key) const {
        const_iterator found_iterator = find(t_key);
        if (found_iterator == end()) {
            throw std::out_of_range("HashMap<...>.at() : index out of range");
        }
        return found_iterator->second;
    }

    void erase(KeyType t_key) {
        if (find(t_key) == end()) {
            return;
        }
        size_t bucket_index = apply_hash(t_key, avail_buckets_);
        auto elem_iter = std::find_if(hash_table_[bucket_index].begin(), hash_table_[bucket_index].end(),
                [t_key](const element_type& element){ return element.first == t_key; });
        hash_table_[bucket_index].erase(elem_iter);
        --filled_amount_;
        if (filled_amount_ != 0) {
            try_downscale();
        }
    }

    void clear() {
        table_type new_table = table_type(default_avail_buckets_, bucket_type());
        hash_table_.clear();
        hash_table_.swap(new_table);
        filled_amount_ = 0;
        avail_buckets_ = default_avail_buckets_;
    }

    template<class table_iterator, class bucket_iterator, class table_pointer>
    class iterator_base {
    public:
        iterator_base() {};

        iterator_base(bucket_iterator t_element, table_iterator t_cell, table_pointer* t_table) {
            current_element = t_element;
            current_cell = t_cell;
            parent_table = t_table;
        }

        explicit iterator_base(iterator_base* t_iter) {
            current_element = t_iter->current_element;
            current_cell = t_iter->current_cell;
            parent_table = t_iter->parent_table;
        }

        iterator_base& operator=(iterator_base* t_iterator) {
            current_element = t_iterator->current_element;
            current_cell = t_iterator->current_cell;
            parent_table = t_iterator->parent_table;
            return *this;
        }

        iterator_base& operator++() {
            ++current_element;
            if (current_element == current_cell->end()) {
                ++current_cell;
                while ((current_cell != parent_table->end()) && (current_cell->empty())) {
                    current_cell++;
                }
                if (current_cell == parent_table->end()) {
                    current_element = current_cell->end();
                } else {
                    current_element = current_cell->begin();
                }
            }
            return *this;
        }

        iterator_base operator++(int) {
            iterator_base result(*this);
            ++(*this);
            return result;
        }

        bool operator==(const iterator_base& other) const {
            return (current_element == other.current_element);
        }

        bool operator!=(const iterator_base& other) const {
            return !(current_element == other.current_element);
        }

        typename std::conditional<std::is_const<table_pointer>::value, const element_type&, element_type&>::type operator*() {
            return *(this->current_element);
        }

        typename std::conditional<std::is_const<table_pointer>::value, const element_type*, element_type*>::type operator->() {
            return &(*this->current_element);
        }

        table_pointer* parent_table;
        bucket_iterator current_element;
        table_iterator current_cell;
    };

    class iterator : public iterator_base<typename table_type::iterator, typename bucket_type::iterator, table_type> {
    public:
        using iterator_base<typename table_type::iterator, typename bucket_type::iterator, table_type>::iterator_base;

        iterator() = default;

        iterator(const_iterator t_iter) {
            this->parent_table = const_cast<table_type*>(t_iter.parent_table);
            this->current_cell = (*this->parent_table).erase(t_iter.current_cell, t_iter.current_cell);
            this->current_element = (*this->current_cell).erase(t_iter.current_element, t_iter.current_element);
            // Trick from twitter.com/_JonKalb/status/202815932089896960
        }
    };

    class const_iterator : public iterator_base<typename table_type::const_iterator, typename bucket_type::const_iterator, const table_type> {
    public:
        using iterator_base<typename table_type::const_iterator, typename bucket_type::const_iterator, const table_type>::iterator_base;
    };

private:
    size_t apply_hash(KeyType t_key, size_t t_bucket_range) const {
        return hasher_(t_key) % t_bucket_range;
    }

    void try_upscale() {
        if ((double)filled_amount_ / avail_buckets_ <= upscale_load_factor_) {
            return;
        }
        rehash(avail_buckets_ * 2);
        avail_buckets_ *= 2;
    }

    void try_downscale() {
        if ((double)filled_amount_ / avail_buckets_ >= downscale_load_factor_) {
            return;
        }
        rehash(avail_buckets_ / 2);
        avail_buckets_ /= 2;
    }

    void rehash(size_t new_size) {
        table_type new_table(new_size, bucket_type());
        for (const auto &element: *this) {
            size_t target_bucket = apply_hash(element.first, new_size);
            new_table[target_bucket].push_front({element.first, element.second});
        }
        hash_table_.swap(new_table);
    }

    void swap(HashMap<KeyType, ValueType, Hash> t_other) {
        avail_buckets_ = t_other.avail_buckets_;
        filled_amount_ = t_other.filled_amount_;
        hash_table_.swap(t_other.hash_table_);
    }

    const double upscale_load_factor_ = 0.75;
    const double downscale_load_factor_ = 0.25;
    const size_t default_avail_buckets_ = 4;
    Hash hasher_;
    size_t avail_buckets_ = default_avail_buckets_;
    size_t filled_amount_ = 0;
    table_type hash_table_ = table_type(avail_buckets_, bucket_type());
};

