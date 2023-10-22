#pragma once

#include "sw_fwd.h"  // Forward declaration
#include "shared.h"

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T>
class WeakPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    WeakPtr() : base_block_(nullptr), observed_ptr_(nullptr){};

    WeakPtr(const WeakPtr& other) {
        base_block_ = other.base_block_;
        observed_ptr_ = other.observed_ptr_;
        if (base_block_) {
            base_block_->IncreaseWeakCounter();
        }
    }

    WeakPtr(WeakPtr&& other) {
        base_block_ = other.base_block_;
        observed_ptr_ = other.observed_ptr_;
        other.base_block_ = nullptr;
        other.observed_ptr_ = nullptr;
    }

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    WeakPtr(const SharedPtr<T>& other) {
        base_block_ = other.base_block_;
        observed_ptr_ = other.observed_ptr_;
        if (base_block_) {
            base_block_->IncreaseWeakCounter();
        }
    }

    template <typename Y>
    WeakPtr& operator=(const SharedPtr<Y>& other) {
        if (base_block_) {
            base_block_->DecreaseWeakCounter();
        }
        base_block_ = other.base_block_;
        observed_ptr_ = other.observed_ptr_;
        if (base_block_) {
            base_block_->IncreaseWeakCounter();
        }
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) {
        if (this != &other) {
            if (base_block_ != nullptr) {
                base_block_->DecreaseWeakCounter();
            }
            base_block_ = other.base_block_;
            if (base_block_) {
                base_block_->IncreaseWeakCounter();
            }
            observed_ptr_ = other.observed_ptr_;
        }
        return *this;
    }

    WeakPtr& operator=(WeakPtr&& other) {
        if (this != &other) {
            if (base_block_ != nullptr) {
                base_block_->DecreaseWeakCounter();
            }
            base_block_ = other.base_block_;
            observed_ptr_ = other.observed_ptr_;
            other.base_block_ = nullptr;
            other.observed_ptr_ = nullptr;
        }
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~WeakPtr() {
        if (base_block_) {
            base_block_->DecreaseWeakCounter();
        }
        base_block_ = nullptr;
        observed_ptr_ = nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (base_block_) {
            base_block_->DecreaseWeakCounter();
        }
        base_block_ = nullptr;
        observed_ptr_ = nullptr;
    }

    void Swap(WeakPtr& other) {
        std::swap(base_block_, other.base_block_);
        std::swap(observed_ptr_, other.observed_ptr_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        if (base_block_) {
            return base_block_->GetStrongCounter();
        }
        return 0;
    }

    bool Expired() const {
        return UseCount() == 0;
    }

    SharedPtr<T> Lock() const {
        if (Expired()) {
            return SharedPtr<T>();
        } else {
            SharedPtr<T> return_ptr = SharedPtr<T>();
            base_block_->IncreaseStrongCounter();
            return_ptr.SetBlockPtr(base_block_);
            return_ptr.SetObservedPtr(observed_ptr_);
            return return_ptr;
        }
    }

private:
    template <typename Y>
    friend class WeakPtr;
    template <typename Y>
    friend class SharedPtr;
    BaseControlBlock* base_block_;
    T* observed_ptr_;
};
