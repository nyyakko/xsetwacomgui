#pragma once

#include "core/MutexProtected.hpp"

#include <liberror/Result.hpp>
#include <liberror/Try.hpp>

#include <mqueue.h>
#include <sys/poll.h>

class IPCServer
{
public:
    IPCServer() : server_{-1}, clients_{} {}

    IPCServer(IPCServer const&) = delete;
    IPCServer& operator=(IPCServer const&) = delete;

    IPCServer(IPCServer&& that)
        : server_(std::exchange(that.server_, -1))
        , clients_(std::move(that.clients_))
    {}

    IPCServer& operator=(IPCServer&& that)
    {
        this->server_ = std::exchange(that.server_, -1);
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

    void stop();

    void message_receiver();
    void message_sender();

private:
    mqd_t server_;
    MutexProtected<std::unordered_map<std::string, mqd_t>> clients_;
};
