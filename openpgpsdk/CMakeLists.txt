# RetroShare decentralized communication platform
#
# Copyright (C) 2021  Gioacchino Mazzurco <gio@eigenlab.org>
# Copyright (C) 2021  Asociación Civil Altermundi <info@altermundi.net>
#
# SPDX-License-Identifier: CC0-1.0

cmake_minimum_required (VERSION 3.12.0)
project(openpgpsdk)

find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)
find_package(OpenSSL REQUIRED)

file(GLOB SOURCES src/openpgpsdk/*.c)
add_library(${PROJECT_NAME} ${SOURCES})

target_include_directories(
	${PROJECT_NAME}
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
	PRIVATE ${OPENSSL_INCLUDE_DIR}
	PRIVATE ${BZIP2_INCLUDE_DIRS}
	PRIVATE ${ZLIB_INCLUDE_DIRS} )

target_link_libraries(
	${PROJECT_NAME}
	OpenSSL::SSL OpenSSL::Crypto BZip2::BZip2 ZLIB::ZLIB )
