#pragma once
#include <iostream>
#include <list>
#include <vector>
#include <algorithm>
#include <stdexcept>

template <class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
public:
    class iterator;
    class const_iterator;

    explicit HashMap(Hash t_hasher = Hash()) : hasher_(t_hasher)  {
        this->filled_amount_ = 0;
        this->avail_buckets_ = 2;
//        this->hasher_ = t_hasher;
//        this->hasher_ = t_hasher;
        this->hash_table_ = std::vector<std::list<std::pair<const KeyType, ValueType>>>(2,
                                                                                        std::list<std::pair<const KeyType, ValueType>>());
    }

    template <class Iterator>
    HashMap(Iterator t_begin, Iterator t_end, Hash t_hasher = Hash()) : hasher_(t_hasher)  {
        this->filled_amount_ = 0;
        this->avail_buckets_ = 2;
//        this->hasher_ = t_hasher;
        this->hash_table_ = std::vector<std::list<std::pair<const KeyType, ValueType>>>(2,
                                                                                        std::list<std::pair<const KeyType, ValueType>>());
        while (t_begin != t_end) {
            this->insert(*t_begin);
            ++t_begin;
        }
    }

    HashMap(std::initializer_list<std::pair<KeyType, ValueType>> t_list, Hash t_hasher = Hash()) : hasher_(t_hasher) {
            this->filled_amount_ = 0;
            this->avail_buckets_ = 2;
            this->hash_table_ = std::vector<std::list<std::pair<const KeyType, ValueType>>>(2,
            std::list<std::pair<const KeyType, ValueType>>());
            for (const auto &element: t_list) {
                this->insert(element);
            }
    }
//
//    HashMap(HashMap &t_other) {
//        this->hasher_ = t_other.hasher_;
//        this->filled_amount_ = t_other.filled_amount_;
//        this->avail_buckets_ = t_other.avail_buckets_;
////        this->hash_table_.copy(t_other.hash_table_.begin(), t_other.end());
//        this->hash_table_ = t_other.hash_table_;
//    }

    HashMap& operator=(const HashMap &t_other) {
        if (this == &t_other) {
            return *this;
        }
        this->clear();
        this->filled_amount_ = 0;
        this->avail_buckets_ = 2;
        this->hasher_ = t_other.hash_function();
        this->hash_table_ = std::vector<std::list<std::pair<const KeyType, ValueType>>>(2,
                                                                                        std::list<std::pair<const KeyType, ValueType>>());
        for (const auto &element: t_other) {
            this->insert(element);
        }
        return *this;
    }

    bool empty() const {
        return !this->filled_amount_;
    }

    size_t size() const {
        return this->filled_amount_;
    }

    iterator begin() {
        typename std::vector<std::list<std::pair<
                const KeyType, ValueType>>>::iterator first_nonempty_cell = hash_table_.begin();
        if (this->empty()) {
            return this->end();
        }
        while (first_nonempty_cell->empty()) {
            ++first_nonempty_cell;
        }
        return iterator(first_nonempty_cell->begin(), first_nonempty_cell, &hash_table_);
    }

    iterator end() {
        typename std::vector<std::list<std::pair<
                const KeyType, ValueType>>>::iterator last_cell = hash_table_.end();
        typename std::list<std::pair<const KeyType, ValueType>>::iterator last_elem = last_cell->end();
        return iterator(last_elem, last_cell, &hash_table_);
    }
