#include <spdlog/spdlog.h>

#include "core/ipc/Server.hpp"

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
    if (this->server_ != -1)
    {
        stop();
    }
}

Result<IPCServer> IPCServer::create()
{
    IPCServer server;

    struct mq_attr attributes {};

    attributes.mq_flags = 0;
    attributes.mq_maxmsg = 10;
    attributes.mq_msgsize = 32;
    attributes.mq_curmsgs = 0;

    auto serverFd = mq_open(SERVER_NAME, O_RDONLY | O_CREAT | O_EXCL, 0660, &attributes);
    if (serverFd < 0)
    {
        if (errno == EEXIST)
        {
#if DEBUG
            spdlog::info("IPCServer::{}: server already running", __FUNCTION__);
#endif
            std::exit(EXIT_FAILURE);
        }

        return make_error("IPCServer::{}: mq_open failed: {}", __FUNCTION__, strerror(errno));
    }

    server.server_ = serverFd;

    struct sigaction action;

    action.sa_handler = [] (int) {
        IPCServer::the().~IPCServer();
        _exit(0);
    };

    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);

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

void IPCServer::stop()
{
    spdlog::info("Server finished");

    mq_close(this->server_);
    mq_unlink(SERVER_NAME);
}

void IPCServer::message_receiver()
{
    while (true)
    {
        std::array<char, 32> buffer {};

        if (mq_receive(server_, buffer.data(), buffer.size(), nullptr) < 0)
        {
            spdlog::error("{}: mq_receive failed: {}", __FUNCTION__, strerror(errno));
            std::exit(EXIT_FAILURE);
        }

        std::string_view message(buffer);

        if (message.starts_with("CONN"))
        {
            auto clientName = std::next(message.data(), 5);
            auto clientFd = mq_open(clientName, O_WRONLY);

            if (clientFd < 0)
            {
                spdlog::error("IPCServer::{}: mq_open failed: {}", __FUNCTION__, strerror(errno));
                std::exit(EXIT_FAILURE);
            }

            clients_.with([=] (auto& clients) { clients.insert({ clientName, clientFd }); });
            spdlog::info("Client {} connected", clientName);
        }
        else if (message.starts_with("QUIT"))
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
            spdlog::error("IPCServer::{}: poll failed: {}", __FUNCTION__, strerror(errno));
            std::exit(EXIT_FAILURE);
        }

        UDevDevice device(udev_monitor_receive_device(monitor.get()));

        if (!device.get_devnode()) continue;

        auto action = magic_enum::enum_name<UDevDevice::Action>(device.get_action());

        for (auto [clientName, clientFd] : clients_.with([] (auto const& clients) { return clients; }))
        {
            if (mq_send(clientFd, action.data(), action.size(), 0) < 0)
            {
                spdlog::error("IPCServer::{}: mq_send failed: {}", __FUNCTION__, strerror(errno));
                std::exit(EXIT_FAILURE);
            }

#if DEBUG
            spdlog::info("IPCServer::{}: sent message to client {}", __FUNCTION__, clientName);
#endif
        }
    }
}
