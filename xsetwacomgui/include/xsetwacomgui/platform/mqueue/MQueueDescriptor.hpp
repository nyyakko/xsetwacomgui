#pragma once

#include <asio/awaitable.hpp>
#include <asio/buffer.hpp>
#include <asio/posix/stream_descriptor.hpp>
#include <asio/use_awaitable.hpp>
#include <liberror/Result.hpp>
#include <liberror/Try.hpp>

#include <mqueue.h>

#include <optional>

class MQueueDescriptor
{
public:
    MQueueDescriptor()
        : value_{-1}
        , stream_{std::nullopt}
    {}

    MQueueDescriptor(MQueueDescriptor const&) = delete;
    MQueueDescriptor& operator=(MQueueDescriptor const&) = delete;

    MQueueDescriptor(MQueueDescriptor&& that)
        : value_{std::exchange(that.value_, -1)}
        , stream_{std::move(that.stream_)}
    {}

    MQueueDescriptor& operator=(MQueueDescriptor&& that)
    {
        this->value_ = std::exchange(that.value_, -1);
        this->stream_ = std::move(that.stream_);
        return *this;
    }

    ~MQueueDescriptor()
    {
        if (value_ != -1)
        {
            mq_close(value_);
        }
    }

public:
    static liberror::Result<MQueueDescriptor> create(std::string_view name, asio::io_context* context, int flag, int mode);
    static liberror::Result<MQueueDescriptor> create(std::string_view name, int flag, int mode);
    static liberror::Result<MQueueDescriptor> create(std::string_view name, asio::io_context* context, int flag);
    static liberror::Result<MQueueDescriptor> create(std::string_view name, int flag);

    auto value() const { return value_; }

    liberror::Result<std::vector<char>> receive() const;

    asio::awaitable<std::vector<char>> receive_async()
    {
        co_await stream_->async_read_some(asio::null_buffers(), asio::use_awaitable);
        co_return MUST(receive());
    }

    liberror::Result<void> send(std::ranges::random_access_range auto data) const
    {
        if (mq_send(value_, data.data(), data.size(), 0) < 0)
        {
            return liberror::make_error(strerror(errno));
        }

        return {};
    }

private:
    mqd_t value_;
    std::optional<asio::posix::stream_descriptor> stream_;
};
