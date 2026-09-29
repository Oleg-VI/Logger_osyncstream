cmake_minimum_required(VERSION 3.16)

project(Logger_osyncstream LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Logger_osyncstream main.cpp
    log.h log.cpp)

include(GNUInstallDirs)
install(TARGETS Logger_osyncstream
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