//
//    size_t hash_function(KeyType t_key) const {
//        return this->hasher_(t_key);
//    }

    Hash hash_function() const {
        return this->hasher_;
    }

    iterator find(KeyType t_key) {
        const size_t bucket = this->apply_hash(t_key, this->avail_buckets_);
        typename std::list<std::pair<const KeyType, ValueType>>::iterator elem_iter = std::find_if(this->hash_table_[bucket].begin(), this->hash_table_[bucket].end(),
                     [t_key](const std::pair<const KeyType, ValueType>& element){ return element.first == t_key; });
        if (elem_iter == this->hash_table_[bucket].end()) {
            return this->end();
        }
        return iterator(elem_iter, this->hash_table_.begin() + bucket, &this->hash_table_);
    }

    const_iterator find(KeyType t_key) const {
        const size_t bucket = this->apply_hash(t_key, this->avail_buckets_);
        typename std::list<std::pair<const KeyType, ValueType>>::const_iterator elem_iter = std::find_if(this->hash_table_[bucket].begin(), this->hash_table_[bucket].end(),
                                                                                                   [t_key](const std::pair<const KeyType, ValueType>& element){ return element.first == t_key; });
        if (elem_iter == this->hash_table_[bucket].end()) {
            return this->end();
        }
        return const_iterator(elem_iter, this->hash_table_.begin() + bucket, &this->hash_table_);
    }

    iterator insert(std::pair<const KeyType, ValueType> t_element) {
        if (this->find(t_element.first) == this->end()) {
            const size_t bucket = this->apply_hash(t_element.first, this->avail_buckets_);
            this->hash_table_[bucket].push_front(t_element);
            ++(this->filled_amount_);
            if (1.0f * this->filled_amount_ / this->avail_buckets_ > this->upscale_load_factor_) {
                this->upscale();
            }
            return this->find(t_element.first);
        } else {
            return this->end();
        }
    }

    void erase(KeyType t_key) {
        if (this->find(t_key) != this->end()) {
            size_t bucket = this->apply_hash(t_key, this->avail_buckets_);
            typename std::list<std::pair<const KeyType, ValueType>>::const_iterator elem_iter = std::find_if(this->hash_table_[bucket].begin(), this->hash_table_[bucket].end(),
                                                                                                             [t_key](const std::pair<const KeyType, ValueType>& element){ return element.first == t_key; });
            this->hash_table_[bucket].erase(elem_iter);
            --(this->filled_amount_);
            if ((this->filled_amount_ != 0) && (1.0f * this->filled_amount_ / this->avail_buckets_ < this->downscale_load_factor_)) {
                this->downscale();
            }
        }
    }

    void clear() {
        std::vector<std::list<std::pair<const KeyType, ValueType>>> new_table(this->avail_buckets_ * 2, std::list<std::pair<const KeyType, ValueType>>());
        for (auto &bucket: this->hash_table_) {
            bucket.clear();
        }
        this->hash_table_.clear();
        this->hash_table_.swap(new_table);
        this->filled_amount_ = 0;
        this->avail_buckets_ = 1;
    }

    ValueType& operator[](KeyType t_key) {
        iterator found_iterator = this->find(t_key);
        if (found_iterator == this->end()) {
            iterator placed_iterator = this->insert({t_key, {}});
            return placed_iterator->second;
        } else {
            return found_iterator->second;
        }
    }

    const ValueType& at(KeyType t_key) const {
        const_iterator found_iterator = this->find(t_key);
        if (found_iterator == this->end()) {
            throw std::out_of_range("HashMap<...>.at() : index out of range");
        }
        else {
            return found_iterator->second;
        }
    }

    const_iterator begin() const {
        typename std::vector<std::list<std::pair<
                const KeyType, ValueType>>>::const_iterator first_nonempty_cell = this->hash_table_.begin();
        if (this->empty()) {
            return this->end();
        }
        while (first_nonempty_cell->empty()) {
            ++first_nonempty_cell;
        }
        return const_iterator(first_nonempty_cell->begin(), first_nonempty_cell, &this->hash_table_);
    }

    const_iterator end() const {
        typename std::vector<std::list<std::pair<
                const KeyType, ValueType>>>::const_iterator last_cell = this->hash_table_.end();
        typename std::list<std::pair<const KeyType, ValueType>>::const_iterator last_elem = last_cell->end();
        return const_iterator(last_elem, last_cell, &this->hash_table_);
    }

    class iterator {
    public:
        iterator(typename std::list<std::pair<const KeyType, ValueType>>::iterator t_element,
                 typename std::vector<std::list<std::pair<
                         const KeyType, ValueType>>>::iterator t_cell,
                 std::vector<std::list<std::pair<const KeyType, ValueType>>>* t_table) {
            this->current_element = t_element;
            this->current_cell = t_cell;
            this->parent_table = t_table;
        }

        iterator() = default;

        explicit iterator(iterator* t_iter) {
            this->current_element = t_iter->current_element;
            this->current_cell = t_iter->current_cell;
            this->parent_table = t_iter->parent_table;
        }

        iterator& operator=(iterator* t_iterator) {
            this->current_element = t_iterator->current_element;
            this->current_cell = t_iterator->current_cell;
            this->parent_table = t_iterator->parent_table;
            return *this;
        }

        iterator& operator++() {
            ++this->current_element;
            if (this->current_element == this->current_cell->end()) {
                ++this->current_cell;
                while ((this->current_cell != this->parent_table->end()) && (this->current_cell->empty())) {
//                    std::cout << std::endl << "New cell" << std::endl;
                    this->current_cell++;
                }
                if (this->current_cell == this->parent_table->end()) {
                    this->current_element = this->current_cell->end();
                } else {
                    this->current_element = this->current_cell->begin();
                }
            }
            return *this;
        }

        iterator operator++(int) {
            iterator result(*this);
            ++(*this);
            return result;
        }

        bool operator==(const iterator& other) const {
            return (this->current_element == other.current_element);
        }

        bool operator!=(const iterator& other) const {
            return !(this->current_element == other.current_element);
        }

        std::pair<const KeyType, ValueType>& operator*() {
            return *(this->current_element);
        }

        std::pair<const KeyType, ValueType>* operator->() {
            return &(*this->current_element);
        }

    private:
        const std::vector<std::list<std::pair<const KeyType, ValueType>>>* parent_table;
        typename std::list<std::pair<const KeyType, ValueType>>::iterator current_element;
        typename std::vector<std::list<std::pair<
                const KeyType, ValueType>>>::iterator current_cell;
    };

    class const_iterator {
    public:
        const_iterator(typename std::list<std::pair<const KeyType, ValueType>>::const_iterator t_element,
                 typename std::vector<std::list<std::pair<
                         const KeyType, ValueType>>>::const_iterator t_cell,
                 const std::vector<std::list<std::pair<const KeyType, ValueType>>>* t_table) {
            this->current_element = t_element;
            this->current_cell = t_cell;
            this->parent_table = t_table;
//            std::cout<< "created iterator" << std::endl;
        }

        const_iterator() = default;

        explicit const_iterator(const_iterator* t_iter) {
            this->current_element = t_iter->current_element;
            this->current_cell = t_iter->current_cell;
            this->parent_table = t_iter->parent_table;
        }

        const_iterator& operator=(const_iterator* t_iterator) {
            this->current_element = t_iterator->current_element;
            this->current_cell = t_iterator->current_cell;
            this->parent_table = t_iterator->parent_table;
            return *this;
        }

        const_iterator& operator++() {
            ++this->current_element;
            if (this->current_element == this->current_cell->end()) {
                ++this->current_cell;
                while ((this->current_cell != this->parent_table->end()) && (this->current_cell->empty())) {
//                    std::cout << std::endl << "New cell" << std::endl;
                    this->current_cell++;
                }
                if (this->current_cell == this->parent_table->end()) {
                    this->current_element = this->current_cell->end();
                } else {
                    this->current_element = this->current_cell->begin();
                }
            }
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator result(*this);
            ++(*this);
            return result;
        }

        bool operator==(const const_iterator& other) const {
            return (this->current_element == other.current_element);
        }

        bool operator!=(const const_iterator& other) const {
            return !(this->current_element == other.current_element);
        }

        const std::pair<const KeyType, ValueType>& operator*() {
            return *(this->current_element);
        }

        const std::pair<const KeyType, ValueType>* operator->() {
            return &(*this->current_element);
        }

    private:
        const std::vector<std::list<std::pair<const KeyType, ValueType>>>* parent_table;
        typename std::list<std::pair<const KeyType, ValueType>>::const_iterator current_element;
        typename std::vector<std::list<std::pair<
                const KeyType, ValueType>>>::const_iterator current_cell;
    };

