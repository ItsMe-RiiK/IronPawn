cmake_minimum_required(VERSION 3.10)
project(IronPawn)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find libcurl
find_package(CURL REQUIRED)

# Include directories
include_directories(src)
include_directories(${CURL_INCLUDE_DIRS})

# Add nlohmann_json if installed system-wide
# It's header-only, so we just need to ensure the compiler finds it
# Arch Linux usually installs it in /usr/include/nlohmann/json.hpp
find_package(nlohmann_json REQUIRED)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

include(FetchContent)

# Find Discord C++ Library (DPP)
find_package(dpp QUIET)
if (dpp_FOUND)
    message(STATUS "Found installed DPP")
    set(DPP_TARGET dpp::dpp)
else()
    message(STATUS "DPP not found, fetching via CMake FetchContent... This will take a few minutes!")
    FetchContent_Declare(
      dpp
      GIT_REPOSITORY https://github.com/brainboxdotcc/DPP.git
      GIT_TAG        v10.0.35
    )
    FetchContent_MakeAvailable(dpp)
    set(DPP_TARGET dpp)
endif()

# Find httplib (for Local Web Server)
find_package(httplib QUIET)
if (httplib_FOUND)
    message(STATUS "Found installed cpp-httplib")
else()
    message(STATUS "Fetching cpp-httplib via CMake FetchContent...")
    FetchContent_Declare(
        httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
        GIT_TAG        v0.15.3
    )
    FetchContent_MakeAvailable(httplib)
endif()

# Executable
add_executable(IronPawn ${SOURCES})

# Link libraries
target_link_libraries(IronPawn PRIVATE ${CURL_LIBRARIES} nlohmann_json::nlohmann_json httplib::httplib ${DPP_TARGET})
