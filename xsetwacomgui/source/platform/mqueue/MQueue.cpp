#include "platform/mqueue/MQueue.hpp"

#include <liberror/Try.hpp>

using namespace liberror;

Result<MQueue> MQueue::create(std::string_view name, int flag)
{
    MQueue queue {};
    queue.name_ = name;
    queue.descriptor_ = TRY(MQueueDescriptor::create(name, flag, 0660));
    return queue;
}
