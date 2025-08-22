#include <spdlog/spdlog.h>
#include <spdlog/sinks/syslog_sink.h>

#include "platform/Daemon.hpp"

#include <fmt/format.h>

#include <fcntl.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <unistd.h>

#include <algorithm>
#include <ranges>

using namespace liberror;

Result<void> daemonize()
{
    umask(0);

    auto const pid = fork();

    if (pid > 0)
    {
        std::exit(0);
    }
    else if (pid < 0)
    {
        return make_error("fork failed: {}", strerror(errno));
    }

    setsid();

    if (chdir("/") < 0)
    {
        return make_error("chdir failed: {}", strerror(errno));
    }

    std::ranges::for_each(std::views::iota(0, 1024), close);

    auto fd0 = open("/dev/null", O_RDWR);
    auto fd1 = dup(0);
    auto fd2 = dup(0);

    openlog("xsetwacomgui-daemon", LOG_CONS, LOG_DAEMON);

    auto sink = std::make_shared<spdlog::sinks::syslog_sink_mt>(NAME"-daemon", LOG_PID, LOG_LOCAL0, false);
    auto logger = std::make_shared<spdlog::logger>(NAME"-daemon", sink);

    spdlog::set_default_logger(logger);

    if (fd0 != 0 || fd1 != 1 || fd2 != 2)
    {
        spdlog::error("unexpected file descriptors {} {} {}", fd0, fd1, fd2);
        std::abort();
    }

    spdlog::info("listening for device connections...");

    return {};
}

