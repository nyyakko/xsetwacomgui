#pragma once

#include <libudev.h>

#include <memory>

class UDev
{
    using deleter_t = decltype(&udev_unref);

public:
    explicit UDev()
        : udev_(udev_new(), udev_unref)
    {}

    auto get(this auto& self) { return self.udev_.get(); }

private:
    std::unique_ptr<struct udev, deleter_t> udev_;
};
