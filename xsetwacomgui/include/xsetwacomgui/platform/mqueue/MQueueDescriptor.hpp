#pragma once

#include <liberror/Result.hpp>

#include <mqueue.h>

class MQueueDescriptor
{
public:
    MQueueDescriptor()
        : value_{-1}
    {}

    MQueueDescriptor(MQueueDescriptor const&) = delete;
    MQueueDescriptor& operator=(MQueueDescriptor const&) = delete;

    MQueueDescriptor(MQueueDescriptor&& that)
        : value_{std::exchange(that.value_, -1)}
    {}

    MQueueDescriptor& operator=(MQueueDescriptor&& that)
    {
        this->value_ = std::exchange(that.value_, -1);
        return *this;
    }

    ~MQueueDescriptor()
    {
        if (value_ != -1)
        {
            mq_close(value_);
        }
    }

public:
    static liberror::Result<MQueueDescriptor> create(std::string_view name, int flag, int mode);
    static liberror::Result<MQueueDescriptor> create(std::string_view name, int flag);

    auto value(this auto& self) { return self.value_; }

    liberror::Result<std::vector<char>> receive() const;

    liberror::Result<void> send(std::ranges::random_access_range auto data) const
    {
        if (mq_send(value_, data.data(), data.size(), 0) < 0)
        {
            return liberror::make_error(strerror(errno));
        }

        return {};
    }

private:
    mqd_t value_;
};
