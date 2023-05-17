#include <iostream>
#include <algorithm>
#include <cstring>
#include <type_traits>
#include <string>

template<typename T>
class Deque {

private:

    static const size_t my_chunk_size = 32;
    static const size_t minimum_size = 3;

    T** my_pointers = nullptr;
    size_t my_reserve_size = 0;

    template<typename Value>
    struct BasicIterator {

    public:

        using difference_type = std::ptrdiff_t;
        using value_type = Value;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::random_access_iterator_tag;

    private:

        T** my_deque_pointers = nullptr;
        size_t my_index = 0;

    public:

        BasicIterator() = default;

        BasicIterator(T** deque_pointers, size_t index) :
                my_deque_pointers(deque_pointers), my_index(index) {};

        BasicIterator& operator+=(int n) {
            my_index += static_cast<size_t>(n);
            return *this;
        }

        BasicIterator operator+(int n) const {
            auto copy = *this;
            return copy += n;
        }

        BasicIterator& operator-=(int n) {
            my_index -= static_cast<size_t>(n);
            return *this;
        }

        BasicIterator operator-(int n) const {
            auto copy = *this;
            return copy -= n;
        }

        BasicIterator& operator++() {
            return *this += 1;
        }

        BasicIterator operator++(int) {
            auto copy = *this;
            *this += 1;
            return copy;
        }

        BasicIterator& operator--() {
            return *this -= 1;
        }

        BasicIterator operator--(int) {
            auto copy = *this;
            *this -= 1;
            return copy;
        }

        difference_type operator-(const BasicIterator<Value>& that) const {
            return static_cast<difference_type>(my_index) - static_cast<difference_type>(that.my_index);
        }

        reference operator*() const {
            return my_deque_pointers[my_index / my_chunk_size][my_index % my_chunk_size];
        }

        pointer operator->() const {
            return &my_deque_pointers[my_index / my_chunk_size][my_index % my_chunk_size];
        }

        bool operator==(const BasicIterator<Value>& that) const = default;

        std::strong_ordering operator<=>(const BasicIterator<Value>& that) const = default;

        operator BasicIterator<const Value>() const {
            return BasicIterator<const Value>(my_deque_pointers, my_index);
        }

    };

    template<class It>
    struct BasicReverseIterator {

    public:

        using difference_type = std::ptrdiff_t;
        using value_type = typename It::value_type;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::random_access_iterator_tag;

    private:

        It my_it;

    public:

        BasicReverseIterator() = default;

        explicit BasicReverseIterator(const It& it) : my_it(it) {}

        BasicReverseIterator& operator+=(int n) {
            my_it -= n;
            return *this;
        }

        BasicReverseIterator operator+(int n) {
            auto copy = *this;
            return copy += n;
        }

        BasicReverseIterator& operator-=(int n) {
            my_it += n;
            return *this;
        }

        BasicReverseIterator operator-(int n) {
            auto copy = *this;
            return copy -= n;
        }

        BasicReverseIterator& operator++() {
            return *this += 1;
        }

        BasicReverseIterator operator++(int) {
            auto copy = *this;
            *this += 1;
            return copy;
        }

        BasicReverseIterator& operator--() {
            return *this -= 1;
        }

        BasicReverseIterator operator--(int) {
            auto copy = *this;
            *this -= 1;
            return copy;
        }

        difference_type operator-(const BasicReverseIterator<It>& that) const {
            return that.my_it - my_it;
        }

        reference operator*() const {
            return *my_it;
        }

        pointer operator->() const {
            return &(*my_it);
        }

        bool operator==(const BasicReverseIterator<It>& that) const = default;

        std::strong_ordering operator<=>(const BasicReverseIterator<It>& that) const {
            return that.my_it <=> my_it;
        }

    };

public:

    using value_type = T;
    using iterator = BasicIterator<T>;
    using const_iterator = BasicIterator<const T>;
    using reverse_iterator = BasicReverseIterator<iterator>;
    using const_reverse_iterator = BasicReverseIterator<const_iterator>;

private:

