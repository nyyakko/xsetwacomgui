#include <spdlog/spdlog.h>

#include "core/ipc/Server.hpp"

#include "platform/udev/UDevDevice.hpp"
#include "platform/udev/UDevMonitor.hpp"

#include <libcoro/Task.hpp>
#include <liberror/Try.hpp>
#include <magic_enum/magic_enum.hpp>

#include <sys/poll.h>
#include <sys/syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/stat.h>

#include <csignal>

using namespace liberror;
using namespace libcoro;

static constexpr auto SERVER_NAME = "/" NAME "-server";

static Task<std::array<char, 32>> receive(mqd_t fd);

IPCServer::~IPCServer()
{
    if (this->server_ != -1)
    {
        spdlog::info("Server finished");

        mq_close(this->server_);
        mq_unlink(SERVER_NAME);
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

Result<void> IPCServer::start()
{
    std::thread schedulerThread {
        [] {
            Scheduler::the().start();
        }
    };

    Scheduler::the().schedule(message_receiver());
    Scheduler::the().schedule(message_sender());

    spdlog::info("Server started");

    schedulerThread.join();

    return {};
}

Task<void> IPCServer::message_receiver()
{
    while (true)
    {
        std::string_view buffer(co_await receive(server_));

        if (buffer.starts_with("CONN"))
        {
            auto clientName = std::next(buffer.data(), 5);
            auto clientFd = mq_open(clientName, O_WRONLY);

            if (clientFd < 0)
            {
                spdlog::error("IPCServer::{}: mq_open failed: {}", __FUNCTION__, strerror(errno));
                std::exit(EXIT_FAILURE);
            }

            clients_.with([=] (auto& clients) {
                clients.insert({ clientName, clientFd });
            });

            spdlog::info("Client {} connected", clientName);
        }
        else if (buffer.starts_with("DISC"))
        {
            auto clientName = std::next(buffer.data(), 5);

            clients_.with([&] (auto& clients) {
                clients.erase(clientName);
            });

            spdlog::info("Client {} disconnected", clientName);
        }
    }

    co_return {};
}

Task<void> IPCServer::message_sender()
{
    UDev udev;

    UDevMonitor monitor(udev);
    monitor.add_subsystem("usb");
    monitor.enable();

    std::vector<pollfd> fds {
        {
            .fd=udev_monitor_get_fd(monitor.get()),
            .events=POLLIN,
            .revents={}
        }
    };

    while (true)
    {
        if (poll(fds.data(), fds.size(), -1) <= 0) continue;

        UDevDevice device(udev_monitor_receive_device(monitor.get()));

        if (!device.get_devnode()) continue;

        auto action = magic_enum::enum_name<UDevDevice::Action>(device.get_action());

        for (auto [_, clientFd] : clients_.with([] (auto const& clients) { return clients; }))
        {
            if (mq_send(clientFd, action.data(), action.size(), 0) < 0)
            {
                spdlog::error("IPCServer::{}: mq_send failed: {}", __FUNCTION__, strerror(errno));
                std::exit(EXIT_FAILURE);
            }
        }
    }

    co_return {};
}

Task<std::array<char, 32>> receive(mqd_t fd)
{
    std::array<char, 32> buffer {};

    if (mq_receive(fd, buffer.data(), buffer.size(), nullptr) < 0)
    {
        spdlog::error("{}: mq_receive failed: {}", __FUNCTION__, strerror(errno));
        std::exit(EXIT_FAILURE);
    }

    co_return buffer;
}
