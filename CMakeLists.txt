cmake_minimum_required(VERSION 3.16)

project(Logger_osyncstream LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Warning flags for this project's own targets only. Linked PRIVATE, so they
# never reach code that uses the logger or third-party subprojects.
# if(MSVC) also covers clang-cl, where -Wall would mean -Weverything.
add_library(project_warnings INTERFACE)
if(MSVC)
    target_compile_options(project_warnings INTERFACE /W4)
else()
    target_compile_options(project_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

add_library(logger STATIC log.h log.cpp)
target_include_directories(logger PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(logger PRIVATE project_warnings)

add_executable(Logger_osyncstream main.cpp)
target_link_libraries(Logger_osyncstream PRIVATE logger project_warnings)

enable_testing()

add_executable(logger_stress_test tests/stress_test.cpp)
target_link_libraries(logger_stress_test PRIVATE logger project_warnings)
add_test(NAME stress COMMAND logger_stress_test)

add_executable(logger_error_macro_test tests/error_macro_test.cpp)
target_link_libraries(logger_error_macro_test PRIVATE logger project_warnings)
add_test(NAME error_macro COMMAND logger_error_macro_test)

include(GNUInstallDirs)
install(TARGETS Logger_osyncstream
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
