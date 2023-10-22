#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t
#include <type_traits>

// https://en.cppreference.com/w/cpp/memory/shared_ptr

class EnableSharedFromThisBase {};

template <typename T>
class EnableSharedFromThis;

class BaseControlBlock {
public:
    virtual ~BaseControlBlock(){};
    virtual void IncreaseStrongCounter() = 0;
    virtual void DecreaseStrongCounter() = 0;
    virtual void IncreaseWeakCounter() = 0;
    virtual void DecreaseWeakCounter() = 0;
    virtual void BruteDecreaseWeakCounter() = 0;
    virtual size_t GetStrongCounter() = 0;
};

template <typename T>
class PtrControlBlock : BaseControlBlock {
public:
    PtrControlBlock(T* ptr) {
        strong_counter_ = 1;
        weak_counter_ = 0;
        control_ptr_ = ptr;
    };
    ~PtrControlBlock() override {
        strong_counter_ = 0;
        weak_counter_ = 0;
        control_ptr_ = nullptr;
    };
    virtual void IncreaseStrongCounter() override {
        ++strong_counter_;
    };
    virtual void DecreaseStrongCounter() override {
        --strong_counter_;
        if (strong_counter_ == 0) {
            delete control_ptr_;
            control_ptr_ = nullptr;
            if (weak_counter_ == 0) {
                delete this;
            }
        }
    };
    virtual void IncreaseWeakCounter() override {
        ++weak_counter_;
    };
    virtual void DecreaseWeakCounter() override {
        --weak_counter_;
        if (weak_counter_ == 0 && strong_counter_ == 0) {
            delete this;
        }
    };
    virtual void BruteDecreaseWeakCounter() override {
        --weak_counter_;
    }
    size_t GetStrongCounter() override {
        return strong_counter_;
    }

    size_t strong_counter_;
    size_t weak_counter_;
    T* control_ptr_;
};

template <typename T, typename... Args>
class ObjectControlBlock : BaseControlBlock {
public:
    ObjectControlBlock(Args&&... args) {
        strong_counter_ = 1;
        weak_counter_ = 0;
        buffer_ptr_ = reinterpret_cast<T*>(new (&buffer_) T(std::forward<Args>(args)...));
    }
    ~ObjectControlBlock() override {
        strong_counter_ = 0;
        weak_counter_ = 0;
        buffer_ptr_ = nullptr;
    };
    virtual void IncreaseStrongCounter() override {
        ++strong_counter_;
    };
    virtual void DecreaseStrongCounter() override {
        --strong_counter_;
        if (strong_counter_ == 0) {
            buffer_ptr_->~T();
            buffer_ptr_ = nullptr;
            if (weak_counter_ == 0) {
                delete this;
            }
        }
    };
    virtual void IncreaseWeakCounter() override {
        ++weak_counter_;
    };
    virtual void DecreaseWeakCounter() override {
        --weak_counter_;
        if (weak_counter_ == 0 && strong_counter_ == 0) {
            delete this;
        }
    };
    virtual void BruteDecreaseWeakCounter() override {
        --weak_counter_;
    }
    size_t GetStrongCounter() override {
        return strong_counter_;
    }

    size_t strong_counter_;
    size_t weak_counter_;
    std::aligned_storage_t<sizeof(T), alignof(T)> buffer_;
    T* buffer_ptr_;
};

