# ------------------------------------------------------------------------ *\
# supportlibs/CMakeLists.txt
# This file is part of RetroShare.
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
# ------------------------------------------------------------------------ */



include(FetchContent)
include(ExternalProject)

################################################################################
### bitdht

set(BITDHT_DEVEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libbitdht/")
if(EXISTS "${BITDHT_DEVEL_DIR}/CMakeLists.txt" )
	message(STATUS "bitdht source found at ${BITDHT_DEVEL_DIR}")
	FetchContent_Declare(
		bitdht
		SOURCE_DIR ${BITDHT_DEVEL_DIR}
		OVERRIDE_FIND_PACKAGE
	)
else()
	FetchContent_Declare(
		bitdht
		GIT_REPOSITORY "https://github.com/RetroShare/BitDHT.git"
		GIT_TAG "origin/master"
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
		TIMEOUT 10
		EXCLUDE_FROM_ALL
		OVERRIDE_FIND_PACKAGE
	)
endif()

################################################################################
### cpptrace

FetchContent_Declare(cpptrace
	GIT_REPOSITORY https://github.com/jeremy-rifkin/cpptrace.git
	GIT_TAG v0.3.1
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
	TIMEOUT 10
	OVERRIDE_FIND_PACKAGE
)

################################################################################
### jni.hpp

set(JNI_HPP_DEVEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/jni.hpp")
if(EXISTS "${JNI_HPP_DEVEL_DIR}/CMakeLists.txt")
	FetchContent_Declare(jni.hpp
		SOURCE_DIR "${JNI_HPP_DEVEL_DIR}"
		OVERRIDE_FIND_PACKAGE
	)
else()
	FetchContent_Declare(jni.hpp
		GIT_REPOSITORY "https://github.com/RetroShare/jni.hpp.git"
		GIT_TAG "origin/master"
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
		TIMEOUT 10
		OVERRIDE_FIND_PACKAGE
	)
endif()
if(
	NOT EXISTS ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/jni.hpp-extra.cmake
	AND
	NOT EXISTS ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/jni.hppExtra.cmake
)
	configure_file(
		"${PROJECT_SOURCE_DIR}/mk/cmake/jni.hpp-extra.cmake.in"
		"${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/jni.hpp-extra.cmake"
		@ONLY
	)
endif()

################################################################################
### librnp

set(RNPLIB_DEVEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/librnp/")
if(EXISTS "${RNPLIB_DEVEL_DIR}/CMakeLists.txt")
	message(STATUS "librnp source found at ${RNPLIB_DEVEL_DIR}")
	FetchContent_Declare(
		rnp
		SOURCE_DIR ${RNPLIB_DEVEL_DIR}
		OVERRIDE_FIND_PACKAGE
	)
else()
	FetchContent_Declare(
		rnp
		GIT_REPOSITORY "https://github.com/rnpgp/rnp.git"
		GIT_TAG "origin/main"
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
		TIMEOUT 10
		EXCLUDE_FROM_ALL
		OVERRIDE_FIND_PACKAGE
	)
endif()
if(
	NOT EXISTS ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/rnp-extra.cmake AND
	NOT EXISTS ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/rnpExtra.cmake
)
	configure_file(
		"${PROJECT_SOURCE_DIR}/mk/cmake/rnp-extra.cmake.in"
		"${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/rnp-extra.cmake"
		@ONLY
	)
endif()

################################################################################
### libsam3

set(SAM3_DEVEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libsam3")
if(EXISTS "${SAM3_DEVEL_DIR}")
	message(STATUS "libsam3 found at ${SAM3_DEVEL_DIR}")
	set(SAM3_DOWNLOAD_OPTIONS
		SOURCE_DIR "${SAM3_DEVEL_DIR}"
	)
  set(libsam3_external_SOURCE_DIR ${SAM3_DEVEL_DIR})
else()
	set(SAM3_DOWNLOAD_OPTIONS
		GIT_REPOSITORY "https://github.com/i2p/libsam3.git"
		GIT_TAG "origin/master"
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
		TIMEOUT 10
	)
  set(libsam3_external_SOURCE_DIR "libsam3_external-prefix/src/libsam3_external")
endif()
message(STATUS "libsam3 source directory: ${libsam3_external_SOURCE_DIR}")
set(libsam3_external_BINARY_DIR ${libsam3_external_SOURCE_DIR})
list(APPEND libsam3_external_BUILD_ENVIRONMENT
  "CC=set:${CMAKE_C_COMPILER}"
)
if(WIN32)
  list(APPEND libsam3_external_BUILD_ENVIRONMENT
    "LDFLAGS=set:-lmingw32 -lws2_32 -lwsock32 -mwindows"
  )
endif()
ExternalProject_Add(
	libsam3_external
	${SAM3_DOWNLOAD_OPTIONS}
	CONFIGURE_COMMAND ""
  BUILD_COMMAND make build
  INSTALL_COMMAND ""
  BUILD_IN_SOURCE TRUE
	EXCLUDE_FROM_ALL YES
  BUILD_BYPRODUCTS "${libsam3_external_BINARY_DIR}/libsam3.a"
  BUILD_ENVIRONMENT_MODIFICATION ${libsam3_external_BUILD_ENVIRONMENT}
)
configure_file(
  "${PROJECT_SOURCE_DIR}/mk/cmake/libsam3-config.cmake.in"
  "${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/libsam3-config.cmake"
  @ONLY
)

################################################################################
### openpgpsdk

set(OPENPGPSDK_DEVEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/openpgpsdk/")
if(EXISTS "${OPENPGPSDK_DEVEL_DIR}/CMakeLists.txt" )
	message(STATUS "openpgpsdk source found at ${OPENPGPSDK_DEVEL_DIR}")
	FetchContent_Declare(
		openpgpsdk
		SOURCE_DIR ${OPENPGPSDK_DEVEL_DIR}
		OVERRIDE_FIND_PACKAGE
	)
else()
	FetchContent_Declare(
		openpgpsdk
		GIT_REPOSITORY "https://github.com/RetroShare/OpenPGP-SDK.git"
		GIT_TAG "origin/master"
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
		TIMEOUT 10
		EXCLUDE_FROM_ALL
		OVERRIDE_FIND_PACKAGE
	)
endif()
if(
	NOT EXISTS ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/openpgpsdk-extra.cmake AND
	NOT EXISTS ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/openpgpsdkExtra.cmake
)
	configure_file(
		"${PROJECT_SOURCE_DIR}/mk/cmake/openpgpsdk-extra.cmake.in"
		"${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/openpgpsdk-extra.cmake"
		@ONLY
	)
endif()

################################################################################
### rapidjson

set(RAPIDJSON_DEVEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/rapidjson")
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "Build rapidjson examples.")
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL
	"Build rapidjson perftests and unittests."
)
if(EXISTS "${RAPIDJSON_DEVEL_DIR}/CMakeLists.txt")
	message(STATUS "RapidJSON found at ${RAPIDJSON_DEVEL_DIR}")
	FetchContent_Declare(
		RapidJSON
		SOURCE_DIR ${RAPIDJSON_DEVEL_DIR}
		OVERRIDE_FIND_PACKAGE
	)
else()
	FetchContent_Declare(
		RapidJSON
		GIT_REPOSITORY "https://github.com/Tencent/rapidjson.git"
		GIT_TAG "origin/master"
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
		TIMEOUT 10
		EXCLUDE_FROM_ALL
		OVERRIDE_FIND_PACKAGE
	)
endif()
if(
	NOT EXISTS ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/rapidjson-extra.cmake AND
	NOT EXISTS ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/RapidJSONExtra.cmake
)
	configure_file(
		"${PROJECT_SOURCE_DIR}/mk/cmake/rapidjson-extra.cmake.in"
		"${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/rapidjson-extra.cmake"
		@ONLY
	)
endif()

################################################################################
### restbed

set(RESTBED_DEVEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/restbed")
set(BUILD_TESTS OFF CACHE BOOL "build restbed tests")
set(BUILD_SSL OFF CACHE BOOL "enable SSL support in restbed")
if(NOT WIN32 AND EXISTS "${RESTBED_DEVEL_DIR}/CMakeLists.txt")
	list(APPEND RESTBED_DOWNLOAD_OPTIONS SOURCE_DIR "${RESTBED_DEVEL_DIR}")
else()
	list(APPEND RESTBED_DOWNLOAD_OPTIONS
		GIT_REPOSITORY "https://github.com/Corvusoft/restbed.git"
		GIT_TAG "4.8"
		GIT_SUBMODULES dependency/asio dependency/catch
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
		TIMEOUT 10
		# 1. Remove /wd4251 (MSVC-specific flag) that breaks GCC/Clang builds.
		# 2. Bump CMake minimum version to 3.5.0 to suppress deprecation warnings in CMake >= 3.27.
		PATCH_COMMAND sed -i -e "s|/wd4251||g" -e "s|VERSION 3.1.0|VERSION 3.5.0|g" CMakeLists.txt
	)
endif()
if(
	NOT EXISTS ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/restbed-extra.cmake AND
	NOT EXISTS ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/restbedExtra.cmake
)
	configure_file(
		"${PROJECT_SOURCE_DIR}/mk/cmake/restbed-extra.cmake.in"
		"${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/restbed-extra.cmake"
		@ONLY
	)
endif()
FetchContent_Declare(restbed
	${RESTBED_DOWNLOAD_OPTIONS}
	OVERRIDE_FIND_PACKAGE
)

################################################################################
### udp-discovery

set(UDP_DISCOVERY_DEVEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/udp-discovery-cpp")
## TODO: upstream option to disable tests building
set(BUILD_EXAMPLE OFF CACHE BOOL "build udp-discovery-cpp examples")
set(BUILD_TOOL OFF CACHE BOOL "build udp-discovery-tool application")
if(EXISTS "${UDP_DISCOVERY_DEVEL_DIR}/CMakeLists.txt")
	message(STATUS "UDP discovery source found at ${UDP_DISCOVERY_DEVEL_DIR}")
	set(UDP-DISCOVERY-CPP_DOWNLOAD_OPTIONS
		SOURCE_DIR "${UDP_DISCOVERY_DEVEL_DIR}"
	)
else()
	set(UDP-DISCOVERY-CPP_DOWNLOAD_OPTIONS
		GIT_REPOSITORY "https://github.com/truvorskameikin/udp-discovery-cpp.git"
		GIT_TAG "origin/master"
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
		TIMEOUT 10
	)
endif()
FetchContent_Declare(udp-discovery-cpp
	${UDP-DISCOVERY-CPP_DOWNLOAD_OPTIONS}
	OVERRIDE_FIND_PACKAGE
	PATCH_COMMAND sed -i -e "s/^\\(cmake_minimum_required(VERSION 2.8\\))\$/\\1...4.3.4)/" CMakeLists.txt
)
if(
	NOT EXISTS ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/udp-discovery-cpp-extra.cmake
	AND
	NOT EXISTS ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/udp-discovery-cppExtra.cmake
)
	configure_file(
		"${PROJECT_SOURCE_DIR}/mk/cmake/udp-discovery-cpp-extra.cmake.in"
		"${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/udp-discovery-cpp-extra.cmake"
		@ONLY
	)
endif()
