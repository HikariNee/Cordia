cmake_minimum_required(VERSION 3.15...4.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
#set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


project(coderunner VERSION 0.1
                         DESCRIPTION "isolated coderunner using linux namespaces"
                         LANGUAGES CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libcap REQUIRED libcap IMPORTED_TARGET)

add_executable(App src/Main.cpp)
target_link_libraries(App PkgConfig::libcap)
