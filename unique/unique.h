#pragma once

#include "compressed_pair.h"
#include "deleters.h"

#include <cstddef>  // std::nullptr_t
#include <type_traits>

template <class T>
class DefaultDeleter {
public:
    DefaultDeleter() = default;

    template <typename K>
    DefaultDeleter(const DefaultDeleter<K>&){};

    template <typename K>
    DefaultDeleter(DefaultDeleter<K>&&){};

    template <typename K>
    DefaultDeleter& operator=(const DefaultDeleter<K>&) {
        return *this;
    }

    template <typename K>
    DefaultDeleter& operator=(DefaultDeleter<K>&&) noexcept {
        return *this;
    };

    void operator()(T* p) const {
        delete p;
    }

    ~DefaultDeleter() = default;
};

template <class T>
class DefaultDeleter<T[]> {
public:
    DefaultDeleter() = default;

    template <typename K>
    DefaultDeleter(const DefaultDeleter<K>&){};

    template <typename K>
    DefaultDeleter(DefaultDeleter<K>&&){};

    template <typename K>
    DefaultDeleter& operator=(const DefaultDeleter<K>&) {
        return *this;
    }

    template <typename K>
    DefaultDeleter& operator=(DefaultDeleter<K>&&) noexcept {
        return *this;
    };

    void operator()(T* p) const {
        delete[] p;
    }

    ~DefaultDeleter() = default;
};

// Primary template
template <typename T, typename DeleterTemp = DefaultDeleter<T>>
class UniquePtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : compressed_pair_(ptr, DeleterTemp()){};

    UniquePtr(T* ptr, const DeleterTemp& deleter) : compressed_pair_(ptr, deleter){};

    UniquePtr(T* ptr, DeleterTemp&& deleter)
        : compressed_pair_(ptr, std::forward<DeleterTemp>(deleter)){};

    template <typename K, typename OtherDeleter = DeleterTemp>
    UniquePtr(UniquePtr<K, OtherDeleter>&& other) noexcept
        : compressed_pair_(other.Release(), std::forward<OtherDeleter>(other.GetDeleter())){};

    UniquePtr(UniquePtr& other) = delete;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    UniquePtr& operator=(UniquePtr& other) = delete;

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        Reset(other.Release());
        compressed_pair_.GetSecond() = std::forward<DeleterTemp>(other.GetDeleter());
        return *this;
    }

    UniquePtr& operator=(std::nullptr_t) {
        Reset();
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        Reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        T* temp = compressed_pair_.GetFirst();
        compressed_pair_.GetFirst() = nullptr;
        return temp;
    }

    void Reset(T* ptr = nullptr) {
        T* temp = compressed_pair_.GetFirst();
        compressed_pair_.GetFirst() = ptr;
        if (temp != nullptr) {
            compressed_pair_.GetSecond()(temp);
        }
    }
    void Swap(UniquePtr& other) {
        std::swap(compressed_pair_, other.compressed_pair_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return compressed_pair_.GetFirst();
    }
    DeleterTemp& GetDeleter() {
        return compressed_pair_.GetSecond();
    }
    const DeleterTemp& GetDeleter() const {
        return compressed_pair_.GetSecond();
    }
    explicit operator bool() const {
        return compressed_pair_.GetFirst() != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    std::add_lvalue_reference_t<T> operator*() {
        return *compressed_pair_.GetFirst();
    }

    std::add_lvalue_reference_t<T> operator*() const {
        return *compressed_pair_.GetFirst();
    }

    T* operator->() {
        return compressed_pair_.GetFirst();
    }

    T* operator->() const {
        return compressed_pair_.GetFirst();
    }

private:
    CompressedPair<T*, DeleterTemp> compressed_pair_;
};

// Specialization for arrays
template <typename T, typename DeleterTemp>
class UniquePtr<T[], DeleterTemp> {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : compressed_pair_(ptr, DeleterTemp()){};

    UniquePtr(T* ptr, const DeleterTemp& deleter) : compressed_pair_(ptr, deleter){};

    UniquePtr(T* ptr, DeleterTemp&& deleter)
        : compressed_pair_(ptr, std::forward<DeleterTemp>(deleter)){};

    template <typename K, typename OtherDeleter = DeleterTemp>
    UniquePtr(UniquePtr<K, OtherDeleter>&& other) noexcept
        : compressed_pair_(other.Release(), std::forward<OtherDeleter>(other.GetDeleter())){};

    UniquePtr(UniquePtr& other) = delete;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    UniquePtr& operator=(UniquePtr& other) = delete;

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        Reset(other.Release());
        compressed_pair_.GetSecond() = std::forward<DeleterTemp>(other.GetDeleter());
        return *this;
    }

    UniquePtr& operator=(std::nullptr_t) {
        Reset();
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        Reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        T* temp = compressed_pair_.GetFirst();
        compressed_pair_.GetFirst() = nullptr;
        return temp;
    }

    void Reset(T* ptr = nullptr) {
        T* temp = compressed_pair_.GetFirst();
        compressed_pair_.GetFirst() = ptr;
        if (temp != nullptr) {
            compressed_pair_.GetSecond()(temp);
        }
    }
    void Swap(UniquePtr& other) {
        std::swap(compressed_pair_, other.compressed_pair_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return compressed_pair_.GetFirst();
    }
    DeleterTemp& GetDeleter() {
        return compressed_pair_.GetSecond();
    }
    const DeleterTemp& GetDeleter() const {
        return compressed_pair_.GetSecond();
    }
    explicit operator bool() const {
        return compressed_pair_.GetFirst() != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    std::add_lvalue_reference_t<T> operator*() {
        return *compressed_pair_.GetFirst();
    }

    std::add_lvalue_reference_t<T> operator*() const {
        return *compressed_pair_.GetFirst();
    }

    std::add_lvalue_reference_t<T> operator[](size_t i) const {
        return compressed_pair_.GetFirst()[i];
    }

    T* operator->() {
        return compressed_pair_.GetFirst();
    }

    T* operator->() const {
        return compressed_pair_.GetFirst();
    }

private:
    CompressedPair<T*, DeleterTemp> compressed_pair_;
};
