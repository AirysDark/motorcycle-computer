cmake_minimum_required(VERSION 3.16)
project(motorcycle_computer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(bike_protocol
    protocol/src/codec.cpp
    library/src/stream_parser.cpp
    library/src/node.cpp
    library/src/router.cpp
    library/src/supervisor.cpp
    library/src/lighting.cpp
    library/src/lighting_server.cpp
    library/src/lighting_controller.cpp
)

target_include_directories(bike_protocol PUBLIC
    protocol/include
    library/include
)

add_executable(protocol_smoke_test
    tests/protocol_smoke_test.cpp
)
target_link_libraries(protocol_smoke_test PRIVATE bike_protocol)

add_executable(network_runtime_test
    tests/network_runtime_test.cpp
)
target_link_libraries(network_runtime_test PRIVATE bike_protocol)

add_executable(supervisor_lighting_test
    tests/supervisor_lighting_test.cpp
)
target_link_libraries(supervisor_lighting_test PRIVATE bike_protocol)

add_executable(lighting_server_test
    tests/lighting_server_test.cpp
)
target_link_libraries(lighting_server_test PRIVATE bike_protocol)

enable_testing()
add_test(NAME protocol_smoke_test COMMAND protocol_smoke_test)
add_test(NAME network_runtime_test COMMAND network_runtime_test)
add_test(NAME supervisor_lighting_test COMMAND supervisor_lighting_test)
add_test(NAME lighting_server_test COMMAND lighting_server_test)