template <typename T>
class SharedPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr() : base_block_(nullptr), observed_ptr_(nullptr){};

    SharedPtr(std::nullptr_t) : base_block_(nullptr), observed_ptr_(nullptr){};

    template <typename Y>
    explicit SharedPtr(Y* ptr) {
        base_block_ = reinterpret_cast<BaseControlBlock*>(new PtrControlBlock<Y>(ptr));
        observed_ptr_ = ptr;
        if constexpr (std::is_convertible_v<Y*, EnableSharedFromThisBase*>) {
            InitWeakThis(observed_ptr_);
        }
    }

    SharedPtr(const SharedPtr& other) {
        base_block_ = other.base_block_;
        observed_ptr_ = other.observed_ptr_;
        if (base_block_) {
            base_block_->IncreaseStrongCounter();
        }
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            InitWeakThis(observed_ptr_);
        }
    }

    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other) {
        base_block_ = other.base_block_;
        observed_ptr_ = other.observed_ptr_;
        if (base_block_) {
            base_block_->IncreaseStrongCounter();
        }
        if constexpr (std::is_convertible_v<Y*, EnableSharedFromThisBase*>) {
            InitWeakThis(observed_ptr_);
        }
    }

    SharedPtr(SharedPtr&& other) {
        base_block_ = other.base_block_;
        observed_ptr_ = other.observed_ptr_;
        other.base_block_ = nullptr;
        other.observed_ptr_ = nullptr;
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            InitWeakThis(observed_ptr_);
        }
    }

    template <typename Y>
    SharedPtr(SharedPtr<Y>&& other) {
        base_block_ = other.base_block_;
        observed_ptr_ = other.observed_ptr_;
        other.base_block_ = nullptr;
        other.observed_ptr_ = nullptr;
        if constexpr (std::is_convertible_v<Y*, EnableSharedFromThisBase*>) {
            InitWeakThis(observed_ptr_);
        }
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) {
        base_block_ = other.base_block_;
        if (base_block_) {
            base_block_->IncreaseStrongCounter();
        }
        observed_ptr_ = ptr;
        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            InitWeakThis(observed_ptr_);
        }
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other) {
        if (other.Expired()) {
            throw BadWeakPtr();
        }
        base_block_ = other.base_block_;
        observed_ptr_ = other.observed_ptr_;
        if (base_block_) {
            base_block_->IncreaseStrongCounter();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (this != &other) {
            if (base_block_ != nullptr) {
                base_block_->DecreaseStrongCounter();
            }
            base_block_ = other.base_block_;
            if (base_block_) {
                base_block_->IncreaseStrongCounter();
            }
            observed_ptr_ = other.observed_ptr_;
        }
        return *this;
    }

    template <typename Y>
    SharedPtr& operator=(const SharedPtr<Y>& other) {
        if (base_block_ != nullptr) {
            base_block_->DecreaseStrongCounter();
        }
        base_block_ = other.base_block_;
        if (base_block_) {
            base_block_->IncreaseStrongCounter();
        }
        observed_ptr_ = other.observed_ptr_;
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) {
        if (this != &other) {
            if (base_block_ != nullptr) {
                base_block_->DecreaseStrongCounter();
            }
            base_block_ = other.base_block_;
            observed_ptr_ = other.observed_ptr_;
            other.base_block_ = nullptr;
            other.observed_ptr_ = nullptr;
        }
        return *this;
    }

    template <typename Y>
    SharedPtr& operator=(SharedPtr<Y>&& other) {
        if (base_block_ != nullptr) {
            base_block_->DecreaseStrongCounter();
        }
        base_block_ = other.base_block_;
        observed_ptr_ = other.observed_ptr_;
        other.base_block_ = nullptr;
        other.observed_ptr_ = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        if (base_block_) {
            base_block_->DecreaseStrongCounter();
        }
        base_block_ = nullptr;
        observed_ptr_ = nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void SetBlockPtr(BaseControlBlock* ptr) {
        base_block_ = ptr;
    }
    void SetObservedPtr(T* ptr) {
        observed_ptr_ = ptr;
    }
    void Reset() {
        if (base_block_) {
            base_block_->DecreaseStrongCounter();
        }
        base_block_ = nullptr;
        observed_ptr_ = nullptr;
    }
    template <typename Y>
    void Reset(Y* ptr) {
        if (base_block_) {
            base_block_->DecreaseStrongCounter();
        }
        base_block_ = reinterpret_cast<BaseControlBlock*>(new PtrControlBlock<Y>(ptr));
        observed_ptr_ = ptr;
    }
    void Swap(SharedPtr& other) {
        std::swap(base_block_, other.base_block_);
        std::swap(observed_ptr_, other.observed_ptr_);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return observed_ptr_;
    }
    T& operator*() const {
        return *observed_ptr_;
    }
    T* operator->() const {
        return observed_ptr_;
    }
    size_t UseCount() const {
        if (base_block_) {
            return base_block_->GetStrongCounter();
        }
        return 0;
    }
    explicit operator bool() const {
        return observed_ptr_ != nullptr;
    }

    template <typename Y>
    void InitWeakThis(EnableSharedFromThis<Y>* e) {
        e->weak_this_ = WeakPtr(*this);
    }

private:
    template <typename Y>
    friend class SharedPtr;
    template <typename Y>
    friend class WeakPtr;
    BaseControlBlock* base_block_;
    T* observed_ptr_;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return left.Get() == right.Get();
}

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    SharedPtr<T> return_ptr;
    ObjectControlBlock<T, Args...>* object_block_ptr =
        new ObjectControlBlock<T, Args...>(std::forward<Args>(args)...);
    return_ptr.SetObservedPtr(object_block_ptr->buffer_ptr_);
    return_ptr.SetBlockPtr(reinterpret_cast<BaseControlBlock*>(object_block_ptr));
    if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
        return_ptr.InitWeakThis(object_block_ptr->buffer_ptr_);
    }
    return return_ptr;
}

// Look for usage examples in tests and seminar
template <typename T>
class EnableSharedFromThis : public EnableSharedFromThisBase {
public:
    SharedPtr<T> SharedFromThis() {
        return weak_this_.Lock();
    };
    SharedPtr<const T> SharedFromThis() const {
        return weak_this_.Lock();
    }
    WeakPtr<T> WeakFromThis() noexcept {
        return weak_this_;
    }
    WeakPtr<const T> WeakFromThis() const noexcept {
        return WeakPtr<const T>(weak_this_);
    }
    ~EnableSharedFromThis() {
        weak_this_.base_block_->BruteDecreaseWeakCounter();
        weak_this_.base_block_ = nullptr;
        weak_this_.observed_ptr_ = nullptr;
    }

private:
    template <typename Y>
    friend class SharedPtr;
    WeakPtr<T> weak_this_;
};