    iterator my_begin = iterator(my_pointers, 0);
    iterator my_end = iterator(my_pointers, 0);

    iterator reserve_begin() {
        return iterator(my_pointers, 0);
    }

    iterator reserve_end() {
        return iterator(my_pointers, my_reserve_size * my_chunk_size);
    }

    T* allocate_chunk() {
        T* ptr_chunk = nullptr;
        ptr_chunk = reinterpret_cast<T*>(new char[sizeof(T) * my_chunk_size]);
        return ptr_chunk;
    }

    void deallocate(T** pointers, size_t start, size_t finish) {
        for (size_t i = start; i != finish; ++i) {
            delete[] reinterpret_cast<char*>(pointers[i]);
        }
    }

    void destruct(iterator begin, iterator end) {
        for (iterator it = begin; it != end; ++it) {
            (*it).~T();
        }
    }

    void fill_pointers(T** new_pointers, size_t index_begin, size_t index_end, size_t dealloc_index_begin
                       , size_t dealloc_index_end) {
        for (size_t i = index_begin; i < index_end; ++i) {
            try {
                new_pointers[i] = allocate_chunk();
            } catch (...) {
                deallocate(new_pointers, index_begin, i);
                deallocate(new_pointers, dealloc_index_begin, dealloc_index_end);
                delete[] new_pointers;
                throw;
            }
        }
    }

    void reallocate(size_t new_reserve_size) {
        T** new_pointers = nullptr;
        new_pointers = new T*[new_reserve_size];

        size_t plus_begin = (new_reserve_size - my_reserve_size) / 2;

        fill_pointers(new_pointers, 0, plus_begin, 0, 0);

        fill_pointers(new_pointers, plus_begin + my_reserve_size, new_reserve_size, 0, plus_begin);

        std::copy(my_pointers, my_pointers + my_reserve_size, new_pointers + plus_begin);

        int size = static_cast<int>(my_end - my_begin);
        int shift = static_cast<int>(my_begin - reserve_begin());

        my_reserve_size = new_reserve_size;
        delete[] my_pointers;
        my_pointers = new_pointers;

        my_begin = reserve_begin() + shift + static_cast<int>(plus_begin) * static_cast<int>(my_chunk_size);
        my_end = my_begin + size;
    }

    void reallocate() {
        if (my_reserve_size) {
            reallocate(my_reserve_size * 2);
        } else {
            reallocate(minimum_size);
        }
    }

    void swap(Deque& deque) {
        std::swap(my_begin, deque.my_begin);
        std::swap(my_end, deque.my_end);
        std::swap(my_pointers, deque.my_pointers);
        std::swap(my_reserve_size, deque.my_reserve_size);
    }

public:

    Deque() = default;

    Deque(size_t size) : my_begin(my_pointers, 0) {
        int int_size = static_cast<int>(size);
        reallocate(size / my_chunk_size + 1);
        my_begin = reserve_begin() + 1; //NOLINT
        my_end = my_begin + int_size;
        for (iterator it = my_begin; it != my_end; ++it) {
            try {
                new(&(*it)) T();
            } catch(...) {
                destruct(my_begin, it);
                my_begin = my_end = reserve_begin();
                deallocate(my_pointers, 0, my_reserve_size);
                delete[] my_pointers;
                throw;
            }
        }
    }

    Deque(size_t size, const T& value) : my_begin(my_pointers, 0) {
        int int_size = static_cast<int>(size);
        reallocate(size / my_chunk_size + 1);
        my_begin = reserve_begin() + 1; //NOLINT
        my_end = my_begin + int_size;
        for (iterator it = my_begin; it != my_end; ++it) {
            try {
                new(&(*it)) T(value);
            } catch(...) {
                destruct(my_begin, it);
                my_begin = my_end = reserve_begin();
                deallocate(my_pointers, 0, my_reserve_size);
                delete[] my_pointers;
                throw;
            }
        }
    }

