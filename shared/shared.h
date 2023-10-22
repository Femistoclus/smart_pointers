#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t

// https://en.cppreference.com/w/cpp/memory/shared_ptr

class BaseControlBlock {
public:
    virtual ~BaseControlBlock(){};
    virtual void IncreaseCounter() = 0;
    virtual void DecreaseCounter() = 0;
    virtual size_t GetCounter() = 0;
};

template <typename T>
class PtrControlBlock : BaseControlBlock {
public:
    PtrControlBlock(T* ptr) {
        control_ptr_ = ptr;
        counter_ = 1;
    };
    ~PtrControlBlock() override{};
    virtual void IncreaseCounter() override {
        ++counter_;
    };
    size_t GetCounter() override {
        return counter_;
    }
    void DecreaseCounter() override {
        --counter_;
        if (counter_ == 0) {
            delete control_ptr_;
            delete this;
        }
    }
    size_t counter_;
    T* control_ptr_;
};

template <typename T, typename... Args>
class ObjectControlBlock : BaseControlBlock {
public:
    ObjectControlBlock(Args&&... args)
        : counter_(1), control_object_(T(std::forward<Args>(args)...)){};

    ~ObjectControlBlock() override{};
    virtual void IncreaseCounter() override {
        ++counter_;
    };
    size_t GetCounter() override {
        return counter_;
    }
    void DecreaseCounter() override {
        --counter_;
        if (counter_ == 0) {
            delete this;
        }
    }
    size_t counter_;
    T control_object_;
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
    }

    SharedPtr(const SharedPtr& other) {
        base_block_ = other.base_block_;
        observed_ptr_ = other.observed_ptr_;
        if (base_block_) {
            base_block_->IncreaseCounter();
        }
    }

    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other) {
        base_block_ = other.base_block_;
        observed_ptr_ = other.observed_ptr_;
        if (base_block_) {
            base_block_->IncreaseCounter();
        }
    }

    SharedPtr(SharedPtr&& other) {
        base_block_ = other.base_block_;
        observed_ptr_ = other.observed_ptr_;
        other.base_block_ = nullptr;
        other.observed_ptr_ = nullptr;
    }

    template <typename Y>
    SharedPtr(SharedPtr<Y>&& other) {
        base_block_ = other.base_block_;
        observed_ptr_ = other.observed_ptr_;
        other.base_block_ = nullptr;
        other.observed_ptr_ = nullptr;
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) {
        base_block_ = other.base_block_;
        if (base_block_) {
            base_block_->IncreaseCounter();
        }
        observed_ptr_ = ptr;
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    //    explicit SharedPtr(const WeakPtr<T>& other);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (this != &other) {
            if (base_block_ != nullptr) {
                base_block_->DecreaseCounter();
            }
            base_block_ = other.base_block_;
            if (base_block_) {
                base_block_->IncreaseCounter();
            }
            observed_ptr_ = other.observed_ptr_;
        }
        return *this;
    }

    template <typename Y>
    SharedPtr& operator=(const SharedPtr<Y>& other) {
        if (base_block_ != nullptr) {
            base_block_->DecreaseCounter();
        }
        base_block_ = other.base_block_;
        if (base_block_) {
            base_block_->IncreaseCounter();
        }
        observed_ptr_ = other.observed_ptr_;
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) {
        if (this != &other) {
            if (base_block_ != nullptr) {
                base_block_->DecreaseCounter();
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
            base_block_->DecreaseCounter();
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
            base_block_->DecreaseCounter();
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
            base_block_->DecreaseCounter();
        }
        base_block_ = nullptr;
        observed_ptr_ = nullptr;
    }
    template <typename Y>
    void Reset(Y* ptr) {
        if (base_block_) {
            base_block_->DecreaseCounter();
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
            return base_block_->GetCounter();
        }
        return 0;
    }
    explicit operator bool() const {
        return observed_ptr_ != nullptr;
    }

private:
    template <typename Y>
    friend class SharedPtr;
    BaseControlBlock* base_block_;
    T* observed_ptr_;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right);

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    SharedPtr<T> return_ptr;
    ObjectControlBlock<T, Args...>* object_block_ptr =
        new ObjectControlBlock<T, Args...>(std::forward<Args>(args)...);
    return_ptr.SetObservedPtr(&object_block_ptr->control_object_);
    return_ptr.SetBlockPtr(reinterpret_cast<BaseControlBlock*>(object_block_ptr));
    return return_ptr;
}
// Look for usage examples in tests
// template <typename T>
// class EnableSharedFromThis {
// public:
//    SharedPtr<T> SharedFromThis();
//    SharedPtr<const T> SharedFromThis() const;
//
//    WeakPtr<T> WeakFromThis() noexcept;
//    WeakPtr<const T> WeakFromThis() const noexcept;
//};
