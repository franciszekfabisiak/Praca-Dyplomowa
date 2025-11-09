# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(initiator)

target_sources(app PRIVATE src/main.c src/distance_estimation.c src/bluetooth/bluetooth_global.c src/bluetooth/bluetooth_reflector.c src/bluetooth/bluetooth_initiator.c)

zephyr_library_include_directories(
        ${CMAKE_CURRENT_LIST_DIR}/include ${CMAKE_CURRENT_LIST_DIR}/src/bluetooth
)
