#pragma once

#include "platform/mqueue/MQueue.hpp"

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <liberror/Result.hpp>
#include <liberror/Try.hpp>

#include <mqueue.h>

class IPCClient
{
public:
    IPCClient()
        : name_{}
        , client_{-1}
        , mqueue_{}
    {}

    IPCClient(IPCClient const&) = delete;
    IPCClient& operator=(IPCClient const&) = delete;

    IPCClient(IPCClient&& that)
        : name_{std::move(that.name_)}
        , client_{std::exchange(that.client_, -1)}
        , mqueue_{std::move(that.mqueue_)}
    {}

    IPCClient& operator=(IPCClient&& that)
    {
        this->name_ = std::move(that.name_);
        this->client_ = std::exchange(that.client_, -1);
        this->mqueue_ = std::move(that.mqueue_);
        return *this;
    }

    ~IPCClient();

public:
    static liberror::Result<IPCClient> create(asio::io_context& context);

    liberror::Result<void> connect() const;

    asio::awaitable<std::vector<char>> receive_message_async();
    liberror::Result<std::vector<char>> receive_message() const;

private:
    std::string name_;
    mqd_t client_;
    MQueue mqueue_;
};
