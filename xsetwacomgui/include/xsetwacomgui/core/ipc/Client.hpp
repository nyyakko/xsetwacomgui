#pragma once

#include <libcoro/Generator.hpp>
#include <liberror/Result.hpp>
#include <liberror/Try.hpp>
#include <libcoro/Task.hpp>

#include <mqueue.h>

#include <optional>

class IPCClient
{
public:
    enum class Mode { SYNC, ASYNC };

public:
    IPCClient() : name_{}, client_{-1} {}

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

    static IPCClient& the()
    {
        static auto the = MUST(IPCClient::create());
        return the;
    }

private:
    static liberror::Result<IPCClient> create();

public:
    liberror::Result<void> configure(Mode mode);

    liberror::Result<void> connect();
    liberror::Result<void> disconnect();

    libcoro::Generator<std::optional<std::array<char, 32>>> receive_message_async();
    liberror::Result<std::array<char, 32>> receive_message();

private:
    std::string name_;
    mqd_t client_;
};
