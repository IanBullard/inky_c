cmake_minimum_required(VERSION 3.14)
project(inky C)

set(COMMON_SOURCES src/inky_common.c)

if(BUILD_EMULATOR)
    set(BACKEND_SOURCES src/inky_emulator.c)
else()
    set(BACKEND_SOURCES src/inky_hardware.c)
endif()

add_library(inky STATIC ${COMMON_SOURCES} ${BACKEND_SOURCES})
target_include_directories(inky PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
