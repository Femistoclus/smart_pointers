#pragma once

#include <cstddef>  // for std::nullptr_t
#include <utility>  // for std::exchange / std::swap

class SimpleCounter {
public:
    size_t IncRef() {
        ++count_;
        return count_;
    }
    size_t DecRef() {
        --count_;
        return count_;
    }
    size_t RefCount() const {
        return count_;
    }

private:
    size_t count_ = 0;
};

struct DefaultDelete {
    template <typename T>
    static void Destroy(T* object) {
        delete object;
    }
};

template <typename Derived, typename Counter, typename Deleter>
class RefCounted {
public:
    // Increase reference counter.
    void IncRef() {
        counter_.IncRef();
    }

    // Decrease reference counter.
    // Destroy object using DefaultDeleter when the last instance dies.
    void DecRef() {
        if (counter_.RefCount() == 0) {
            Deleter::Destroy(static_cast<Derived*>(this));
        } else {
            counter_.DecRef();
            if (counter_.RefCount() == 0) {
                Deleter::Destroy(static_cast<Derived*>(this));
            }
        }
    }

    // Get current counter value (the number of strong references).
    size_t RefCount() const {
        return counter_.RefCount();
    }

private:
    Counter counter_;
};

template <typename Derived, typename D = DefaultDelete>
using SimpleRefCounted = RefCounted<Derived, SimpleCounter, D>;

template <typename T>
class IntrusivePtr {
    template <typename Y>
    friend class IntrusivePtr;

public:
    // Constructors
    IntrusivePtr() : ptr_object_(nullptr){};
    IntrusivePtr(std::nullptr_t) : ptr_object_(nullptr){};
    IntrusivePtr(T* ptr) : ptr_object_(ptr) {
        ptr_object_->IncRef();
    }

    template <typename Y>
    IntrusivePtr(const IntrusivePtr<Y>& other) {
        ptr_object_ = other.ptr_object_;
        if (ptr_object_) {
            ptr_object_->IncRef();
        }
    }

    template <typename Y>
    IntrusivePtr(IntrusivePtr<Y>&& other) {
        ptr_object_ = other.ptr_object_;
        other.ptr_object_ = nullptr;
    }

    IntrusivePtr(const IntrusivePtr& other) {
        ptr_object_ = other.ptr_object_;
        if (ptr_object_) {
            ptr_object_->IncRef();
        }
    }
    IntrusivePtr(IntrusivePtr&& other) {
        ptr_object_ = other.ptr_object_;
        other.ptr_object_ = nullptr;
    }

    // `operator=`-s
    IntrusivePtr& operator=(const IntrusivePtr& other) {
        if (*this != other) {
            if (ptr_object_) {
                ptr_object_->DecRef();
            }
            ptr_object_ = other.ptr_object_;
            if (ptr_object_) {
                ptr_object_->IncRef();
            }
        }
        return *this;
    }
    IntrusivePtr& operator=(IntrusivePtr&& other) {
        if (*this != other) {
            if (ptr_object_) {
                ptr_object_->DecRef();
            }
            ptr_object_ = other.ptr_object_;
            other.ptr_object_ = nullptr;
        }
        return *this;
    }

    // Destructor
    ~IntrusivePtr() {
        if (ptr_object_) {
            ptr_object_->DecRef();
        }
        ptr_object_ = nullptr;
    }

    // Modifiers
    void Reset() {
        if (ptr_object_) {
            ptr_object_->DecRef();
        }
        ptr_object_ = nullptr;
    }
    void Reset(T* ptr) {
        if (ptr_object_) {
            ptr_object_->DecRef();
        }
        ptr_object_ = ptr;
    }
    void Swap(IntrusivePtr& other) {
        std::swap(ptr_object_, other.ptr_object_);
    }

    // Observers
    T* Get() const {
        return ptr_object_;
    }
    T& operator*() const {
        return *ptr_object_;
    }
    T* operator->() const {
        return ptr_object_;
    }
    template <typename Y>
    bool operator==(const IntrusivePtr<Y>& other) const {
        return ptr_object_ == other.ptr_object_;
    }
    size_t UseCount() const {
        if (ptr_object_) {
            return ptr_object_->RefCount();
        }
        return 0;
    }
    explicit operator bool() const {
        return ptr_object_ != nullptr;
    }

private:
    T* ptr_object_;
};

template <typename T, typename... Args>
IntrusivePtr<T> MakeIntrusive(Args&&... args) {
    return IntrusivePtr<T>(reinterpret_cast<T*>(new T(std::forward<Args>(args)...)));
}
