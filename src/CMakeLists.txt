cmake_minimum_required(VERSION 3.15)

# Collect only main.cpp for the original bs-rtype executable
# Exclude subdirectories (game/ and client/) which have their own targets
set(PROJECT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)

if (PROJECT_SOURCES STREQUAL "")
    message(FATAL_ERROR "No source files found in src/ (expected at least src/main.cpp)")
endif()

add_executable(bs-rtype ${PROJECT_SOURCES})

# Include public headers
target_include_directories(bs-rtype PRIVATE "${CMAKE_SOURCE_DIR}/include")

# Compiler warnings / flags
if (MSVC)
    target_compile_options(bs-rtype PRIVATE /W4 /permissive-)
else()
    target_compile_options(bs-rtype PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Link SFML by name (assuming SFML is on the linker path, as before)
target_link_libraries(bs-rtype PRIVATE sfml-graphics sfml-window sfml-system)

# -------------------------
# On Linux: after building, copy the produced executable to the repo root
# so there's a convenient copy at <repo-root>/bs-rtype
# This only runs on UNIX-like systems (not on Windows).
# -------------------------
if(UNIX AND NOT WIN32)
  add_custom_command(TARGET bs-rtype POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:bs-rtype>" "${CMAKE_SOURCE_DIR}/bs-rtype"
    # use the system chmod to set the executable bit (cmake -E has no chmod subcommand)
    COMMAND chmod +x "${CMAKE_SOURCE_DIR}/bs-rtype"
    COMMENT "Copying bs-rtype executable to repository root"
  )
endif()

# -------------------------
# dist target (windows only)
# -------------------------
if(WIN32)
  find_program(MSYS_BASH_EXECUTABLE NAMES bash.exe bash HINTS "C:/msys64/usr/bin" "C:/msys2/usr/bin" ENV PATH)

  set(MAKE_DIST_SCRIPT "${CMAKE_SOURCE_DIR}/tools/make_dist.sh")

  if(MSYS_BASH_EXECUTABLE AND EXISTS "${MAKE_DIST_SCRIPT}")
    message(STATUS "MSYS bash found: ${MSYS_BASH_EXECUTABLE}. dist will use tools/make_dist.sh")
    add_custom_target(dist
      DEPENDS bs-rtype
      COMMENT "Create ${CMAKE_BINARY_DIR}/dist with executable and required DLLs (via make_dist.sh)"
      COMMAND "${MSYS_BASH_EXECUTABLE}" -lc "\"${MAKE_DIST_SCRIPT}\" \"${CMAKE_BINARY_DIR}/src/bs-rtype.exe\""
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
  else()
    message(STATUS "tools/make_dist.sh not found or MSYS bash unavailable; dist target will not be created. To enable, ensure tools/make_dist.sh exists and MSYS bash is on PATH.")
  endif()
endif()