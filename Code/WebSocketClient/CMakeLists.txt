cmake_minimum_required(VERSION 3.21)

project(UDM15WebSocketClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.12 REQUIRED COMPONENTS Core Widgets WebSockets)

add_executable(udm15_websocket_client
    main.cpp
    WebSocketClient.cpp
    WebSocketClient.h
    WebSocketClientWindow.cpp
    WebSocketClientWindow.h
    ../WebSocketServer/MessageProtocol.h
)

target_include_directories(udm15_websocket_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../WebSocketServer
)
target_link_libraries(udm15_websocket_client PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::WebSockets
)
