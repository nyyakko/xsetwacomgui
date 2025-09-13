#include <spdlog/fmt/bundled/base.h>
#include <spdlog/spdlog.h>

#include "core/ipc/Client.hpp"

#include <mqueue.h>
#include <unistd.h>

#include <random>
#include <csignal>

using namespace liberror;
using namespace libcoro;

static constexpr auto SERVER_NAME = "/" NAME "-server";
static constexpr auto CLIENT_NAME = "/" NAME "-client";

IPCClient::~IPCClient()
{
    if (client_ != -1)
    {
        disconnect();

        mq_close(client_);
        mq_unlink(name_.data());
    }
}

Result<IPCClient> IPCClient::create()
{
    struct sigaction action;

    action.sa_handler = [] (int) {
        IPCClient::the().~IPCClient();
        _exit(0);
    };

    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);

    return {};
}

Result<void> IPCClient::configure(Mode mode)
{
    struct mq_attr attributes {};

    attributes.mq_flags = 0;
    attributes.mq_maxmsg = 10;
    attributes.mq_msgsize = 32;
    attributes.mq_curmsgs = 0;

    std::random_device device;
    std::mt19937 generator(device());
    std::uniform_int_distribution<> distribution(1, 32);

    name_ = fmt::format("{}-{}", CLIENT_NAME, distribution(generator));

    auto flags = O_RDONLY | O_CREAT | (mode == Mode::ASYNC ? O_NONBLOCK : 0);
    auto clientFd = mq_open(name_.data(), flags, 0660, &attributes);
    if (clientFd < 0)
    {
        return make_error("IPCClient::{}: mq_open failed: {}", __FUNCTION__, strerror(errno));
    }

    client_ = clientFd;

    return {};
}

Result<void> IPCClient::connect()
{
    using namespace std::literals;

    if (client_ == -1)
    {
        return make_error("IPCClient::{}: client descriptor was -1. Did you call configure?", __FUNCTION__);
    }

    mqd_t serverFd = -1;
    while (serverFd = mq_open(SERVER_NAME, O_WRONLY), serverFd < 0)
    {
        spdlog::warn("IPCClient::{}: timed out, retrying...", __FUNCTION__);

        static int retry = 0;

        if (retry++ == 3)
        {
            return make_error("IPCClient::{}: could not connect to IPCServer.", __FUNCTION__);
        }

        std::this_thread::sleep_for(500ms);
    }

    auto request = fmt::format("CONN {}", this->name_.data());
    if (mq_send(serverFd, request.data(), request.size(), 0) < 0)
    {
        return make_error("IPCClient::{}: mq_send failed: {}", __FUNCTION__, strerror(errno));
    }

    return {};
}

Result<void> IPCClient::disconnect()
{
    auto serverFd = mq_open(SERVER_NAME, O_WRONLY);
    if (serverFd < 0)
    {
        return make_error("IPCClient::{}: mq_open failed: {}", __FUNCTION__, strerror(errno));
    }

    auto request = fmt::format("QUIT {}", this->name_.data());
    if (mq_send(serverFd, request.data(), request.size(), 0) < 0)
    {
        return make_error("IPCClient::{}: mq_send failed: {}", __FUNCTION__, strerror(errno));
    }

    return {};
}

Generator<std::optional<std::array<char, 32>>> IPCClient::receive_message_async()
{
    while (true)
    {
        std::array<char, 32> buffer {};

        auto bytesRead = mq_receive(this->client_, buffer.data(), buffer.size(), nullptr);

        if (bytesRead >= 0 || errno == EAGAIN)
        {
            if (bytesRead < 0 && errno == EAGAIN)
                co_yield std::nullopt;
            else
                co_yield buffer;
        }
        else
        {
            spdlog::error("IPCClient::{}: mq_receive failed: {}", __FUNCTION__, strerror(errno));
            std::exit(EXIT_FAILURE);
        }
    }

    co_return;
}

Result<std::array<char, 32>> IPCClient::receive_message()
{
    std::array<char, 32> buffer {};

    if (mq_receive(this->client_, buffer.data(), buffer.size(), nullptr) < 0)
    {
        spdlog::error("IPCClient::{}: mq_receive failed: {}", __FUNCTION__, strerror(errno));
        std::exit(EXIT_FAILURE);
    }

    return buffer;
}
