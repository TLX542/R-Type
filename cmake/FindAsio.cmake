# FindAsio.cmake
# ---------------
# Find ASIO (standalone, header-only library)
#
# This module defines:
#  ASIO_FOUND        - True if ASIO headers are found
#  ASIO_INCLUDE_DIR  - Directory containing asio.hpp
#
# You can specify ASIO_ROOT to help find the installation.
#
# Example usage:
#   find_package(Asio)
#   if(ASIO_FOUND)
#     include_directories(${ASIO_INCLUDE_DIR})
#   endif()

# Try to find asio.hpp header
find_path(ASIO_INCLUDE_DIR
    NAMES asio.hpp
    PATHS
        ${ASIO_ROOT}
        ${CMAKE_PREFIX_PATH}
        /usr
        /usr/local
        $ENV{HOME}/.local
        C:/asio
        C:/Program\ Files/asio
    PATH_SUFFIXES
        include
        asio/include
    DOC "Path to ASIO header files"
)

# Handle the QUIETLY and REQUIRED arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Asio
    REQUIRED_VARS ASIO_INCLUDE_DIR
    FAIL_MESSAGE "Could not find ASIO standalone library. Try setting ASIO_ROOT or install with package manager."
)

# Create imported target if found
if(ASIO_FOUND AND NOT TARGET Asio::Asio)
    add_library(Asio::Asio INTERFACE IMPORTED)
    set_target_properties(Asio::Asio PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${ASIO_INCLUDE_DIR}"
        INTERFACE_COMPILE_DEFINITIONS "ASIO_STANDALONE"
    )
    
    # Add platform-specific link libraries
    if(WIN32)
        set_target_properties(Asio::Asio PROPERTIES
            INTERFACE_LINK_LIBRARIES "ws2_32;wsock32"
        )
    else()
        find_package(Threads REQUIRED)
        set_target_properties(Asio::Asio PROPERTIES
            INTERFACE_LINK_LIBRARIES "Threads::Threads"
        )
    endif()
endif()

# Mark variables as advanced
mark_as_advanced(ASIO_INCLUDE_DIR)
