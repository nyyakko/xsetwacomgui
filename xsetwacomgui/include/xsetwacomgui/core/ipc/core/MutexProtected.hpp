#pragma once

#include <mutex>

template <class T>
class MutexProtected
{
public:
    MutexProtected() : data_ {}, mutex_ {} {}

    MutexProtected(MutexProtected&& that)
        : data_(std::move(that.data_))
        , mutex_{}
    {}

    MutexProtected& operator=(MutexProtected&& that)
    {
        this->data_ = std::move(that.data_);
        return *this;
    }

    MutexProtected(T&& data)
        : data_(std::move(data))
        , mutex_ {}
    {}

    decltype(auto) with(auto&& functor)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return functor(data_);
    }

private:
    T data_;
    std::mutex mutex_;
};

