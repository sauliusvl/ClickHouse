#pragma once

#if defined(__linux__)

#include <IO/ReadBufferFromFileBase.h>
#include <IO/ReadBuffer.h>
#include <IO/BufferWithOwnMemory.h>
// #include <IO/AIO.h>
// #include <Core/Defines.h>
// #include <Common/CurrentMetrics.h>
// #include <string>
// #include <limits>
// #include <future>
// #include <unistd.h>
// #include <fcntl.h>
#include <liburing/compat.h>
#include <liburing.h>


// namespace CurrentMetrics
// {
//     extern const Metric OpenFileForRead;
// }

namespace DB
{

/** Class for asynchronous data reading using io-uring.
  */
class ReadBufferIOUring final : public ReadBufferFromFileBase
{
public:
    ReadBufferIOUring(const std::string & filename_, size_t buffer_size_ = DBMS_DEFAULT_BUFFER_SIZE, int flags_ = -1,
        char * existing_memory_ = nullptr);
    ~ReadBufferIOUring() override;

    ReadBufferIOUring(const ReadBufferIOUring &) = delete;
    ReadBufferIOUring & operator=(const ReadBufferIOUring &) = delete;

    off_t getPosition() override { return pos_in_file - (working_buffer.end() - pos); };
    std::string getFileName() const override { return filename; }
    int getFD() const { return fd; }

    off_t seek(off_t off, int whence) override;

private:

    struct io_uring ring;

    bool nextImpl() override;

    const std::string filename;

    /// What offset in file corresponds to working_buffer.end().
    off_t pos_in_file;

    /// The file descriptor for read.
    int fd = -1;
};

}

#endif
