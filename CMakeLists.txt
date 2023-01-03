cmake_minimum_required(VERSION 3.22)

project(ucx_demo)
find_package(UCX REQUIRED COMPONENTS ucp uct ucs)

set(CMAKE_CXX_STANDARD 23)
add_executable(ucx_demo
        src/main.cpp
        src/base.h src/base.cpp
        src/messaging.h src/messaging.cpp
)

target_link_libraries(ucx_demo ucx::ucp ucx::ucs)
