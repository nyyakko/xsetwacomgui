#include "platform/mqueue/MQueue.hpp"

#include <liberror/Try.hpp>

using namespace liberror;

Result<MQueue> MQueue::create(std::string_view name, asio::io_context* context, int flag)
{
    MQueue queue {};
    queue.name_ = name;
    queue.descriptor_ = TRY(MQueueDescriptor::create(name, context, flag, 0660));
    return queue;
}
