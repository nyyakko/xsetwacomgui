#pragma once

#include "platform/mqueue/MQueue.hpp"

#include <liberror/Result.hpp>
#include <liberror/Try.hpp>

#include <mqueue.h>

class IPCClient
{
public:
    enum class Mode { SYNC, ASYNC };

public:
    IPCClient() : name_{}, client_{-1} {}

    IPCClient(IPCClient const&) = delete;
    IPCClient& operator=(IPCClient const&) = delete;

    IPCClient(IPCClient&& that)
        : name_(std::move(that.name_))
        , client_(std::exchange(that.client_, -1))
    {}

    IPCClient& operator=(IPCClient&& that)
    {
        this->name_ = std::move(that.name_);
        this->client_ = std::exchange(that.client_, -1);
        return *this;
    }

    ~IPCClient();

public:
    static IPCClient& the()
    {
        static auto the = MUST(IPCClient::create());
        return the;
    }

    liberror::Result<void> configure(Mode mode);

    liberror::Result<void> connect();

    liberror::Result<std::vector<char>> receive_message_async();
    liberror::Result<std::vector<char>> receive_message();

private:
    static liberror::Result<IPCClient> create();

    MQueue mqueue_;

    std::string name_;
    mqd_t client_;
};
