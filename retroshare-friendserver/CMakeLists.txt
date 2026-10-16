## ------------------------------------------------------------------------- ##
 # CMakeLists.txt
 # This file is part of RetroShare-service.
 #
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

project(retroshare-friendserver
	VERSION 0.0.0
	LANGUAGES CXX
)

set(PROJECT_NAME_PRETTY "RetroShare-FriendServer")
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)


### options
set(RS_VERSION_SUFFIX
	""
	CACHE STRING
	"Custom version suffix; useful for downstream packaging."
)

option(
	RS_DEVELOPMENT_BUILD
	"Disable optimization to speed up build, enable verbose build log"
	OFF
)

option(
	RS_RNPLIB
	"Enable use RNP lib for PGP"
	ON
)


### dependencies
list(APPEND CMAKE_MODULE_PATH
	${PROJECT_SOURCE_DIR}/mk/cmake
	${PROJECT_SOURCE_DIR}/mk/cmake/fetch_provider_packages
)
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
find_package(RetroShare REQUIRED)


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
endif()
message(STATUS "${PROJECT_NAME_PRETTY} version: ${PROJECT_VERSION_FULL}")


### globals
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(RS_DEVELOPMENT_BUILD)
	set(CMAKE_VERBOSE_MAKEFILE ON)
	set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif(RS_DEVELOPMENT_BUILD)


### target
add_executable(${PROJECT_NAME}
	src/retroshare-friendserver.cc
	src/friendserver.h
	src/friendserver.cc
	src/network.h
	src/network.cc
)
target_link_libraries(${PROJECT_NAME}
	PRIVATE
	retroshare::retroshare
)
target_include_directories(${PROJECT_NAME}
	PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
)


### platforms
if(WIN32)
	target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()


## install
install(TARGETS ${PROJECT_NAME}
	COMPONENT retroshare-friendserver
)
