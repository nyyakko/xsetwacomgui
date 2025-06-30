#pragma once

#include <libenum/Enum.hpp>
#include <liberror/Result.hpp>
#include <libudev.h>

using udev_deleter_t = decltype(&udev_unref);
using udev_monitor_deleter_t = decltype(&udev_monitor_unref);

class USBEvent
{
public:
    static constexpr auto MAGIC = 3;

    ENUM_CLASS(Action,
        UNBIND,
        REMOVE,
        ADD,
        BIND
    )

private:
    using listener_t = liberror::Result<void>(std::string_view node, Action event);

public:
    USBEvent()
        : udev(udev_new(), udev_unref)
        , monitor(udev_monitor_new_from_netlink(udev.get(), "udev"), udev_monitor_unref)
        , monitorFd(udev_monitor_get_fd(monitor.get()))
    {
        udev_monitor_filter_add_match_subsystem_devtype(monitor.get(), "usb", NULL);
        udev_monitor_enable_receiving(monitor.get());
    }

    void subscribe(std::function<listener_t> const& listener);
    liberror::Result<void> update() const;
    liberror::Result<void> notify_all(std::string_view node, Action event) const;

private:
    std::unique_ptr<struct udev, udev_deleter_t> udev;
    std::unique_ptr<struct udev_monitor, udev_monitor_deleter_t> monitor;
    int monitorFd;

    std::vector<std::function<listener_t>> listeners {};
};

