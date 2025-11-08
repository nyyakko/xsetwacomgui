#include "platform/mqueue/MQueueDescriptor.hpp"

using namespace liberror;

Result<MQueueDescriptor> MQueueDescriptor::create(std::string_view name, asio::io_context* context, int flag, int mode)
{
    MQueueDescriptor descriptor {};

    struct mq_attr attributes {};

    attributes.mq_flags = 0;
    attributes.mq_maxmsg = 10;
    attributes.mq_msgsize = 256;
    attributes.mq_curmsgs = 0;

    descriptor.value_ = mq_open(name.data(), flag, mode, &attributes);

    if (descriptor.value_ < 0)
    {
        return make_error(strerror(errno));
    }

    if (context != nullptr)
    {
        descriptor.stream_ = asio::posix::stream_descriptor(*context, descriptor.value_);
    }

    return descriptor;
}

Result<MQueueDescriptor> MQueueDescriptor::create(std::string_view name, int flag, int mode)
{
    return MQueueDescriptor::create(name, nullptr, flag, mode);
}

Result<MQueueDescriptor> MQueueDescriptor::create(std::string_view name, asio::io_context* context, int flag)
{
    MQueueDescriptor descriptor {};

    descriptor.value_ = mq_open(name.data(), flag);

    if (descriptor.value_ < 0)
    {
        return make_error(strerror(errno));
    }

    if (context != nullptr)
    {
        descriptor.stream_ = asio::posix::stream_descriptor(*context, descriptor.value_);
    }

    return descriptor;
}

Result<MQueueDescriptor> MQueueDescriptor::create(std::string_view name, int flag)
{
    return MQueueDescriptor::create(name, nullptr, flag);
}

Result<std::vector<char>> MQueueDescriptor::receive() const
{
    std::vector<char> buffer(256);

    auto bytesReceived = mq_receive(value_, buffer.data(), buffer.size(), nullptr);
    if (bytesReceived < 0)
    {
        return make_error(strerror(errno));
    }

    buffer.resize(size_t(bytesReceived + 1));

    return buffer;
}
