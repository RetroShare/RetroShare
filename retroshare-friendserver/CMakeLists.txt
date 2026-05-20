# CMakeLists.txt for retroshare-friendserver
#
# SPDX-License-Identifier: AGPL-3.0-or-later

cmake_minimum_required (VERSION 3.18.0)
project(retroshare-friendserver)

include(CMakeDependentOption)

set(
	RS_BIN_INSTALL_DIR
	"${CMAKE_INSTALL_PREFIX}/bin"
	CACHE PATH
	"Path where to install retroshare-friendserver compiled binary" )

option(
	RS_DEVELOPMENT_BUILD
	"Disable optimization to speed up build, enable verbose build log"
	OFF )

option(
	RS_RNPLIB
	"Enable use RNP lib for PGP"
	ON )

find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)
find_package(Git REQUIRED)

if(RS_DEVELOPMENT_BUILD)
	set(CMAKE_VERBOSE_MAKEFILE ON)
	set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif(RS_DEVELOPMENT_BUILD)

# Sources
set(FRIENDSERVER_SOURCES
	src/retroshare-friendserver.cc
	src/friendserver.cc
	src/network.cc
)

add_executable(${PROJECT_NAME} ${FRIENDSERVER_SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-deprecated-declarations)


# Find libretroshare
if(NOT TARGET retroshare)
	set(LIBRETROSHARE_DEVEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../libretroshare/")
	if(EXISTS "${LIBRETROSHARE_DEVEL_DIR}/CMakeLists.txt" )
		message(STATUS "libretroshare source found at ${LIBRETROSHARE_DEVEL_DIR} using it" )
		add_subdirectory("${LIBRETROSHARE_DEVEL_DIR}" "${CMAKE_BINARY_DIR}/libretroshare" )
	else()
		include(FetchContent)
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
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE retroshare)

if(WIN32)
	target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 winmm iphlpapi mswsock)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION ${RS_BIN_INSTALL_DIR})
