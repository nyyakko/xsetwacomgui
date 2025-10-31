#pragma once

#include <mutex>

template <class T>
class MutexProtected
{
public:
    explicit MutexProtected(T&& data)
        : data_(std::move(data))
        , mutex_{}
    {}

    MutexProtected()
        : data_{}
        , mutex_{}
    {}

    MutexProtected(MutexProtected const&) = delete;
    MutexProtected& operator=(MutexProtected const&) = delete;

    MutexProtected(MutexProtected&& that)
        : data_{std::move(that.data_)}
        , mutex_{}
    {}

    MutexProtected& operator=(MutexProtected&& that)
    {
        this->data_ = std::move(that.data_);
        return *this;
    }

    decltype(auto) with(auto&& functor)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return functor(data_);
    }

private:
    T data_;
    std::mutex mutex_;
};

