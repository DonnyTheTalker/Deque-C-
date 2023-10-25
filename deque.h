#pragma once

#include <initializer_list>
#include <algorithm>
#include <deque>

template <typename T>
class Block {
public:
    Block() {
        content_ = new T[max_size_];

        start_ = content_;
        end_ = (content_ + max_size_ - 1);

        first_elem_ = start_;
        last_elem_ = start_;
    }

    Block& operator=(Block&& other) {
        if (this != &other) {
            delete[] content_;

            content_ = other.content_;
            size_ = other.size_;

            start_ = other.start_;
            end_ = other.end_;

            first_elem_ = other.first_elem_;
            last_elem_ = other.last_elem_;

            other.content_ = nullptr;
        }

        return *this;
    }

    ~Block() {
        delete[] content_;
    }

    void Clear() {
        size_ = 0;
        first_elem_ = start_;
        last_elem_ = start_;
    }

    template <typename V>
    requires std::is_convertible<V, T>
    void PushBack(V&& value) {
        if (size_ == 0) {
            *start_ = std::forward(value);
            first_elem_ = start_;
            last_elem_ = start_;
        } else {
            ++last_elem_;
            *last_elem_ = std::forward(value);
        }
        ++size_;
    }

    void PopBack() {
        --last_elem_;
        --size_;
    }

    template <typename V>
    requires std::is_convertible<V, T>
    void PushFront(V&& value) {
        if (size_ == 0) {
            *end_ = std::forward(value);
            first_elem_ = end_;
            last_elem_ = end_;
        } else {
            --first_elem_;
            *first_elem_ = std::forward(value);
        }
        ++size_;
    }

    void PopFront() {
        ++first_elem_;
        --size_;
    }

    size_t Size() const {
        return size_;
    }

    T& Get(size_t ind) {
        return *(first_elem_ + ind);
    }

    const T& Get(size_t ind) const {
        return *(first_elem_ + ind);
    }

    bool IsEmpty() {
        return size_ == 0;
    }

private:
    bool CanPushBack() {
        return size_ == 0 || last_elem_ != end_;
    }

    bool CanPushFront() {
        return size_ == 0 || first_elem_ != start_;
    }

    void Copy(const Block& rhs) {
        if (this == &rhs) {
            return;
        }

        delete[] content_;

        size_ = rhs.size_;
        content_ = new T[max_size_];

        for (size_t i = 0; i < max_size_; i++) {
            content_[i] = rhs.content_[i];
        }

        start_ = content_;
        end_ = content_ + max_size_ - 1;

        first_elem_ = start_ + (rhs.first_elem_ - rhs.start_);
        last_elem_ = start_ + (rhs.last_elem_ - rhs.start_);
    }

private:
    friend class Deque;

private:
    size_t size_ = 0;
    const size_t max_size_ = 128;

    T* start_;
    T* end_;

    T* first_elem_;
    T* last_elem_;

    T* content_;
};

template <typename T>
class Deque {
public:
    Deque() {
        content_ = new Block<T>[1];
        block_cnt_ = 1;

        first_elem_ = content_;
        last_elem_ = content_;
    }

    Deque(const Deque& rhs) {
        Copy(rhs);
    }

    Deque(Deque&& rhs) {
        content_ = rhs.content_;
        size_ = rhs.size_;
        block_cnt_ = rhs.block_cnt_;
        first_elem_ = rhs.first_elem_;
        last_elem_ = rhs.last_elem_;

        rhs.content_ = nullptr;
        rhs.size_ = 0;
    }

    explicit Deque(size_t size) {
        block_cnt_ = std::max(static_cast<size_t>(1), size / block_size_);
        content_ = new Block<T>[block_cnt_];

        first_elem_ = content_;
        last_elem_ = content_;
    }

    Deque(std::initializer_list<int> list) {
        block_cnt_ = std::max(static_cast<size_t>(1), list.size() / block_size_);
        content_ = new Block<T>[block_cnt_];

        first_elem_ = content_;
        last_elem_ = content_;

        for (const auto val : list) {
            PushBack(val);
        }
    }

    Deque& operator=(const Deque& rhs) {
        if (&rhs == this) {
            return *this;
        }

        delete[] content_;
        Copy(rhs);
        return *this;
    }

    Deque& operator=(Deque&& rhs) {
        if (this == &rhs) {
            return *this;
        }

        delete[] content_;

        content_ = rhs.content_;
        size_ = rhs.size_;
        block_cnt_ = rhs.block_cnt_;
        first_elem_ = rhs.first_elem_;
        last_elem_ = rhs.last_elem_;

        rhs.content_ = nullptr;
        rhs.size_ = 0;

        return *this;
    }

