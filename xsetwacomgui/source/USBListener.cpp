#include "USBListener.hpp"

#include <fplus/fplus.hpp>
#include <liberror/Try.hpp>
#include <poll.h>

#include <ranges>

liberror::Result<void> USBListener::update()
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
                    for (auto const& listener : listeners)
                        TRY(listener(node, Event::from_string(std::string(actionUppercase.begin(), actionUppercase.end()))));
                }

                udev_device_unref(device);
            }
        }
    }

    return {};
}

void USBListener::add_listener(std::function<listener_t> listener)
{
    listeners.push_back(std::move(listener));
}
