# RetroShare decentralized communication platform
#
# Copyright (C) 2021-2022 Gioacchino Mazzurco <gio@retroshare.cc>
# Copyright (C) 2026      David Bears <dbear4q@gmail.com>
#
# SPDX-License-Identifier: CC0-1.0

cmake_minimum_required(VERSION 3.0...4.4)

project(retroshare-service)


include(CMakeDependentOption)

option(
	RS_DEVELOPMENT_BUILD
	"Disable optimization to speed up build, enable verbose build log. \
	 just for development purposes, not suitable for library usage"
	OFF )

option(
	RS_RNPLIB
	"Enable use RNP lib for PGP"
	ON )

option(
	RS_JSON_API
	"Use restbed to expose libretroshare as JSON API via HTTP"
	ON )

option(
	RS_SERVICE_DESKTOP
	"Install icons and shortcuts for desktop environements"
	OFF )

option(
	RS_SERVICE_TERMINAL_LOGIN
	"Enable RetroShare login via terminal"
	ON )

cmake_dependent_option(
	RS_SERVICE_TERMINAL_WEBUI_PASSWORD
	"Enable settin Web UI password via terminal in retroshare-service"
	OFF
	"RS_WEBUI"
	OFF )

cmake_dependent_option(
	RS_WEBUI
	"Install RetroShare Web UI"
	OFF
	"RS_JSON_API"
	OFF )

################################################################################
### dependencies

# list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/mk/cmake")
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)

find_package(RetroShare REQUIRED)
find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)
find_package(Git REQUIRED)

################################################################################
### global options

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(RS_DEVELOPMENT_BUILD)
	set(CMAKE_VERBOSE_MAKEFILE ON)
	set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif(RS_DEVELOPMENT_BUILD)

################################################################################
### target

add_executable(${PROJECT_NAME}
	src/retroshare-service.cc
	src/util/retroshare-service_path.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE retroshare::retroshare)

################################################################################
### target options

if(RS_JSON_API)
	target_compile_definitions(${PROJECT_NAME} PUBLIC RS_JSONAPI)
endif(RS_JSON_API)

if(RS_SERVICE_TERMINAL_LOGIN)
	target_compile_definitions(${PROJECT_NAME} PUBLIC RS_SERVICE_TERMINAL_LOGIN)
endif(RS_SERVICE_TERMINAL_LOGIN)

if(RS_SERVICE_TERMINAL_WEBUI_PASSWORD)
	target_compile_definitions(${PROJECT_NAME}
		PUBLIC RS_SERVICE_TERMINAL_WEBUI_PASSWORD
	)
endif(RS_SERVICE_TERMINAL_WEBUI_PASSWORD)

if(RS_DEVELOPMENT_BUILD)
	target_compile_options(${PROJECT_NAME} PRIVATE "-O0")
endif(RS_DEVELOPMENT_BUILD)

################################################################################
### platforms

if(WIN32)
	target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 winmm iphlpapi mswsock)
endif()

if(APPLE)
	target_include_directories(${PROJECT_NAME} PRIVATE "/usr/local/include")
	target_link_directories(${PROJECT_NAME} PRIVATE "/usr/local/lib")
endif()

################################################################################
### install

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})

if(UNIX AND NOT APPLE AND RS_SERVICE_DESKTOP)
	install(
		FILES data/retroshare-service.svg
		DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/
	)

	foreach(size 48x48 128x128)
		install(
			FILES data/retroshare-service_${size}.png
			DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/${size}/apps/
			RENAME retroshare-service.png
		)
	endforeach()

	install(
		FILES data/retroshare-service.desktop
		DESTINATION ${CMAKE_INSTALL_PREFIX}/data/
	)
endif()
