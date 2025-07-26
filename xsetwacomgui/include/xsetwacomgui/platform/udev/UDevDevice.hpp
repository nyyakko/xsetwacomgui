#pragma once

#include <algorithm>
#include <libenum/Enum.hpp>

#include <libudev.h>

#include <memory>

class UDevDevice
{
    using device_t = std::unique_ptr<struct udev_device, decltype(&udev_device_unref)>;
public:
    ENUM_CLASS(Action,
        UNBIND,
        REMOVE,
        ADD,
        BIND
    )

public:
    UDevDevice()
        : device_ { nullptr, &udev_device_unref }
    {}

    UDevDevice(UDevDevice const&) = delete;
    UDevDevice operator=(UDevDevice const&) = delete;

    UDevDevice(UDevDevice&& that)
        : device_ { std::move(that.device_) }
    {}

    UDevDevice& operator=(UDevDevice&& that)
    {
        this->device_ = std::move(that.device_);
        return *this;
    }

    explicit UDevDevice(struct udev_device* device)
        : device_ { device, &udev_device_unref }
    {}

public:
    auto get(this auto& self) { return self.device_.get(); }
    auto get_devnode(this auto& self) { return udev_device_get_devnode(self.device_.get()); }
    auto get_action(this auto& self)
    {
        std::string action(udev_device_get_action(self.device_.get()));
        std::transform(action.begin(), action.end(), action.begin(), ::toupper);
        return Action::from_string(action);
    }

public:
    operator bool(this auto&& self) { return self.device_.get(); }

private:
    device_t device_;
};