    Deque(const Deque& that) : my_begin(my_pointers, 0), my_end(my_begin) {
        reallocate(that.my_reserve_size);
        my_begin = reserve_begin() + 1; //NOLINT
        my_end = my_begin + static_cast<int>(that.size()); //NOLINT
        for (iterator it = my_begin, source_it = that.my_begin; source_it != that.my_end; ++it, ++source_it) {
            try {
                new(&(*it)) T(*source_it);
            } catch(...) {
                destruct(my_begin, it);
                my_begin = my_end = reserve_begin();
                deallocate(my_pointers, 0, my_reserve_size);
                delete[] my_pointers;
                throw;
            }
        }
    }

    Deque& operator=(const Deque& that) {
        Deque copy(that);
        this->swap(copy);
        return *this;
    }

    size_t size() const {
        return static_cast<size_t>(my_end - my_begin);
    }

    T& operator[](size_t index) {
        return *(my_begin + static_cast<int>(index));
    }

    const T& operator[](size_t index) const {
        return *(my_begin + static_cast<int>(index));
    }

    T& at(size_t index) {
        iterator it = my_begin + static_cast<int>(index);
        if (it >= my_end || it < my_begin) {
            throw std::out_of_range("Deque out of range");
        }
        return *it;
    }

    const T& at(size_t index) const {
        iterator it = my_begin + static_cast<int>(index);
        if (it >= my_end || it < my_begin) {
            throw std::out_of_range("Deque out of range");
        }
        return *it;
    }

    iterator push_back(const T& value) {
        if (my_end == reserve_end()) {
            reallocate();
        }
        new(&(*my_end)) T(value);
        ++my_end;
        return my_end - 1;
    }

    void pop_back() {
        --my_end;
        (*my_end).~T();
    }

    iterator push_front(const T& value) {
        if (my_begin == reserve_begin()) {
            reallocate();
        }
        new(&(*(my_begin - 1))) T(value);
        --my_begin;
        return my_begin;
    }

    void pop_front() {
        (*my_begin).~T();
        ++my_begin;
    }

    iterator begin() {
        return my_begin;
    }

    iterator end() {
        return my_end;
    }

    const_iterator cbegin() const {
        return static_cast<const_iterator>(my_begin);
    }

    const_iterator cend() const {
        return static_cast<const_iterator>(my_end);
    }

    const_iterator begin() const {
        return cbegin();
    }

    const_iterator end() const {
        return cend();
    }

    reverse_iterator rbegin() {
        return reverse_iterator(my_end - 1);
    }

    reverse_iterator rend() {
        return reverse_iterator(my_begin - 1);
    }

    const_reverse_iterator crbegin() const {
        return reverse_iterator(cend() - 1);
    }

    const_reverse_iterator crend() const {
        return reverse_iterator(cbegin() - 1);
    }

    const_reverse_iterator rbegin() const {
        return crbegin();
    }

    const_reverse_iterator rend() const {
        return crend();
    }

    iterator insert(iterator it_value, const T& value) {
        if (it_value < my_begin || it_value > my_end) {
            return my_end;
        }
        auto shift = static_cast<int>(it_value - my_begin);
        if (my_end == reserve_end()) {
            reallocate();
        }
        it_value = my_begin + shift;
        new(&(*my_end)) T(value);
        for (iterator it = my_end - 1; it >= it_value; --it) {
            std::swap(*it, *(it + 1));
        }
        ++my_end;
        return it_value;
    }

    void erase(iterator it_value) {
        if (it_value < my_begin || it_value >= my_end) {
            return;
        }
        for (iterator it = it_value; it != my_end - 1; ++it) {
            std::swap(*it, *(it + 1));
        }
        (*my_end).~T();
        --my_end;
    }

    ~Deque() {
        destruct(my_begin, my_end);
        deallocate(my_pointers, 0, my_reserve_size);
        delete[] my_pointers;
    }

};

