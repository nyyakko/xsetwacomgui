#include <spdlog/spdlog.h>

#include "app/core/ipc/IPCClient.hpp"

#include <range/v3/view/iota.hpp>

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

Result<IPCClient> IPCClient::create(asio::io_context& context)
{
    IPCClient client {};
    client.name_ = fmt::format("{}-{}", CLIENT_NAME, getpid());
    client.mqueue_ = TRY(MQueue::create(client.name_, &context));
    return client;
}

Result<void> IPCClient::connect() const
{
    using namespace std::literals;

    if (mqueue_.descriptor().value() == -1)
    {
        return make_error("Client descriptor was -1. did you call configure?");
    }

    MQueueDescriptor server {};

    for (auto retry : ranges::views::iota(0, 3))
    {
        auto maybeServer = MQueueDescriptor::create(SERVER_NAME, O_WRONLY);
        if (maybeServer.has_value())
        {
            server = std::move(*maybeServer);
            break;
        }

        if (retry+1 == 3) return make_error("Failed to connect to IPCServer");
        spdlog::warn("Could not connect to IPCServer ({}/3)", retry+1);

        std::this_thread::sleep_for(1s);
    }

    TRY(server.send(fmt::format("CONN {}", name_.data())));

    return {};
}

asio::awaitable<std::vector<char>> IPCClient::receive_message_async()
{
    return mqueue_.receive_async();
}

Result<std::vector<char>> IPCClient::receive_message() const
{
    return TRY(mqueue_.receive());
}
