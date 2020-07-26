#include <Common/Exception.h>
#include <common/logger_useful.h>
#include <Common/MemorySanitizer.h>
#include <Poco/Logger.h>
#include <boost/range/iterator_range.hpp>
#include <errno.h>

#include <IO/IOContextPool.h>


namespace DB
{

namespace ErrorCodes
{
    extern const int CANNOT_IO_SUBMIT;
    extern const int CANNOT_IO_GETEVENTS;
}











// int IOContextPool::getCompletionEvents(io_event events[], const int max_events) const
// {
//     timespec timeout{timeout_sec, 0};

//     auto num_events = 0;

//     /// request 1 to `max_events` events
//     while ((num_events = io_getevents(aio_context.ctx, 1, max_events, events, &timeout)) < 0)
//         if (errno != EINTR)
//             throwFromErrno("io_getevents: Failed to wait for asynchronous IO completion", ErrorCodes::CANNOT_IO_GETEVENTS, errno);

//     /// Unpoison the memory returned from a non-instrumented system call.
//     __msan_unpoison(events, sizeof(*events) * num_events);

//     return num_events;
// }







// std::future<IOContextPool::BytesRead> IOContextPool::post(struct iocb & iocb)
// {
//     std::unique_lock lock{mutex};

//     /// get current id and increment it by one
//     const auto request_id = next_id;
//     ++next_id;

//     /// create a promise and put request in "queue"
//     promises.emplace(request_id, std::promise<BytesRead>{});
//     /// store id in IO request for further identification
//     iocb.aio_data = request_id;

//     struct iocb * requests[] { &iocb };

//     /// submit a request
//     while (io_submit(aio_context.ctx, 1, requests) < 0)
//     {
//         if (errno == EAGAIN)
//             /// wait until at least one event has been completed (or a spurious wakeup) and try again
//             have_resources.wait(lock);
//         else if (errno != EINTR)
//             throwFromErrno("io_submit: Failed to submit a request for asynchronous IO", ErrorCodes::CANNOT_IO_SUBMIT);
//     }

//     return promises[request_id].get_future();
// }



}
