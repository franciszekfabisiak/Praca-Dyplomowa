# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(initiator)

file(GLOB app_sources src/*.c src/bluetooth/*.c src/logic_manager/*.c)

target_sources(app PRIVATE ${app_sources})

zephyr_library_include_directories(
        ${CMAKE_CURRENT_LIST_DIR}/include
)