protected:
//    size_t posmod(size_t t_hash, size_t t_mod) const {
//        t_hash %= t_mod;
//        if (t_hash < 0) {
//            return t_hash + t_mod;
//        } else {
//            return t_hash;
//        }
//    }

    size_t apply_hash(KeyType t_key, size_t t_bucket_range) const {
        return this->hasher_(t_key) % t_bucket_range;
    }

    void upscale() {
        std::vector<std::list<std::pair<const KeyType, ValueType>>> new_table(this->avail_buckets_ * 2, std::list<std::pair<const KeyType, ValueType>>());
        for (auto &bucket: this->hash_table_) {
            for (auto const &element: bucket) {
                size_t target_bucket = this->apply_hash(element.first, this->avail_buckets_ * 2);
                new_table[target_bucket].push_front({element.first, element.second});
            }
            bucket.clear();
        }
        this->hash_table_.clear();
//        this->hash_table_ = new_table;
        this->hash_table_.swap(new_table);
        this->avail_buckets_ *= 2;
    }

    void downscale() {
        std::vector<std::list<std::pair<const KeyType, ValueType>>> new_table(this->avail_buckets_ / 2, std::list<std::pair<const KeyType, ValueType>>());
        for (auto &bucket: this->hash_table_) {
            for (auto const &element: bucket) {
                size_t target_bucket = this->apply_hash(element.first, this->avail_buckets_ / 2);
                new_table[target_bucket].push_front({element.first, element.second});
            }
            bucket.clear();
        }
        this->hash_table_.clear();
//        this->hash_table_ = new_table;
        this->hash_table_.swap(new_table);
        this->avail_buckets_ /= 2;
    }

    const double upscale_load_factor_ = 0.75;
    const double downscale_load_factor_ = 0.25;
    Hash hasher_;
    int32_t avail_buckets_ = 2;
    size_t filled_amount_ = 0;
    std::vector<std::list<std::pair<const KeyType, ValueType>>> hash_table_;

};
