#if defined(__linux__)

#include <IO/ReadBufferIOUring.h>

namespace DB
{

namespace ErrorCodes
{
    extern const int FILE_DOESNT_EXIST;
    extern const int CANNOT_OPEN_FILE;
    extern const int CANNOT_READ_FROM_FILE_DESCRIPTOR;
    extern const int ARGUMENT_OUT_OF_BOUND;
    extern const int CANNOT_SEEK_THROUGH_FILE;
}

ReadBufferIOUring::ReadBufferIOUring(const std::string & filename_, size_t buffer_size_, int flags_, char * existing_memory_)
    : ReadBufferFromFileBase(buffer_size_, existing_memory_, 0),
      filename(filename_)
{
    int open_flags = (flags_ == -1) ? O_RDONLY : flags_;
    open_flags |= O_CLOEXEC;

    fd = ::open(filename.c_str(), open_flags);
    if (fd == -1)
    {
        auto error_code = (errno == ENOENT) ? ErrorCodes::FILE_DOESNT_EXIST : ErrorCodes::CANNOT_OPEN_FILE;
        throwFromErrnoWithPath("Cannot open file " + filename, filename, error_code);
    }

    io_uring_queue_init(1, &ring, 0);
}

ReadBufferIOUring::~ReadBufferIOUring()
{
    if (fd != -1)
        ::close(fd);
    
    io_uring_queue_exit(&ring);
}

bool ReadBufferIOUring::nextImpl()
{
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);

    io_uring_prep_read(sqe, fd, internal_buffer.begin(), internal_buffer.size(), pos_in_file);
    io_uring_submit(&ring);

    struct io_uring_cqe *cqe;
retry:
    int ret = io_uring_wait_cqe(&ring, &cqe);
    if (ret == -4) {
        printf("retrying!\n");
        goto retry;
    }

    if (ret < 0) {
        perror("io_uring_wait_cqe");
        printf("%d\n", ret);
        return 1;
    }
    if (cqe->res < 0) {
        fprintf(stderr, "Async readv failed.\n");
        return 1;
    }

    size_t bytes_read = cqe->res;

    io_uring_cqe_seen(&ring, cqe);

    pos_in_file += bytes_read;

    if (bytes_read)
    {
        //ProfileEvents::increment(ProfileEvents::ReadBufferFromFileDescriptorReadBytes, bytes_read);
        working_buffer.resize(bytes_read);
    }
    else
        return false;

    return true;
}

/// If 'offset' is small enough to stay in buffer after seek, then true seek in file does not happen.
off_t ReadBufferIOUring::seek(off_t offset, int whence)
{
    off_t new_pos;
    if (whence == SEEK_SET)
        new_pos = offset;
    else if (whence == SEEK_CUR)
        new_pos = pos_in_file - (working_buffer.end() - pos) + offset;
    else
        throw Exception("ReadBufferFromFileDescriptor::seek expects SEEK_SET or SEEK_CUR as whence", ErrorCodes::ARGUMENT_OUT_OF_BOUND);

    /// Position is unchanged.
    if (new_pos + (working_buffer.end() - pos) == pos_in_file)
        return new_pos;

    if (hasPendingData() && new_pos <= pos_in_file && new_pos >= pos_in_file - static_cast<off_t>(working_buffer.size()))
    {
        /// Position is still inside buffer.
        pos = working_buffer.begin() + (new_pos - (pos_in_file - working_buffer.size()));
        return new_pos;
    }
    else
    {
        //ProfileEvents::increment(ProfileEvents::Seek);
        //Stopwatch watch(profile_callback ? clock_type : CLOCK_MONOTONIC);

        pos = working_buffer.end();
        off_t res = ::lseek(fd, new_pos, SEEK_SET);
        if (-1 == res)
            throwFromErrnoWithPath("Cannot seek through file " + getFileName(), getFileName(),
                                   ErrorCodes::CANNOT_SEEK_THROUGH_FILE);
        pos_in_file = new_pos;

        //watch.stop();
        //ProfileEvents::increment(ProfileEvents::DiskReadElapsedMicroseconds, watch.elapsedMicroseconds());

        return res;
    }
}

}

#endif
