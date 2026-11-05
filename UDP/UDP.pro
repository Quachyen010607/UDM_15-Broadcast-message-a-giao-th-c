QT += core network widgets
CONFIG += c++17
TEMPLATE = app
TARGET = udp_demo
SOURCES += demo/main.cpp \
           src/UdpService.cpp
HEADERS += src/UdpService.h
INCLUDEPATH += src
