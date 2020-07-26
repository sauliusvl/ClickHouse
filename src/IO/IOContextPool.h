#pragma once

#include <condition_variable>
#include <future>
#include <mutex>
#include <map>
#include <IO/AIO.h>
#include <Common/ThreadPool.h>
#include <Common/Exception.h>
#include <common/logger_useful.h>
#include <Common/MemorySanitizer.h>
#include <Poco/Logger.h>
#include <boost/range/iterator_range.hpp>
#include <errno.h>


namespace DB
{

template<typename Request, typename Event>
class IOContextPool : private boost::noncopyable
{
    static const auto max_concurrent_events = 128;
    static const auto timeout_sec = 1;

    //AIOContext aio_context{max_concurrent_events};

    using ID = size_t;
    using BytesRead = ssize_t;

    /// Autoincremental id used to identify completed requests
    ID next_id{};
    mutable std::mutex mutex;
    mutable std::condition_variable have_resources;
    std::map<ID, std::promise<BytesRead>> promises;

    std::atomic<bool> cancelled{false};
    ThreadFromGlobalPool io_completion_monitor{&IOContextPool<Request, Event>::doMonitor, this};

    ~IOContextPool()
    {
        cancelled.store(true, std::memory_order_relaxed);
        io_completion_monitor.join();
    }

    void doMonitor()
    {
        /// continue checking for events unless cancelled
        while (!cancelled.load(std::memory_order_relaxed))
            waitForCompletion();

        /// wait until all requests have been completed
        while (!promises.empty())
            waitForCompletion();
    }
    
    void waitForCompletion()
    {
        /// array to hold completion events
        Event events[max_concurrent_events];

        try
        {
            const auto num_events = getCompletionEvents(events, max_concurrent_events);
            fulfillPromises(events, num_events);
            notifyProducers(num_events);
        }
        catch (...)
        {
            /// there was an error, log it, return to any producer and continue
            reportExceptionToAnyProducer();
            tryLogCurrentException("IOContextPool::waitForCompletion()");
        }
    }
    
    int getCompletionEvents(Event events[], const int max_events) const;

    ID getEventID(Event event) const;

    BytesRead getEventResult(Event event) const;

    void fulfillPromises(const Event events[], const int num_events)
    {
        if (num_events == 0)
            return;

        const std::lock_guard lock{mutex};

        /// look at returned events and find corresponding promise, set result and erase promise from map
        for (const auto & event : boost::make_iterator_range(events, events + num_events))
        {
            const auto completed_id = getEventID(event);

            /// set value via promise and release it
            const auto it = promises.find(completed_id);
            if (it == std::end(promises))
            {
                LOG_ERROR(&Poco::Logger::get("IOcontextPool"), "Found IO event with unknown id {}", completed_id);
                continue;
            }

            it->second.set_value(getEventResult(event));
            promises.erase(it);
        }
    }

    void notifyProducers(const int num_producers) const
    {
        if (num_producers == 0)
            return;

        if (num_producers > 1)
            have_resources.notify_all();
        else
            have_resources.notify_one();
    }

    
    void reportExceptionToAnyProducer()
    {
        const std::lock_guard lock{mutex};

        const auto any_promise_it = std::begin(promises);
        any_promise_it->second.set_exception(std::current_exception());
    }


public:
    IOContextPool<Request, Event> & instance()
    {
        static IOContextPool<Request, Event> instance;
        return instance;
    }

    void submitRequest(Request &request, ID id);

    /// Request IO read operation for iocb, returns a future with number of bytes read
    std::future<IOContextPool::BytesRead> IOContextPool::post(Request &request)
    {
        std::unique_lock lock{mutex};

        /// get current id and increment it by one
        const auto request_id = next_id;
        ++next_id;

        /// create a promise and put request in "queue"
        promises.emplace(request_id, std::promise<BytesRead>{});

        submitRequest(request, request_id);
        /// store id in IO request for further identification
        /*iocb.aio_data = request_id;

        struct iocb * requests[] { &iocb };

        /// submit a request
        while (io_submit(aio_context.ctx, 1, requests) < 0)
        {
            if (errno == EAGAIN)
                /// wait until at least one event has been completed (or a spurious wakeup) and try again
                have_resources.wait(lock);
            else if (errno != EINTR)
                throwFromErrno("io_submit: Failed to submit a request for asynchronous IO", ErrorCodes::CANNOT_IO_SUBMIT);
        }*/

        return promises[request_id].get_future();
    }
};

}