    ~Deque() {
        delete[] content_;
    }

    void Swap(Deque& rhs) {
        std::swap(size_, rhs.size_);
        std::swap(block_cnt_, rhs.block_cnt_);
        std::swap(first_elem_, rhs.first_elem_);
        std::swap(last_elem_, rhs.last_elem_);
        std::swap(content_, rhs.content_);
    }

    template <typename V>
    requires std::is_convertible<V, T>
    void PushBack(V&& value) {
        if (!last_elem_->CanPushBack()) {
            auto next = (last_elem_ - content_ + 1) % block_cnt_ + content_;
            if (next == first_elem_) {
                Extend();
                PushBack(std::forward(value));
                return;
            }
            last_elem_ = next;
        }
        last_elem_->PushBack(std::forward(value));
        ++size_;
    }

    void PopBack() {
        last_elem_->PopBack();
        --size_;

        if (last_elem_->IsEmpty()) {
            if (size_ > 0) {
                last_elem_ = (last_elem_ - content_ + block_cnt_ - 1) % block_cnt_ + content_;
            }
        }
    }

    template <typename V>
    requires std::is_convertible<V, T>
    void PushFront(V&& value) {
        if (!first_elem_->CanPushFront()) {
            auto prev = (first_elem_ - content_ + block_cnt_ - 1) % block_cnt_ + content_;
            if (prev == last_elem_) {
                Extend();
                PushFront(std::forward(value));
                return;
            }
            first_elem_ = prev;
        }
        first_elem_->PushFront(std::forward(value));
        ++size_;
    }

    void PopFront() {
        first_elem_->PopFront();
        --size_;

        if (first_elem_->IsEmpty()) {
            if (size_ > 0) {
                first_elem_ = (first_elem_ - content_ + 1) % block_cnt_ + content_;
            }
        }
    }

    int& operator[](size_t ind) {
        if (first_elem_->size_ >= ind + 1) {
            return first_elem_->Get(ind);
        } else if (size_ - last_elem_->size_ < ind + 1) {
            return last_elem_->Get(ind - (size_ - last_elem_->size_));
        } else {
            ind -= first_elem_->size_;
            auto block = (first_elem_ - content_ + 1 + ind / block_size_) % block_cnt_ + content_;
            return block->Get(ind % block_size_);
        }
    }

    int operator[](size_t ind) const {
        if (first_elem_->size_ >= ind + 1) {
            return first_elem_->Get(ind);
        } else if (size_ - last_elem_->size_ < ind + 1) {
            return last_elem_->Get(ind - (size_ - last_elem_->size_));
        } else {
            ind -= first_elem_->size_;
            auto block = (first_elem_ - content_ + 1 + ind / block_size_) % block_cnt_ + content_;
            return block->Get(ind % block_size_);
        }
    }

    size_t Size() const {
        return size_;
    }

    void Clear() {
        for (size_t i = 0; i < block_cnt_; i++) {
            content_[i].Clear();
        }
        size_ = 0;
        first_elem_ = content_;
        last_elem_ = content_;
    }

private:
    void Copy(const Deque& rhs) {
        if (this == &rhs) {
            return;
        }
        
        content_ = new Block<T>[rhs.block_cnt_];
        block_cnt_ = rhs.block_cnt_;
        size_ = rhs.size_;

        first_elem_ = content_ + (rhs.first_elem_ - rhs.content_);
        last_elem_ = content_ + (rhs.last_elem_ - rhs.content_);

        for (size_t i = 0; i < block_cnt_; ++i) {
            content_[i].Copy(rhs.content_[i]);
        }
    }

    void Extend() {
        auto new_content = new Block<T>[block_cnt_ * 2];
        auto new_first_elem = new_content;
        auto new_last_elem = new_content + block_cnt_ - 1;

        for (size_t i = 0; i < block_cnt_; i++) {
            new_content[i] = std::move(content_[(first_elem_ - content_ + i) % block_cnt_]);
        }
        delete[] content_;

        content_ = new_content;
        block_cnt_ *= 2;
        first_elem_ = new_first_elem;
        last_elem_ = new_last_elem;
    }

private:
    size_t size_ = 0;
    size_t block_cnt_ = 0;
    const size_t block_size_ = 128;

    Block<T>* first_elem_;
    Block<T>* last_elem_;

    Block<T>* content_;
};
