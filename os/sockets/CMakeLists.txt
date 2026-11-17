cmake_minimum_required(VERSION 3.16)
project(udp_multicast CXX)

# Same standard the rclcpp packages build with.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)  # -g -O0: step through it in gdb
endif()

add_compile_options(-Wall -Wextra -Wpedantic)

add_library(udp_socket STATIC udp_socket.cpp)
target_include_directories(udp_socket PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(sender sender.cpp)
target_link_libraries(sender PRIVATE udp_socket)

add_executable(receiver receiver.cpp)
target_link_libraries(receiver PRIVATE udp_socket)

add_executable(announcer announce.cpp)
target_link_libraries(announcer PRIVATE udp_socket)
