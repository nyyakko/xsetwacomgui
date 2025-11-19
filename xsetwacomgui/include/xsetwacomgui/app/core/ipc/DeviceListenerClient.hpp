#pragma once

#include "platform/mqueue/MQueue.hpp"

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <liberror/Result.hpp>
#include <liberror/Try.hpp>

#include <mqueue.h>

class DeviceListenerClient
{
public:
    DeviceListenerClient()
        : name_{}
        , client_{-1}
        , mqueue_{}
    {}

    DeviceListenerClient(DeviceListenerClient const&) = delete;
    DeviceListenerClient& operator=(DeviceListenerClient const&) = delete;

    DeviceListenerClient(DeviceListenerClient&& that)
        : name_{std::move(that.name_)}
        , client_{std::exchange(that.client_, -1)}
        , mqueue_{std::move(that.mqueue_)}
    {}

    DeviceListenerClient& operator=(DeviceListenerClient&& that)
    {
        this->name_ = std::move(that.name_);
        this->client_ = std::exchange(that.client_, -1);
        this->mqueue_ = std::move(that.mqueue_);
        return *this;
    }

    ~DeviceListenerClient();

public:
    static liberror::Result<DeviceListenerClient> create(asio::io_context& context);

    liberror::Result<void> connect() const;

    asio::awaitable<std::vector<char>> receive_message_async();
    liberror::Result<std::vector<char>> receive_message() const;

private:
    std::string name_;
    mqd_t client_;
    MQueue mqueue_;
};
