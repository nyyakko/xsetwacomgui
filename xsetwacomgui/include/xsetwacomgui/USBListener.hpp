#pragma once

#include <libenum/Enum.hpp>
#include <liberror/Result.hpp>
#include <libudev.h>

class USBListener
{
public:
    ENUM_CLASS(Event,
        UNBIND,
        REMOVE,
        ADD,
        BIND
    )

private:
    using udev_deleter_t = decltype([] (udev* udev) {
        udev_unref(udev);
    });

    using udev_monitor_deleter_t = decltype([] (udev_monitor* monitor) {
        udev_monitor_unref(monitor);
    });

    using listener_t = liberror::Result<void>(std::string_view node, Event event);

public:
    USBListener()
        : udev(udev_new(), udev_deleter_t{})
        , monitor(udev_monitor_new_from_netlink(udev.get(), "udev"), udev_monitor_deleter_t{})
        , monitorFd(udev_monitor_get_fd(monitor.get()))
    {
        udev_monitor_filter_add_match_subsystem_devtype(monitor.get(), "usb", NULL);
        udev_monitor_enable_receiving(monitor.get());
    }

    liberror::Result<void> update();
    void add_listener(std::function<listener_t> listener);

private:
    std::unique_ptr<struct udev, udev_deleter_t> udev;
    std::unique_ptr<struct udev_monitor, udev_monitor_deleter_t> monitor;
    int monitorFd;

    std::vector<std::function<listener_t>> listeners {};
};

