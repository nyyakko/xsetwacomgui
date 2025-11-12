#pragma once

#include "UDev.hpp"
#include "platform/udev/UDevDevice.hpp"

#include <asio/awaitable.hpp>
#include <asio/buffer.hpp>
#include <asio/io_context.hpp>
#include <asio/posix/stream_descriptor.hpp>
#include <asio/use_awaitable.hpp>
#include <libudev.h>

#include <string_view>
#include <memory>

class UDevMonitor
{
    using deleter_t = decltype(&udev_monitor_unref);

public:
    explicit UDevMonitor(asio::io_context& context)
        : udev_{}
        , monitor_{udev_monitor_new_from_netlink(udev_.get(), "udev"), udev_monitor_unref}
        , stream_{context, udev_monitor_get_fd(monitor_.get())}
    {}

public:
    // cppcheck-suppress [functionStatic, constParameterReference]
    void enable(this auto& self)
    {
        udev_monitor_enable_receiving(self.monitor_.get());
    }

    // cppcheck-suppress [functionStatic, constParameterReference]
    void add_subsystem(this auto& self, std::string_view subsystem)
    {
        udev_monitor_filter_add_match_subsystem_devtype(self.monitor_.get(), subsystem.data(), NULL);
    }

    asio::awaitable<UDevDevice> get_device_async()
    {
        co_await stream_.async_read_some(asio::null_buffers(), asio::use_awaitable);
        co_return udev_monitor_receive_device(monitor_.get());
    }

    // cppcheck-suppress [functionStatic, constParameterReference]
    auto get(this auto& self) { return self.monitor_.get(); }

private:
    UDev udev_;
    std::unique_ptr<struct udev_monitor, deleter_t> monitor_;
    asio::posix::stream_descriptor stream_;
};
