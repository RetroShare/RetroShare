## ------------------------------------------------------------------------- ##
 # CMakeLists.txt
 # This file is part of RetroShare-service.
 #
 # Copyright (C) 2026      David Bears <dbear4q@gmail.com>
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
# SPDX-License-Identifier: AGPL-3.0-or-later

cmake_minimum_required(VERSION 3.0...4.4)

project(retroshare-friendserver)


### options
option(
	RS_DEVELOPMENT_BUILD
	"Disable optimization to speed up build, enable verbose build log"
	OFF )

option(
	RS_RNPLIB
	"Enable use RNP lib for PGP"
	ON )


### dependencies
# list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/mk/cmake")
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
find_package(RetroShare REQUIRED)


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
install(TARGETS ${PROJECT_NAME})
