# RetroShare decentralized communication platform
#
# Copyright (C) 2021  Gioacchino Mazzurco <gio@eigenlab.org>
# Copyright (C) 2021  Asociación Civil Altermundi <info@altermundi.net>
#
# SPDX-License-Identifier: CC0-1.0

cmake_minimum_required (VERSION 2.8.12)
project(bitdht)

file(
	GLOB BITDHT_SOURCES
	src/bitdht/*.c src/bitdht/*.cc src/udp/*.cc src/util/*.cc )

add_library(${PROJECT_NAME} ${BITDHT_SOURCES})

target_include_directories(
	${PROJECT_NAME}
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src )
