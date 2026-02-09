#pragma once

#include "app/core/concurrency/MutexProtected.hpp"
#include "platform/mqueue/MQueueDescriptor.hpp"
#include "platform/mqueue/MQueue.hpp"

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <liberror/Result.hpp>
#include <liberror/Try.hpp>

#include <mqueue.h>
#include <sys/poll.h>

class DeviceListenerServer
{
public:
    DeviceListenerServer()
        : mqueue_{}
        , clients_{}
        , context_{nullptr}
    {}

    DeviceListenerServer(DeviceListenerServer const&) = delete;
    DeviceListenerServer& operator=(DeviceListenerServer const&) = delete;

    DeviceListenerServer(DeviceListenerServer&& that)
        : mqueue_{std::move(that.mqueue_)}
        , clients_{std::move(that.clients_)}
        , context_{std::exchange(that.context_, nullptr)}
    {}

    DeviceListenerServer& operator=(DeviceListenerServer&& that)
    {
        this->mqueue_ = std::move(that.mqueue_);
        this->clients_ = std::move(that.clients_);
        this->context_ = std::exchange(that.context_, nullptr);
        return *this;
    }

    ~DeviceListenerServer();

public:
    static liberror::Result<DeviceListenerServer> create(asio::io_context& context);

    void start();

private:
    asio::awaitable<void> message_receiver();
    asio::awaitable<void> message_sender();

private:
    MQueue mqueue_;
    MutexProtected<std::unordered_map<std::string, MQueueDescriptor>> clients_;
    asio::io_context* context_;
};
