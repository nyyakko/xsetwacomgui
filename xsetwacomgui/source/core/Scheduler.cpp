#include "core/Scheduler.hpp"

#include <coro/sync_wait.hpp>
#include <liberror/Try.hpp>

using namespace liberror;

void Scheduler::run(coro::task<std::function<liberror::Result<void>()>> task)
{
    auto worker = [] (auto& pool, auto task) -> coro::task<std::function<liberror::Result<void>()>> {
        co_await pool->schedule();
        co_return co_await task;
    }(pool_, std::move(task));

    worker.resume();

    tasks_.push(std::move(worker));
}

Result<void> Scheduler::update()
{
    if (tasks_.empty() || !tasks_.front().is_ready())
    {
        return {};
    }

    auto task = std::move(tasks_.front());
    tasks_.pop();
    TRY(std::invoke(coro::sync_wait(task)));

    return {};
}
