#pragma once

#include <magic_enum/magic_enum.hpp>

#include <libudev.h>

#include <algorithm>
#include <memory>

class UDevDevice
{
    using device_t = std::unique_ptr<struct udev_device, decltype(&udev_device_unref)>;
public:
    enum class Action { UNBIND, REMOVE, ADD, BIND };

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

    auto get_devnode(this auto& self)
    {
        return self.device_.get() ? udev_device_get_devnode(self.device_.get()) : nullptr;
    }

    auto get_action(this auto& self)
    {
        assert(self.device_.get() && "DEVICE POINTER WAS NULLPTR");
        std::string action(udev_device_get_action(self.device_.get()));
        std::transform(action.begin(), action.end(), action.begin(), ::toupper);
        return *magic_enum::enum_cast<Action>(action);
    }

public:
    operator bool(this auto&& self) { return self.device_.get(); }

private:
    device_t device_;
};
