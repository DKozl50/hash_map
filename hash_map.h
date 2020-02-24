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
    using bucket_type = std::list<std::pair<const KeyType, ValueType>>; // = std::list<element_type>;
    using table_type = std::vector<std::list<std::pair<const KeyType, ValueType>>>; // = std::vector<bucket_type>;

    explicit HashMap(Hash t_hasher = Hash()) : hasher_(t_hasher) {}

    template <class ForwardIterator>
    HashMap(ForwardIterator t_begin, ForwardIterator t_end, Hash t_hasher = Hash()) : hasher_(t_hasher)  {
        while (t_begin != t_end) {
            this->insert(*t_begin);
            ++t_begin;
        }
    }

    HashMap(std::initializer_list<std::pair<KeyType, ValueType>> t_list, Hash t_hasher = Hash()) : hasher_(t_hasher) {
        for (const auto &element: t_list) {
            this->insert(element);
        }
    }

    HashMap& operator=(const HashMap &t_other) {
        if (this == &t_other) {
            return *this;
        }
        this->clear();
        this->hasher_ = t_other.hash_function();
        for (const auto &element: t_other) {
            this->insert(element);
        }
        return *this;
    }

    bool empty() const {
        return this->filled_amount_ == 0;
    }

    size_t size() const {
        return this->filled_amount_;
    }

    iterator begin() {
        auto first_nonempty_cell = hash_table_.begin();
        if (this->empty()) {
            return this->end();
        }
        while (first_nonempty_cell->empty()) {
            ++first_nonempty_cell;
        }
        return iterator(first_nonempty_cell->begin(), first_nonempty_cell, &hash_table_);
    }

    iterator end() {
        auto last_cell = hash_table_.end();
        auto last_elem = last_cell->end();
        return iterator(last_elem, last_cell, &hash_table_);
    }

    Hash hash_function() const {
        return this->hasher_;
    }

    iterator find(KeyType t_key) {
        const size_t bucket_index = this->apply_hash(t_key, this->avail_buckets_);
        auto elem_iter = std::find_if(this->hash_table_[bucket_index].begin(), this->hash_table_[bucket_index].end(),
              [t_key](const element_type& element){ return element.first == t_key; });
        if (elem_iter == this->hash_table_[bucket_index].end()) {
            return this->end();
        }
        return iterator(elem_iter, this->hash_table_.begin() + bucket_index, &this->hash_table_);
    }

    const_iterator find(KeyType t_key) const {
        const size_t bucket_index = this->apply_hash(t_key, this->avail_buckets_);
        auto elem_iter = std::find_if(this->hash_table_[bucket_index].begin(), this->hash_table_[bucket_index].end(),
              [t_key](const element_type& element){ return element.first == t_key; });
        if (elem_iter == this->hash_table_[bucket_index].end()) {
            return this->end();
        }
        return const_iterator(elem_iter, this->hash_table_.begin() + bucket_index, &this->hash_table_);
        /* return this->finder(t_key); */
    }

    iterator insert(element_type t_element) {
        if (this->find(t_element.first) == this->end()) {
            ++(this->filled_amount_);
            this->upscale();
            const size_t bucket_index = this->apply_hash(t_element.first, this->avail_buckets_);
            this->hash_table_[bucket_index].push_front(t_element);
            return iterator(hash_table_[bucket_index].begin(), hash_table_.begin() + bucket_index, &this->hash_table_);
            /* return this->find(t_element.first); */
        } else {
            return this->end();
        }
    }

    void erase(KeyType t_key) {
        if (this->find(t_key) != this->end()) {
            size_t bucket_index = this->apply_hash(t_key, this->avail_buckets_);
            auto elem_iter = std::find_if(this->hash_table_[bucket_index].begin(), this->hash_table_[bucket_index].end(),
                  [t_key](const element_type& element){ return element.first == t_key; });
            this->hash_table_[bucket_index].erase(elem_iter);
            --(this->filled_amount_);
            if (this->filled_amount_ != 0) {
                    this->downscale();
            }
        }
    }

    void clear() {
        table_type new_table(this->avail_buckets_ * 4, bucket_type());
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
        }
        return found_iterator->second;
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
        auto first_nonempty_cell = this->hash_table_.begin();
        if (this->empty()) {
            return this->end();
        }
        while (first_nonempty_cell->empty()) {
            ++first_nonempty_cell;
        }
        return const_iterator(first_nonempty_cell->begin(), first_nonempty_cell, &this->hash_table_);
    }

    const_iterator end() const {
        auto last_cell = this->hash_table_.end();
        auto last_elem = last_cell->end();
        return const_iterator(last_elem, last_cell, &this->hash_table_);
    }

    template<class table_iterator, class bucket_iterator>
    class iterator_template {
    public:
        iterator_template(bucket_iterator t_element,
                 table_iterator t_cell,
                 const table_type* t_table) {
            this->current_element = t_element;
            this->current_cell = t_cell;
            this->parent_table = t_table;
        }

        iterator_template() = default;

        explicit iterator_template(iterator_template* t_iter) {
            this->current_element = t_iter->current_element;
            this->current_cell = t_iter->current_cell;
            this->parent_table = t_iter->parent_table;
        }

        iterator_template& operator=(iterator_template* t_iterator) {
            this->current_element = t_iterator->current_element;
            this->current_cell = t_iterator->current_cell;
            this->parent_table = t_iterator->parent_table;
            return *this;
        }

        iterator_template& operator++() {
            ++this->current_element;
            if (this->current_element == this->current_cell->end()) {
                ++this->current_cell;
                while ((this->current_cell != this->parent_table->end()) && (this->current_cell->empty())) {
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

        iterator_template operator++(int) {
            iterator_template result(*this);
            ++(*this);
            return result;
        }

        bool operator==(const iterator_template& other) const {
            return (this->current_element == other.current_element);
        }

        bool operator!=(const iterator_template& other) const {
            return !(this->current_element == other.current_element);
        }

    /* protected: */
        const table_type* parent_table;
        bucket_iterator current_element;
        table_iterator current_cell;
    };

    class iterator : public iterator_template<typename table_type::iterator, typename bucket_type::iterator> {
    public:
        using iterator_template<typename table_type::iterator, typename bucket_type::iterator>::iterator_template;

        /* iterator(const_iterator t_iter) { */
        /*     this->current_element = (*t_iter.current_cell).erase(t_iter.current_element, t_iter.current_element); */
        /*     this->current_element = t_iter.current_element; */
        /*     this->current_cell = t_iter.current_cell; */
        /*     this->parent_table = t_iter.parent_table; */
        /* } */

        element_type& operator*() {
            return *(this->current_element);
        }

        element_type* operator->() {
            return &(*this->current_element);
        }
    };

    class const_iterator : public iterator_template<typename table_type::const_iterator, typename bucket_type::const_iterator> {
    public:
        using iterator_template<typename table_type::const_iterator, typename bucket_type::const_iterator>::iterator_template;

        const element_type& operator*() {
            return *(this->current_element);
        }

        const element_type* operator->() {
            return &(*this->current_element);
        }
    };

private:
    size_t apply_hash(KeyType t_key, size_t t_bucket_range) const {
        return this->hasher_(t_key) % t_bucket_range;
    }

    const_iterator finder(KeyType t_key) const {
        const size_t bucket_index = this->apply_hash(t_key, this->avail_buckets_);
        auto elem_iter = std::find_if(this->hash_table_[bucket_index].begin(), this->hash_table_[bucket_index].end(),
              [t_key](const element_type& element){ return element.first == t_key; });
        if (elem_iter == this->hash_table_[bucket_index].end()) {
            return this->end();
        }
        return const_iterator(elem_iter, this->hash_table_.begin() + bucket_index, &this->hash_table_);
    }

    void upscale() {
        if ((double)this->filled_amount_ / this->avail_buckets_ > this->upscale_load_factor_) {
            table_type new_table(this->avail_buckets_ * 2, bucket_type());
            for (auto &bucket: this->hash_table_) {
                for (const auto &element: bucket) {
                    size_t target_bucket = this->apply_hash(element.first, this->avail_buckets_ * 2);
                    new_table[target_bucket].push_front({element.first, element.second});
                }
                bucket.clear();
            }
            this->hash_table_.clear();
            this->hash_table_.swap(new_table);
            this->avail_buckets_ *= 2;
        }
    }

    void downscale() {
        if ((double)this->filled_amount_ / this->avail_buckets_ < this->downscale_load_factor_) {
            table_type new_table(this->avail_buckets_ / 2, bucket_type());
            for (auto &bucket: this->hash_table_) {
                for (const auto &element: bucket) {
                    size_t target_bucket = this->apply_hash(element.first, this->avail_buckets_ / 2);
                    new_table[target_bucket].push_front({element.first, element.second});
                }
                bucket.clear();
            }
            this->hash_table_.clear();
            this->hash_table_.swap(new_table);
            this->avail_buckets_ /= 2;
        }
    }

    const double upscale_load_factor_ = 0.75;
    const double downscale_load_factor_ = 0.25;
    Hash hasher_;
    size_t avail_buckets_ = 4;
    size_t filled_amount_ = 0;
    table_type hash_table_ = table_type(avail_buckets_, bucket_type());
};

