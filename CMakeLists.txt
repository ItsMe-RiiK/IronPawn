cmake_minimum_required(VERSION 3.10)
project(IronPawn)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

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

# Executable
add_executable(IronPawn ${SOURCES})

# Link libraries
target_link_libraries(IronPawn PRIVATE ${CURL_LIBRARIES} nlohmann_json::nlohmann_json)
