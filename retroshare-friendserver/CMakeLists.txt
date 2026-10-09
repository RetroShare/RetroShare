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

## Offline build guard. libretroshare falls back to FetchContent (network
## download at configure time) when its local sources are absent. For
## offline/distribution builds (e.g. a Debian source package) that fallback must
## be a hard error: it would silently build against an unpinned upstream tip.
option(
	RS_FETCH_MISSING_DEPS
	"Allow downloading missing dependencies at configure time via FetchContent. Turn OFF for offline/distribution builds: a missing dependency then stops the configure with an explicit error instead of a silent network download." ON )

macro(rs_fetch_fallback_guard RS_FFG_DEP_NAME)
	if(RS_FETCH_MISSING_DEPS)
		message(STATUS "${RS_FFG_DEP_NAME} not found locally, downloading it via FetchContent (pass -DRS_FETCH_MISSING_DEPS=OFF to make this an error)")
	else()
		message(FATAL_ERROR "${RS_FFG_DEP_NAME} not found locally and RS_FETCH_MISSING_DEPS is OFF: refusing to download it at configure time. Provide its sources (git submodule / complete source tarball) or the system library." )
	endif()
endmacro()

# NOTE: ZLIB, BZip2 and OpenSSL are NOT direct dependencies of the friendserver.
# They are only used through libretroshare and are propagated transitively by the
# "retroshare" target's link interface, so they are intentionally not searched or
# linked here.
# Git is optional: it is only used to stamp versions (git describe, with a
# Source_Version file fallback for source tarballs) and by the FetchContent
# network fallbacks, which are guarded by RS_FETCH_MISSING_DEPS.
find_package(Git)

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


# Find libretroshare.
# The "retroshare" target may already exist when the friendserver is configured
# as part of the top-level RetroShare build (libretroshare added once via
# add_subdirectory). Guarding with NOT TARGET avoids adding the same subdirectory
# twice (which is a hard CMake error). When built standalone the target does not
# exist yet, so we add the local checkout or fetch libretroshare here.
if(NOT TARGET retroshare)
	set(LIBRETROSHARE_DEVEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../libretroshare/")
	if(EXISTS "${LIBRETROSHARE_DEVEL_DIR}/CMakeLists.txt" )
		message(STATUS "libretroshare source found at ${LIBRETROSHARE_DEVEL_DIR} using it" )
		add_subdirectory("${LIBRETROSHARE_DEVEL_DIR}" "${CMAKE_BINARY_DIR}/libretroshare" )
	else()
		include(FetchContent)
		rs_fetch_fallback_guard(libretroshare)
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

# Link libraries.
# OpenSSL must be linked here: friendserver.cc includes libretroshare public
# headers whose inline functions call the OpenSSL EVP API (e.g. EVP_DigestUpdate),
# so those symbols end up referenced in this target's objects. libretroshare links
# OpenSSL PRIVATE (and is built shared on Windows), so it does NOT propagate those
# symbols to consumers, hence the friendserver has to link OpenSSL itself.
find_package(OpenSSL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE retroshare OpenSSL::SSL OpenSSL::Crypto)

if(WIN32)
	# The friendserver uses the Winsock API directly (src/network.cc:
	# socket/bind/listen/accept/setsockopt/...), so it links ws2_32 explicitly
	# ("link what you use"). winmm and iphlpapi are already exported PUBLIC by
	# libretroshare, and mswsock is not used here, so none of those are linked.
	target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION ${RS_BIN_INSTALL_DIR})
