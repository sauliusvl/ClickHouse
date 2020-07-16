set (LIBURING_SOURCE_DIR ${ClickHouse_SOURCE_DIR}/contrib/liburing/src)
set (LIBURING_BINARY_DIR ${ClickHouse_BINARY_DIR}/contrib/liburing/src)

set (LIBURING_SOURCES
    ${LIBURING_SOURCE_DIR}/queue.c
    ${LIBURING_SOURCE_DIR}/register.c
    ${LIBURING_SOURCE_DIR}/setup.c
    ${LIBURING_SOURCE_DIR}/syscall.c
)

# The below checks should run against the bundled contrib/libc-headers
set (CMAKE_REQUIRED_INCLUDES ${CMAKE_C_STANDARD_INCLUDE_DIRECTORIES})

check_c_source_runs ("
#include <linux/fs.h>
int main(int argc, char **argv)
{
  __kernel_rwf_t x;
  x = 0;
  return x;
}" LIBURING_CONFIG_HAS_KERNEL_RWF_T)

check_c_source_runs ("
#include <linux/time.h>
#include <linux/time_types.h>
int main(int argc, char **argv)
{
  struct __kernel_timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 1;
  return 0;
}" LIBURING_CONFIG_HAS_KERNEL_TIMESPEC)

check_c_source_runs ("
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
int main(int argc, char **argv)
{
  struct open_how how;
  how.flags = 0;
  how.mode = 0;
  how.resolve = 0;
  return 0;
}" LIBURING_CONFIG_HAS_OPEN_HOW)

unset (CMAKE_REQUIRED_INCLUDES)

# Since we're using custom libc headers from contrib, we have to generate compat.h
# and override it even if we're using the system liburing library

configure_file (compat.h.in ${LIBURING_COMPAT_HEADER})

if (USE_INTERNAL_LIBURING_LIBRARY)
  add_library (liburing ${LIBURING_SOURCES})
  target_include_directories (liburing PUBLIC ${LIBURING_COMPAT_INCLUDE_DIR} "${LIBURING_SOURCE_DIR}/include")
endif()
