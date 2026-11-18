cmake_minimum_required(VERSION 3.16)
project(threads CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Writes build/compile_commands.json: the exact compiler flags for every file.
# clangd (code completion, go-to-definition) reads it.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)  # no optimisation: keeps the race in race.cpp visible
endif()

add_compile_options(-Wall -Wextra -Wpedantic)

# std::thread needs the platform thread library. Threads::Threads is the portable name for it.
find_package(Threads REQUIRED)

# Reuse your UdpSocket from exercise 01: same source file, no copy.
set(EX01 ${CMAKE_CURRENT_SOURCE_DIR}/../sockets)
add_library(udp_socket STATIC ${EX01}/udp_socket.cpp)
target_include_directories(udp_socket PUBLIC ${EX01})

add_executable(subscriber subscriber.cpp)
target_link_libraries(subscriber PRIVATE udp_socket Threads::Threads)

add_executable(publisher publisher.cpp)
target_link_libraries(publisher PRIVATE udp_socket)

add_executable(race race.cpp)
target_link_libraries(race PRIVATE Threads::Threads)

add_executable(deadlock deadlock.cpp)
target_link_libraries(deadlock PRIVATE Threads::Threads)

# Optional: race.cpp and your subscriber, built with ThreadSanitizer.
# Needs: sudo dnf install libtsan     then: cmake -B build -DWITH_TSAN=ON
option(WITH_TSAN "Also build race_tsan and subscriber_tsan with ThreadSanitizer" OFF)
if(WITH_TSAN)
  add_executable(race_tsan race.cpp)
  target_compile_options(race_tsan PRIVATE -fsanitize=thread)
  target_link_options(race_tsan PRIVATE -fsanitize=thread)
  target_link_libraries(race_tsan PRIVATE Threads::Threads)

  # udp_socket.cpp is compiled in directly so it is instrumented too.
  add_executable(subscriber_tsan subscriber.cpp ${EX01}/udp_socket.cpp)
  target_include_directories(subscriber_tsan PRIVATE ${EX01})
  target_compile_options(subscriber_tsan PRIVATE -fsanitize=thread)
  target_link_options(subscriber_tsan PRIVATE -fsanitize=thread)
  target_link_libraries(subscriber_tsan PRIVATE Threads::Threads)
endif()
