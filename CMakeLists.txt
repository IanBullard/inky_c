cmake_minimum_required(VERSION 3.21)
project(inky VERSION 1.1 LANGUAGES C)

include(GNUInstallDirs)

option(BUILD_EMULATOR "Build the emulator backend instead of Raspberry Pi hardware" OFF)
option(INKY_BUILD_TESTS "Build test programs (emulator only)" ${PROJECT_IS_TOP_LEVEL})

set(COMMON_SOURCES src/inky_common.c)

if(BUILD_EMULATOR)
    set(BACKEND_SOURCES src/inky_emulator.c)
else()
    set(BACKEND_SOURCES src/inky_hardware.c)
endif()

add_library(inky STATIC ${COMMON_SOURCES} ${BACKEND_SOURCES})
target_include_directories(inky PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(inky PROPERTIES
    C_STANDARD 99
    C_EXTENSIONS ON
    PUBLIC_HEADER include/inky.h
)
target_compile_definitions(inky PRIVATE _GNU_SOURCE)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(inky PRIVATE -Wall -Wextra)
endif()

install(TARGETS inky
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(INKY_BUILD_TESTS AND BUILD_EMULATOR)
    enable_testing()

    add_executable(test_models test/test_models.c)
    target_link_libraries(test_models PRIVATE inky)
    set_target_properties(test_models PROPERTIES C_STANDARD 99 C_EXTENSIONS ON)

    # Writes test_*.ppm into the build directory
    add_test(NAME test_models COMMAND test_models WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif()
