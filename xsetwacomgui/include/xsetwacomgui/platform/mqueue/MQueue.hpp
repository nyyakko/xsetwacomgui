#pragma once

#include "MQueueDescriptor.hpp"

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <liberror/Result.hpp>

#include <mqueue.h>

class MQueue
{
public:
    MQueue()
        : name_{}
        , descriptor_{}
    {}

    MQueue(MQueue const&) = delete;
    MQueue& operator=(MQueue const&) = delete;

    MQueue(MQueue&& that)
        : name_{std::move(that.name_)}
        , descriptor_{std::move(that.descriptor_)}
    {}

    MQueue& operator=(MQueue&& that)
    {
        this->name_ = std::move(that.name_);
        this->descriptor_ = std::move(that.descriptor_);
        return *this;
    }

    ~MQueue()
    {
        if (descriptor_.value() != -1)
        {
            mq_unlink(name_.c_str());
        }
    }

public:
    static liberror::Result<MQueue> create(std::string_view name, asio::io_context* context, int flag = O_RDONLY | O_CREAT | O_EXCL);
    static liberror::Result<MQueue> create(std::string_view name, int flag = O_RDONLY | O_CREAT | O_EXCL);

    // cppcheck-suppress [functionStatic, constParameterReference]
    auto& descriptor(this auto& self) { return self.descriptor_; }

    liberror::Result<std::vector<char>> receive() const { return descriptor_.receive(); }
    asio::awaitable<std::vector<char>> receive_async() { return descriptor_.receive_async(); }

    liberror::Result<void> send(std::ranges::random_access_range auto data) const { return descriptor_.send(data); }

private:
    std::string name_;
    MQueueDescriptor descriptor_;
};
