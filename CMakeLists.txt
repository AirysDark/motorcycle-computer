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
    library/src/northbridge.cpp
    library/src/supervisor.cpp
    library/src/lighting.cpp
    library/src/lighting_server.cpp
    library/src/lighting_controller.cpp
    library/src/security.cpp
    library/src/security_server.cpp
    library/src/security_controller.cpp
    library/src/motorcycle_computer.cpp
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

add_executable(northbridge_runtime_test
    tests/northbridge_runtime_test.cpp
)
target_link_libraries(northbridge_runtime_test PRIVATE bike_protocol)

add_executable(security_server_test
    tests/security_server_test.cpp
)
target_link_libraries(security_server_test PRIVATE bike_protocol)

add_executable(motorcycle_computer_state_test
    tests/motorcycle_computer_state_test.cpp
)
target_link_libraries(motorcycle_computer_state_test PRIVATE bike_protocol)

if(UNIX)
    add_executable(motorcycle_main
        raspberry-pi/main-computer/src/main.cpp
        raspberry-pi/main-computer/src/posix_serial_transport.cpp
    )
    target_include_directories(motorcycle_main PRIVATE
        raspberry-pi/main-computer/include
    )
    target_link_libraries(motorcycle_main PRIVATE bike_protocol)
endif()

enable_testing()
add_test(NAME protocol_smoke_test COMMAND protocol_smoke_test)
add_test(NAME network_runtime_test COMMAND network_runtime_test)
add_test(NAME supervisor_lighting_test COMMAND supervisor_lighting_test)
add_test(NAME lighting_server_test COMMAND lighting_server_test)
add_test(NAME northbridge_runtime_test COMMAND northbridge_runtime_test)
add_test(NAME security_server_test COMMAND security_server_test)
add_test(NAME motorcycle_computer_state_test COMMAND motorcycle_computer_state_test)
