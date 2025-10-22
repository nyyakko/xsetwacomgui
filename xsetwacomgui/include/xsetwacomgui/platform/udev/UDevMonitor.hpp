#pragma once

#include "UDev.hpp"

#include <libudev.h>

#include <string_view>
#include <memory>

class UDevMonitor
{
    using deleter_t = decltype(&udev_monitor_unref);

public:
    UDevMonitor(UDev& context)
        : context_(context)
        , monitor_(udev_monitor_new_from_netlink(context_.get(), "udev"), udev_monitor_unref)
    {}

public:
    void enable(this auto& self)
    {
        udev_monitor_enable_receiving(self.monitor_.get());
    }

    void add_subsystem(this auto& self, std::string_view subsystem)
    {
        udev_monitor_filter_add_match_subsystem_devtype(self.monitor_.get(), subsystem.data(), NULL);
    }

    auto get(this auto& self) { return self.monitor_.get(); }

private:
    UDev& context_;
    std::unique_ptr<struct udev_monitor, deleter_t> monitor_;
};
