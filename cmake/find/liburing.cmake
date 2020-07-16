if (OS_LINUX)
    option(ENABLE_LIBURING "Enable liburing" ${ENABLE_LIBRARIES})
else ()
    option(ENABLE_LIBURING "Enable liburing" 0)
endif ()

if (ENABLE_LIBURING)

execute_process(COMMAND uname -r OUTPUT_VARIABLE UNAME_RESULT OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REGEX MATCH "[0-9]+.[0-9]+" LINUX_KERNEL_VERSION ${UNAME_RESULT})

if (LINUX_KERNEL_VERSION LESS 5.1) # io-uring appeared in 5.1
    message (STATUS "Building without liburing, kernel version must be at least 5.1")
else ()
    option (USE_INTERNAL_LIBURING_LIBRARY "Set to FALSE to use system liburing library instead of bundled" ${NOT_UNBUNDLED})

    if(NOT EXISTS "${ClickHouse_SOURCE_DIR}/contrib/liburing/src/include/liburing.h")
        if(USE_INTERNAL_LIBURING_LIBRARY)
            message(WARNING "submodule contrib/liburing is missing. to fix try run: \n git submodule update --init --recursive")
        endif()
        set(USE_INTERNAL_LIBURING_LIBRARY 0)
        set(MISSING_INTERNAL_LIBURING_LIBRARY 1)
    endif()

    if (NOT USE_INTERNAL_LIBURING_LIBRARY)
        find_library (LIBURING_LIBRARY NAMES uring)
        find_path (LIBURING_INCLUDE_DIR NAMES liburing.h PATHS ${LIBURING_INCLUDE_PATHS})
    endif ()

    if (LIBURING_LIBRARY AND LIBURING_INCLUDE_DIR)
        set(USE_LIBURING 1)
    elseif (NOT MISSING_INTERNAL_LIBURING_LIBRARY)
        set(USE_LIBURING 1)
        set (USE_INTERNAL_LIBURING_LIBRARY 1)
        set (LIBURING_LIBRARY liburing)
        set (LIBURING_INCLUDE_DIR ${ClickHouse_SOURCE_DIR}/contrib/liburing/src/include)
    endif ()

    if (USE_LIBURING)
        set (LIBURING_COMPAT_INCLUDE_DIR ${ClickHouse_BINARY_DIR}/contrib/liburing/src/include-compat)
        set (LIBURING_COMPAT_HEADER ${LIBURING_COMPAT_INCLUDE_DIR}/liburing/compat.h)
    endif ()

    message (STATUS "Using liburing=${USE_LIBURING}: ${LIBURING_INCLUDE_DIR} : ${LIBURING_LIBRARY}")
endif()

endif()
