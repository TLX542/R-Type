# FindRaylib.cmake
# ----------------
# Find raylib library (https://www.raylib.com/)
#
# This module defines:
#  RAYLIB_FOUND          - True if raylib is found
#  RAYLIB_INCLUDE_DIR    - Directory containing raylib.h
#  RAYLIB_LIBRARY        - Path to raylib library
#  RAYLIB_LIBRARIES      - Libraries to link against
#
# You can specify RAYLIB_ROOT to help find the installation.
#
# Example usage:
#   find_package(Raylib)
#   if(RAYLIB_FOUND)
#     target_link_libraries(mytarget raylib)
#   endif()

# Try to find raylib.h header
find_path(RAYLIB_INCLUDE_DIR
    NAMES raylib.h
    PATHS
        ${RAYLIB_ROOT}
        ${CMAKE_PREFIX_PATH}
        /usr
        /usr/local
        $ENV{HOME}/.local
        C:/raylib
        C:/Program\ Files/raylib
    PATH_SUFFIXES
        include
        raylib/include
    DOC "Path to raylib header files"
)

# Try to find raylib library
find_library(RAYLIB_LIBRARY
    NAMES raylib libraylib
    PATHS
        ${RAYLIB_ROOT}
        ${CMAKE_PREFIX_PATH}
        /usr
        /usr/local
        $ENV{HOME}/.local
    PATH_SUFFIXES
        lib
        lib64
        raylib/lib
    DOC "Path to raylib library"
)

# Set RAYLIB_LIBRARIES
if(RAYLIB_LIBRARY)
    set(RAYLIB_LIBRARIES ${RAYLIB_LIBRARY})
    
    # Add platform-specific dependencies
    if(WIN32)
        list(APPEND RAYLIB_LIBRARIES winmm gdi32 user32 shell32)
    elseif(UNIX AND NOT APPLE)
        list(APPEND RAYLIB_LIBRARIES m pthread dl)
        # Check for X11 (usually required on Linux)
        find_package(X11)
        if(X11_FOUND)
            list(APPEND RAYLIB_LIBRARIES ${X11_LIBRARIES})
        endif()
    endif()
endif()

# Handle the QUIETLY and REQUIRED arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Raylib
    REQUIRED_VARS RAYLIB_LIBRARY RAYLIB_INCLUDE_DIR
    FAIL_MESSAGE "Could not find raylib library. Try setting RAYLIB_ROOT or install with package manager."
)

# Create imported target if found
if(RAYLIB_FOUND AND NOT TARGET raylib)
    add_library(raylib UNKNOWN IMPORTED)
    set_target_properties(raylib PROPERTIES
        IMPORTED_LOCATION "${RAYLIB_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${RAYLIB_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${RAYLIB_LIBRARIES}"
    )
endif()

# Mark variables as advanced
mark_as_advanced(RAYLIB_INCLUDE_DIR RAYLIB_LIBRARY RAYLIB_LIBRARIES)
