#pragma once

#include <coro/task.hpp>
#include <coro/thread_pool.hpp>
#include <liberror/Result.hpp>

#include <queue>

class Scheduler
{
public:
    Scheduler()
        : pool_(coro::thread_pool::make_unique())
        , tasks_{}
    {}

    Scheduler(Scheduler const&) = delete;
    Scheduler& operator=(Scheduler const&) = delete;

    Scheduler(Scheduler&& that)
        : pool_(std::move(that.pool_))
        , tasks_(std::move(that.tasks_))
    {}

    Scheduler& operator=(Scheduler&& that)
    {
        this->pool_ = std::move(that.pool_);
        this->tasks_ = std::move(that.tasks_);
        return *this;
    }

public:
    void run(coro::task<std::function<liberror::Result<void>()>> task);

    liberror::Result<void> update();

private:
    std::unique_ptr<coro::thread_pool> pool_;
    std::queue<coro::task<std::function<liberror::Result<void>()>>> tasks_;
};
