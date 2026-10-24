## ------------------------------------------------------------------------- ##
 # CMakeLists.txt
 # This file is part of RetroShare-GUI.
 #
 # Copyright (C) 2021-2022 Gioacchino Mazzurco <gio@retroshare.cc>
 # Copyright (C) 2026      David Bears <dbear4q@gmail.com>
 #
 # SPDX-License-Identifier: AGPL-3.0-or-later
 #
 # This program is free software: you can redistribute it and/or modify
 # it under the terms of the GNU Affero General Public License as
 # published by the Free Software Foundation, either version 3 of the
 # License, or (at your option) any later version.
 #
 # This program is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 # GNU Affero General Public License for more details.
 #
 # You should have received a copy of the GNU Affero General Public License
 # along with this program.  If not, see <https://www.gnu.org/licenses/>.
## ------------------------------------------------------------------------- ##

# 3.10: string(PREPEND)
cmake_minimum_required(VERSION 3.10...4.4)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
	list(PREPEND CMAKE_PROJECT_TOP_LEVEL_INCLUDES
		"${CMAKE_CURRENT_SOURCE_DIR}/mk/cmake/fetch_provider.cmake"
	)
else()
	include(mk/cmake/fetch_provider.cmake)
endif()

project(retroshare-service
	VERSION 0.0.0
	LANGUAGES CXX
)

set(PROJECT_NAME_PRETTY "RetroShare-Service")
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)


################################################################################
### options

include(CMakeDependentOption)

set(RS_VERSION_SUFFIX
	""
	CACHE STRING
	"Custom version suffix; useful for downstream packaging."
)

option(
	RS_RNPLIB
	"Enable use RNP lib for PGP"
	ON
)

option(
	RS_JSON_API
	"Use restbed to expose libretroshare as JSON API via HTTP"
	ON
)

option(
	RS_SERVICE_DESKTOP
	"Install icons and shortcuts for desktop environements"
	OFF
)

option(
	RS_SERVICE_TERMINAL_LOGIN
	"Enable RetroShare login via terminal"
	ON
)

cmake_dependent_option(
	RS_SERVICE_TERMINAL_WEBUI_PASSWORD
	"Enable settin Web UI password via terminal in retroshare-service"
	OFF
	"RS_WEBUI"
	OFF
)

cmake_dependent_option(
	RS_WEBUI
	"Install RetroShare Web UI"
	OFF
	"RS_JSON_API"
	OFF
)

################################################################################
### dependencies

list(APPEND CMAKE_MODULE_PATH
	${PROJECT_SOURCE_DIR}/mk/cmake
	${PROJECT_SOURCE_DIR}/mk/cmake/fetch_provider_packages
)
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)

find_package(RetroShare REQUIRED)


################################################################################
### build configurations

get_property(BUILD_TYPES CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS)
list(APPEND BUILD_TYPES
	Release MinSizeRel RelWithDebInfo Debug BetaTest DebugO2 None
)
list(REMOVE_DUPLICATES BUILD_TYPES)
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${BUILD_TYPES})
string(TOUPPER "${BUILD_TYPES}" BUILD_TYPES_UPPER)
string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_UPPER)
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo)
	message(STATUS "No build type specified; defaulting to ${CMAKE_BUILD_TYPE}.")
elseif(NOT BUILD_TYPE_UPPER IN_LIST BUILD_TYPES_UPPER)
	message(FATAL_ERROR "Unrecognized build type: ${CMAKE_BUILD_TYPE}.")
endif()
get_property(LANGUAGES GLOBAL PROPERTY ENABLED_LANGUAGES)
foreach(lang ${LANGUAGES})
	string(APPEND CMAKE_${lang}_FLAGS_RELEASE        " -O3 -w -DNDEBUG")
	string(APPEND CMAKE_${lang}_FLAGS_MINSIZEREL     " -Os -w -DNDEBUG")
	string(APPEND CMAKE_${lang}_FLAGS_RELWITHDEBINFO " -O3 -g -DNDEBUG")
	string(APPEND CMAKE_${lang}_FLAGS_DEBUG          " -O0 -g -UNDEBUG")
	string(APPEND CMAKE_${lang}_FLAGS_BETATEST       " -O3 -g -UNDEBUG -w")
	string(APPEND CMAKE_${lang}_FLAGS_DEBUGO2        " -O2 -g -UNDEBUG")
	string(APPEND CMAKE_${lang}_FLAGS_NONE           "")
endforeach()
set(DEV_BUILD_TYPES RELWITHDEBINFO DEBUG DEBUGO2)
set(VERSION_RELEASE "")
set(VERSION_MINSIZEREL "")
set(VERSION_RELWITHDEBINFO "debinfo")
set(VERSION_BETATEST "beta")
set(VERSION_DEBUG "debug")
set(VERSION_DEBUGOPT "debug")
set(VERSION_NONE "")


################################################################################
### version

if(NOT PROJECT_VERSION)
	message(SEND_ERROR "${PROJECT_NAME_PRETTY} version seems to be missing!")
endif()
if(DEFINED RS_VERSION_SUFFIX AND NOT (RS_VERSION_SUFFIX STREQUAL ""))
	set(PROJECT_VERSION_FULL "${PROJECT_VERSION}-${RS_VERSION_SUFFIX}")
else()
	include(GetGitRevisionDescription)
	git_describe_working_tree(PROJECT_VERSION_FULL --dirty --match "v*.*.*")
	if(PROJECT_VERSION_FULL AND EXISTS ${PROJECT_SOURCE_DIR}/.git)
		string(SUBSTRING ${PROJECT_VERSION_FULL} 1 -1 PROJECT_VERSION_FULL)
		string(FIND ${PROJECT_VERSION_FULL}- ${PROJECT_VERSION}- pos)
		if(NOT pos EQUAL 0)
			message(WARNING "${PROJECT_NAME_PRETTY} version mismatch!"
				" Declared version is ${PROJECT_VERSION},"
				" but git describe returned ${PROJECT_VERSION_FULL}."
			)
			string(PREPEND PROJECT_VERSION *)
			string(PREPEND PROJECT_VERSION_FULL *)
		endif()
	else()
		set(PROJECT_VERSION_FULL "${PROJECT_VERSION}-unknown")
	endif()
	if(VERSION_${BUILD_TYPE_UPPER})
		string(APPEND PROJECT_VERSION_FULL "-${VERSION_${BUILD_TYPE_UPPER}}")
	endif()
endif()
message(STATUS "${PROJECT_NAME_PRETTY} version: ${PROJECT_VERSION_FULL}")

################################################################################
### global options

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

install(TARGETS ${PROJECT_NAME}
	DESTINATION ${CMAKE_INSTALL_BINDIR}
	COMPONENT retroshare-service
)

if(UNIX AND NOT APPLE AND RS_SERVICE_DESKTOP)
	install(
		FILES data/retroshare-service.svg
		DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/
		COMPONENT retroshare-service
	)

	foreach(size 48x48 128x128)
		install(
			FILES data/retroshare-service_${size}.png
			DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/${size}/apps/
			RENAME retroshare-service.png
			COMPONENT retroshare-service
		)
	endforeach()

	install(
		FILES data/retroshare-service.desktop
		DESTINATION ${CMAKE_INSTALL_PREFIX}/data/
		COMPONENT retroshare-service
	)
endif()
