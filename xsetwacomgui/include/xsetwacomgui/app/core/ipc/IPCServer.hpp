#pragma once

#include "app/core/MutexProtected.hpp"
#include "platform/mqueue/MQueueDescriptor.hpp"
#include "platform/mqueue/MQueue.hpp"

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <liberror/Result.hpp>
#include <liberror/Try.hpp>

#include <mqueue.h>
#include <sys/poll.h>

class IPCServer
{
public:
    IPCServer()
        : mqueue_{}
        , clients_{}
    {}

    IPCServer(IPCServer const&) = delete;
    IPCServer& operator=(IPCServer const&) = delete;

    IPCServer(IPCServer&& that)
        : mqueue_(std::move(that.mqueue_))
        , clients_(std::move(that.clients_))
    {}

    IPCServer& operator=(IPCServer&& that)
    {
        this->mqueue_ = std::move(that.mqueue_);
        this->clients_ = std::move(that.clients_);
        return *this;
    }

    ~IPCServer();

public:
    static IPCServer& the()
    {
        static auto the = MUST(IPCServer::create());
        return the;
    }

    void start();

private:
    static liberror::Result<IPCServer> create();

    asio::awaitable<void> message_receiver(asio::io_context& context);
    asio::awaitable<void> message_sender(asio::io_context& context);

private:
    MQueue mqueue_;
    MutexProtected<std::unordered_map<std::string, MQueueDescriptor>> clients_;
};
