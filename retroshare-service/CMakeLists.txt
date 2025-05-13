# RetroShare decentralized communication platform
#
# Copyright (C) 2021-2022  Gioacchino Mazzurco <gio@retroshare.cc>
# Copyright (C) 2021-2022  Asociación Civil Altermundi <info@altermundi.net>
#
# SPDX-License-Identifier: CC0-1.0

cmake_minimum_required (VERSION 3.18.0)
project(retroshare-service)

include(CMakeDependentOption)

if(APPLE)
  include_directories("/usr/local/include") 
  LINK_DIRECTORIES("/usr/local/lib")
endif()

set(
	RS_BIN_INSTALL_DIR
	"${CMAKE_INSTALL_PREFIX}/bin"
	CACHE PATH
	"Path where to install retroshare-service compiled binary" )

option(
	RS_DEVELOPMENT_BUILD
	"Disable optimization to speed up build, enable verbose build log. \
	 just for development purposes, not suitable for library usage"
	OFF )

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
	ON )

cmake_dependent_option(
	RS_WEBUI
	"Install RetroShare Web UI"
	OFF
	"RS_JSON_API"
	ON )

################################################################################

set(FETCHCONTENT_QUIET OFF)
include(FetchContent)

find_package(Git REQUIRED)

################################################################################

if(RS_DEVELOPMENT_BUILD)
	set(CMAKE_VERBOSE_MAKEFILE ON)
	set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif(RS_DEVELOPMENT_BUILD)

################################################################################

list(
	APPEND RS_SERVICE_SOURCES
	src/retroshare-service.cc )

add_executable(${PROJECT_NAME} ${RS_SERVICE_SOURCES})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

install(TARGETS ${PROJECT_NAME} DESTINATION ${RS_BIN_INSTALL_DIR})

################################################################################

if(RS_DEVELOPMENT_BUILD)
	target_compile_options(${PROJECT_NAME} PRIVATE "-O0")
endif(RS_DEVELOPMENT_BUILD)

################################################################################

set(LIBRETROSHARE_DEVEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../libretroshare/")
if(EXISTS "${LIBRETROSHARE_DEVEL_DIR}/CMakeLists.txt" )
	message(
		STATUS
		"libretroshare source found at ${LIBRETROSHARE_DEVEL_DIR} using it" )
	add_subdirectory(
		"${LIBRETROSHARE_DEVEL_DIR}" "${CMAKE_BINARY_DIR}/libretroshare" )
else()
	FetchContent_Declare(
		libretroshare
		GIT_REPOSITORY "https://github.com/RetroShare/libretroshare.git"
		GIT_TAG "origin/master"
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
		TIMEOUT 10
	)
	FetchContent_MakeAvailable(libretroshare)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE retroshare)

################################################################################

# Add RNP build directories to linker search path
target_link_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/../supportlibs/librnp/Build/src/lib
    ${PROJECT_SOURCE_DIR}/../supportlibs/librnp/Build/src/libsexpp
)

# Link libraries by name, including RNP libs and dependencies first
target_link_libraries(${PROJECT_NAME} PRIVATE
    rnp             # Link by name
    sexpp
    botan-3
    json-c
    bz2
    z
    retroshare
)

if(RS_SERVICE_DESKTOP)
	if(UNIX AND NOT APPLE)
		install(
			FILES data/retroshare-service.svg
			DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/ )

		install(
			FILES data/retroshare-service_48x48.png
			DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps/
			RENAME retroshare-service.png)

		install(
			FILES data/retroshare-service_128x128.png
			DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/128x128/apps/
			RENAME retroshare-service.png )

		install(
			FILES data/retroshare-service.desktop
			DESTINATION ${CMAKE_INSTALL_PREFIX}/data/ )
	endif(UNIX AND NOT APPLE)
endif(RS_SERVICE_DESKTOP)

################################################################################

if(RS_JSON_API)
	target_compile_definitions(${PROJECT_NAME} PUBLIC RS_JSONAPI)
endif(RS_JSON_API)

if(RS_SERVICE_TERMINAL_LOGIN)
	target_compile_definitions(${PROJECT_NAME} PUBLIC RS_SERVICE_TERMINAL_LOGIN)
endif(RS_SERVICE_TERMINAL_LOGIN)

if(RS_WEBUI)
	set(RS_WEBUI_DEVEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../webui/")
	if(EXISTS "${RS_WEBUI_DEVEL_DIR}/CMakeLists.txt" )
		message(
			STATUS
			"RetroShare WebUI source found at ${RS_WEBUI_DEVEL_DIR} using it" )
		add_subdirectory(
			"${RS_WEBUI_DEVEL_DIR}" "${CMAKE_BINARY_DIR}/webui" )
	else()
		FetchContent_Declare(
			webui
			GIT_REPOSITORY "https://github.com/RetroShare/RSNewWebUI.git"
			GIT_TAG "origin/master"
			GIT_SHALLOW TRUE
			GIT_PROGRESS TRUE
			TIMEOUT 10
		)
		FetchContent_MakeAvailable(webui)
	endif()

	add_dependencies(${PROJECT_NAME} retroshare-webui)
	target_compile_definitions(${PROJECT_NAME} PUBLIC RS_WEBUI)
endif(RS_WEBUI)

if(RS_SERVICE_TERMINAL_WEBUI_PASSWORD)
	target_compile_definitions(
		${PROJECT_NAME} PUBLIC RS_SERVICE_TERMINAL_WEBUI_PASSWORD )
endif(RS_SERVICE_TERMINAL_WEBUI_PASSWORD)

################################################################################
