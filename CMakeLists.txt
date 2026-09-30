cmake_minimum_required(VERSION 3.16)

project(Logger_osyncstream LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /EHsc)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_library(logger STATIC log.h log.cpp)
target_include_directories(logger PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(Logger_osyncstream main.cpp)
target_link_libraries(Logger_osyncstream PRIVATE logger)

enable_testing()

add_executable(logger_stress_test tests/stress_test.cpp)
target_link_libraries(logger_stress_test PRIVATE logger)
add_test(NAME stress COMMAND logger_stress_test)

include(GNUInstallDirs)
install(TARGETS Logger_osyncstream
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
