cmake_minimum_required(VERSION 3.4)

project(test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -Wall -Wextra -O3")

add_subdirectory(third_party)

add_executable(main
    src/main.cc
)

target_include_directories(main
    PRIVATE src
    PRIVATE third_party
)

target_link_directories(main
    PRIVATE third_party/libdeflate_win32
)

target_link_libraries(main
    libdeflate
)
