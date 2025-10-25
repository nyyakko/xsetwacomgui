#include <spdlog/spdlog.h>

#include "app/core/ipc/IPCClient.hpp"

#include <mqueue.h>
#include <unistd.h>

#include <csignal>

using namespace liberror;

static constexpr auto SERVER_NAME = "/" NAME "-server";
static constexpr auto CLIENT_NAME = "/" NAME "-client";

IPCClient::~IPCClient()
{
    if (mqueue_.descriptor().value() != -1)
    {
        auto server = MUST(MQueueDescriptor::create(SERVER_NAME, O_WRONLY));
        MUST(server.send(fmt::format("QUIT {}", name_.data())));
    }
}

Result<IPCClient> IPCClient::create()
{
    IPCClient client {};
    client.name_ = fmt::format("{}-{}", CLIENT_NAME, getpid());
    return client;
}

Result<void> IPCClient::configure(Mode mode)
{
    mqueue_ = TRY(MQueue::create(name_, O_RDONLY | O_CREAT | (mode == Mode::ASYNC ? O_NONBLOCK : 0)));
    return {};
}

Result<void> IPCClient::connect()
{
    using namespace std::literals;

    if (mqueue_.descriptor().value() == -1)
    {
        return make_error("Client descriptor was -1. did you call configure?");
    }

    MQueueDescriptor server {};

    while (true)
    {
        auto maybeServer = MQueueDescriptor::create(SERVER_NAME, O_WRONLY);
        if (maybeServer.has_value())
        {
            server = std::move(*maybeServer);
            break;
        }

        static auto retry = 0;

        spdlog::warn("Could not connect to IPCServer ({}/3)", retry+1);

        if (retry++ == 3)
        {
            return make_error("Failed to connect to IPCServer");
        }

        std::this_thread::sleep_for(1s);
    }

    TRY(server.send(fmt::format("CONN {}", name_.data())));

    return {};
}

Result<std::vector<char>> IPCClient::receive_message_async()
{
    auto received = mqueue_.receive();

    if (!(received.has_value() || received.error().message() == strerror(EAGAIN)))
    {
        return make_error(received.error().message());
    }
    else
    {
        if (!received.has_value() && received.error().message() == strerror(EAGAIN))
            return {};
        else
            return *received;
    }
}

Result<std::vector<char>> IPCClient::receive_message()
{
    return TRY(mqueue_.receive());
}
