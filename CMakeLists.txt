cmake_minimum_required(VERSION 3.16)
project(motorcycle_computer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(bike_protocol
    protocol/src/codec.cpp
)

target_include_directories(bike_protocol PUBLIC
    protocol/include
    library/include
)

add_executable(protocol_smoke_test
    tests/protocol_smoke_test.cpp
)

target_link_libraries(protocol_smoke_test PRIVATE bike_protocol)
