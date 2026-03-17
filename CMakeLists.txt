cmake_minimum_required(VERSION 3.19)
project(uart_emulator LANGUAGES CXX)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets SerialPort)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(uart_emulator
    WIN32 MACOSX_BUNDLE
    main.cpp
    emulateddevice.cpp
    emulateddevice.h
    mainwindow.cpp
    mainwindow.h
)

target_link_libraries(uart_emulator
    PRIVATE
        Qt5::Core
        Qt5::Widgets
        Qt5::SerialPort
)

include(GNUInstallDirs)

install(TARGETS uart_emulator
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
