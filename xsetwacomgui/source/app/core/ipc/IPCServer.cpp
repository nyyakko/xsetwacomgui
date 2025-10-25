#include <spdlog/spdlog.h>

#include "app/core/ipc/IPCServer.hpp"

#include "platform/udev/UDevDevice.hpp"
#include "platform/udev/UDevMonitor.hpp"

#include <asio.hpp>
#include <liberror/Try.hpp>
#include <magic_enum/magic_enum.hpp>

#include <fcntl.h>
#include <syslog.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <unistd.h>

#include <functional>
#include <csignal>

using namespace liberror;

static constexpr auto SERVER_NAME = "/" NAME "-server";

IPCServer::~IPCServer()
{
    if (mqueue_.descriptor().value() != -1)
    {
        spdlog::info("Server finished");
    }
}

Result<IPCServer> IPCServer::create()
{
    IPCServer server {};

    auto mqueue = MQueue::create(SERVER_NAME);
    if (!mqueue.has_value())
    {
        if (mqueue.error().message() == strerror(EEXIST))
            spdlog::warn("Server already running");
        else
            spdlog::error("Could not create server mqueue: {}", mqueue.error().message());
        std::exit(EXIT_FAILURE);
    }

    server.mqueue_ = std::move(*mqueue);

    return server;
}

void IPCServer::start()
{
    spdlog::info("Server started");

    asio::thread_pool pool(8);

    asio::post(pool, std::bind_front(std::mem_fn(&IPCServer::message_receiver), this));
    asio::post(pool, std::bind_front(std::mem_fn(&IPCServer::message_sender), this));

    pool.join();

    assert(false && "UNREACHABLE");
}

void IPCServer::message_receiver()
{
    while (true)
    {
        auto buffer = mqueue_.receive();
        if (!buffer.has_value())
        {
            spdlog::error("Receive failed: {}", buffer.error().message());
            std::exit(EXIT_FAILURE);
        }

        std::string_view message(*buffer);

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
}

void IPCServer::message_sender()
{
    UDev udev;

    UDevMonitor monitor(udev);
    monitor.add_subsystem("usb");
    monitor.enable();

    pollfd fd {
        .fd=udev_monitor_get_fd(monitor.get()),
        .events=POLLIN,
        .revents={}
    };

    while (true)
    {
        auto result = poll(&fd, 1, -1);

        if (result == 0) continue;
        if (result < 0)
        {
            spdlog::error("Poll failed: {}", strerror(errno));
            std::exit(EXIT_FAILURE);
        }

        UDevDevice device(udev_monitor_receive_device(monitor.get()));

        if (!device.get_devnode()) continue;

        auto action = magic_enum::enum_name<UDevDevice::Action>(device.get_action());

        for (auto const& client : clients_.with([] (auto& clients) -> auto& { return clients; }))
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
    }
}
