cmake_minimum_required(VERSION 3.21)

project(UDM15WebSocketConnectionManager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.12 REQUIRED COMPONENTS Core WebSockets)

add_executable(udm15_websocket_server
    main.cpp
    WebSocketServer.cpp
    WebSocketServer.h
    MessageProtocol.h
)
target_link_libraries(udm15_websocket_server PRIVATE Qt6::Core Qt6::WebSockets)

add_executable(udm15_websocket_client
    WebSocketClient.cpp
    MessageProtocol.h
)
target_link_libraries(udm15_websocket_client PRIVATE Qt6::Core Qt6::WebSockets)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.12 REQUIRED COMPONENTS Test)

    add_executable(udm15_websocket_server_tests
        WebSocketServerTests.cpp
        WebSocketServer.cpp
        WebSocketServer.h
        MessageProtocol.h
    )
    target_link_libraries(udm15_websocket_server_tests PRIVATE
        Qt6::Core Qt6::WebSockets Qt6::Test
    )
    add_test(NAME websocket_server_lifecycle COMMAND udm15_websocket_server_tests)
endif()
