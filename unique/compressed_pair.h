#pragma once

#include <type_traits>
#include <utility>

// Me think, why waste time write lot code, when few code do trick.
template <typename Type1, typename Type2, bool = std::is_empty_v<Type1> && !std::is_final_v<Type1>,
          bool = std::is_empty_v<Type2> && !std::is_final_v<Type2>>
class CompressedPair;

template <typename F, typename S>
class CompressedPair<F, S, false, false> {
public:
    CompressedPair() {
        first_ = F();
        second_ = S();
    };

    CompressedPair(const F& first, const S& second) : first_(first), second_(second){};
    CompressedPair(F&& first, const S& second) : first_(std::move(first)), second_(second){};
    CompressedPair(const F& first, S&& second) : first_(first), second_(std::move(second)){};
    CompressedPair(F&& first, S&& second) : first_(std::move(first)), second_(std::move(second)){};

    F& GetFirst() {
        return first_;
    }

    const F& GetFirst() const {
        return first_;
    }

    S& GetSecond() {
        return second_;
    };

    const S& GetSecond() const {
        return second_;
    };

private:
    F first_;
    S second_;
};

template <typename F, typename S>
class CompressedPair<F, S, false, true> : private S {
public:
    CompressedPair() : S(), first_(F()){};
    CompressedPair(const F& first, const S& second) : S(second), first_(first){};
    CompressedPair(F&& first, const S& second) : S(second), first_(std::move(first)){};
    CompressedPair(const F& first, S&& second) : S(std::move(second)), first_(first){};
    CompressedPair(F&& first, S&& second) : S(std::move(second)), first_(std::move(first)){};

    F& GetFirst() {
        return first_;
    }

    const F& GetFirst() const {
        return first_;
    }

    S& GetSecond() {
        return reinterpret_cast<S&>(*this);
    };

    const S& GetSecond() const {
        return reinterpret_cast<const S&>(*this);
    };

private:
    F first_;
};

template <typename F, typename S>
class CompressedPair<F, S, true, false> : private F {
public:
    CompressedPair() : F(), second_(S()){};
    CompressedPair(const F& first, const S& second) : F(first), second_(second){};
    CompressedPair(F&& first, const S& second) : F(std::move(first)), second_(second){};
    CompressedPair(const F& first, S&& second) : F(first), second_(std::move(second)){};
    CompressedPair(F&& first, S&& second) : F(std::move(first)), second_(std::move(second)){};

    F& GetFirst() {
        return reinterpret_cast<F&>(*this);
    }

    const F& GetFirst() const {
        return reinterpret_cast<const F&>(*this);
    }

    S& GetSecond() {
        return second_;
    };

    const S& GetSecond() const {
        return second_;
    };

private:
    S second_;
};

template <typename F, typename S>
class CompressedPair<F, S, true, true> : private S, private F {
public:
    CompressedPair() = default;
    CompressedPair(const F& first, const S& second) : S(second), F(first){};
    CompressedPair(F&& first, const S& second) : S(second), F(std::move(first)){};
    CompressedPair(const F& first, S&& second) : S(std::move(second)), F(first){};
    CompressedPair(F&& first, S&& second) : S(std::move(second)), F(std::move(first)){};

    F& GetFirst() {
        return reinterpret_cast<F&>(*this);
    };

    const F& GetFirst() const {
        return reinterpret_cast<const F&>(*this);
    };

    S& GetSecond() {
        return reinterpret_cast<S&>(*this);
    };

    const S& GetSecond() const {
        return reinterpret_cast<const S&>(*this);
    };

private:
};

template <typename K>
class CompressedPair<K, K, true, true> {
public:
    CompressedPair() : K(){};
    K& GetFirst() {
        return first_;
    };

    const K& GetFirst() const {
        return first_;
    };

    K& GetSecond() {
        return second_;
    };

    const K& GetSecond() const {
        return second_;
    };

private:
    K first_;
    K second_;
};