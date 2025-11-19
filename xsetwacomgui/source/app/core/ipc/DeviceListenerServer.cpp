#include <spdlog/spdlog.h>

#include "app/core/ipc/DeviceListenerServer.hpp"

#include "platform/udev/UDevDevice.hpp"
#include "platform/udev/UDevMonitor.hpp"

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/executor_work_guard.hpp>
#include <asio/thread_pool.hpp>
#include <liberror/Try.hpp>
#include <magic_enum/magic_enum.hpp>

#include <fcntl.h>
#include <syslog.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <unistd.h>

#include <csignal>

using namespace liberror;

static constexpr auto SERVER_NAME = "/" NAME "-server";

DeviceListenerServer::~DeviceListenerServer()
{
    if (mqueue_.descriptor().value() != -1)
    {
        spdlog::info("Server finished");
    }
}

Result<DeviceListenerServer> DeviceListenerServer::create(asio::io_context& context)
{
    DeviceListenerServer server {};

    auto mqueue = MQueue::create(SERVER_NAME, &context);
    if (!mqueue.has_value())
    {
        if (mqueue.error().message() == strerror(EEXIST))
            spdlog::warn("Server already running");
        else
            spdlog::error("Could not create server mqueue: {}", mqueue.error().message());
        std::exit(EXIT_FAILURE);
    }

    server.mqueue_ = std::move(*mqueue);
    server.context_ = &context;

    return server;
}

void DeviceListenerServer::start()
{
    spdlog::info("Server started");

    asio::thread_pool pool(8);

    asio::post(pool, [&] { context_->run(); });

    asio::co_spawn(pool, message_receiver(), asio::detached);
    asio::co_spawn(pool, message_sender(), asio::detached);

    pool.join();

    assert(false && "UNREACHABLE");
}

asio::awaitable<void> DeviceListenerServer::message_receiver()
{
    while (true)
    {
        auto buffer = co_await mqueue_.receive_async();
        std::string_view message(buffer);

#if DEBUG
        spdlog::info("Received '{}'", message);
#endif

        if (message.starts_with("CONN"))
        {
            auto clientName = std::next(message.data(), 5);
            auto client = MQueueDescriptor::create(clientName, O_WRONLY);
            if (!client.has_value())
            {
                spdlog::error("Could not create client descriptor: {}", strerror(errno));
                std::exit(EXIT_FAILURE);
            }

            clients_.with([&] (auto& clients) { clients.insert({ clientName, std::move(*client) }); });
            spdlog::info("Client {} connected", clientName);
        }

        if (message.starts_with("QUIT"))
        {
            auto clientName = std::next(message.data(), 5);
            clients_.with([&] (auto& clients) { clients.erase(clientName); });
            spdlog::info("Client {} disconnected", clientName);
        }
    }

    co_return;
}

asio::awaitable<void> DeviceListenerServer::message_sender()
{
    UDevMonitor monitor(*context_);
    monitor.add_subsystem("usb");
    monitor.enable();

    while (true)
    {
        auto device = co_await monitor.get_device_async();
        if (!device.get_devnode()) continue;

        clients_.with([&device] (auto const& clients) {
            auto action = magic_enum::enum_name<UDevDevice::Action>(device.get_action());

            for (auto const& client : clients)
            {
                auto maybeSent = client.second.send(action);
                if (!maybeSent.has_value())
                {
                    spdlog::error("Send failed: {}", maybeSent.error().message());
                    std::exit(EXIT_FAILURE);
                }
#if DEBUG
                spdlog::info("Sent '{}' to client {}", action, client.first);
#endif
            }
        });
    }
}
