#include <spdlog/spdlog.h>
#include <spdlog/sinks/syslog_sink.h>

#include "platform/Daemon.hpp"

#include <fmt/format.h>
#include <range/v3/algorithm.hpp>

#include <fcntl.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <sys/wait.h>
#include <unistd.h>

#include <ranges>

using namespace liberror;

Result<IsDaemon> daemonize(std::string_view name, Detached detached)
{
    umask(0);

    auto const firstFork = fork();

    if (firstFork < 0)
    {
        return make_error("{}: fork failed: {}", __FUNCTION__, strerror(errno));
    }
    else if (firstFork > 0)
    {
        waitpid(firstFork, nullptr, 0);

        if (detached == Detached::TRUE)
        {
            std::exit(0);
        }

        return IsDaemon::FALSE;
    }

    setsid();

    auto const secondFork = fork();

    if (secondFork > 0)
    {
        std::exit(EXIT_SUCCESS);
    }
    else if (secondFork < 0)
    {
        spdlog::error("{}: fork failed: {}", __FUNCTION__, strerror(errno));
        std::exit(EXIT_FAILURE);
    }

    ranges::for_each(std::views::iota(0, 1024), close);

    auto fd0 = open("/dev/null", O_RDWR);
    auto fd1 = dup(0);
    auto fd2 = dup(0);

    auto sink = std::make_shared<spdlog::sinks::syslog_sink_mt>(name.data(), LOG_PID, LOG_LOCAL0, false);
    auto logger = std::make_shared<spdlog::logger>(name.data(), sink);

    spdlog::set_default_logger(logger);

    if (chdir("/") < 0)
    {
        return make_error("{}: chdir failed: {}", __FUNCTION__, strerror(errno));
    }

    if (fd0 != 0 || fd1 != 1 || fd2 != 2)
    {
        spdlog::error("{}: unexpected file descriptors {} {} {}", __FUNCTION__, fd0, fd1, fd2);
        std::exit(EXIT_FAILURE);
    }

    return IsDaemon::TRUE;
}
