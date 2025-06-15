cmake_minimum_required(VERSION 3.15...4.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(ROOTFS_PATH "/var/lib/cordia/rootfs")

project(Cordia VERSION 0.1
                         DESCRIPTION "use linux namespaces to run arbitrary code."
                         LANGUAGES CXX)

set(LIBS_CPP
        "src/error_handling.cpp"
        "src/utilities.cpp"
        "src/container.cpp"
        "src/mount_rootfs.cpp"
        "src/mq.cpp"
)

set(LIBS_HPP
        "src/error_handling.cpp"
        "src/utilities.hpp"
        "src/container.hpp"
        "src/mount_rootfs.hpp"
        "src/mq.hpp"
)

configure_file (config.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/src/config.hpp @ONLY)
add_library(libs ${LIBS_CPP} ${LIBS_HPP})
add_executable(App src/main.cpp)
target_link_libraries(App PRIVATE libs)
