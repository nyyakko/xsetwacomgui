#include "USBAction.hpp"

#include <fplus/fplus.hpp>
#include <liberror/Try.hpp>
#include <poll.h>

#include <ranges>

liberror::Result<void> USBAction::update() const
{
    pollfd pfd;

    pfd.fd = monitorFd;
    pfd.events = POLLIN;

    if (poll(&pfd, 1, 0))
    {
        if (pfd.revents & POLLIN)
        {
            auto device = udev_monitor_receive_device(monitor.get());

            if (device)
            {
                auto node = udev_device_get_devnode(device);
                auto action = udev_device_get_action(device);

                if (node)
                {
                    auto actionUppercase = std::string(action) | std::views::transform(::toupper);
                    notify_all(node, Event::from_string(std::string(actionUppercase.begin(), actionUppercase.end())));
                }

                udev_device_unref(device);
            }
        }
    }

    return {};
}

liberror::Result<void> USBAction::notify_all(std::string_view node, Event event) const
{
    for (auto const& listener : listeners)
        TRY(listener(node, event));
    return {};
}

void USBAction::subscribe(std::function<listener_t> const& listener)
{
    listeners.push_back(listener);
}
