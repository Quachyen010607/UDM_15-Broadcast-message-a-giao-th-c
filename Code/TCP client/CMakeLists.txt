cmake_minimum_required(VERSION 3.16)
project(UDM15_TcpServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

add_executable(TcpServer
    main.cpp
    MainWindow.cpp
    MainWindow.h
    TcpServer.cpp
    TcpServer.h
    Message.h
)

target_link_libraries(TcpServer PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

if(WIN32)
    set_target_properties(TcpServer PROPERTIES WIN32_EXECUTABLE ON)
endif()

# ---- Test tu dong: nhieu client ket noi, ngat, truyen message ----
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test QUIET)
if(Qt${QT_VERSION_MAJOR}Test_FOUND)
    enable_testing()
    add_executable(tst_multiclient
        tests/tst_multiclient.cpp
        TcpServer.cpp
        TcpServer.h
        Message.h
    )
    target_link_libraries(tst_multiclient PRIVATE
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Test
    )
    add_test(NAME multiclient COMMAND tst_multiclient)
endif()
