# CMakeLists.txt for the RetroShare super-project (root aggregate build)
#
# SPDX-License-Identifier: AGPL-3.0-or-later

cmake_minimum_required(VERSION 3.5.0)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(retroshare-superproject)

# Default to Release build type if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()
# Force C++17 globally for all targets (including plugins)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
       # On macOS, allow undefined symbols in shared modules/libraries at link time
       # because they will be resolved at runtime by the host executable (retroshare-gui)
       set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup")
       set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -undefined dynamic_lookup")
endif()

# Global setting to allow executables to export symbols for plugin/module linking
set(CMAKE_ENABLE_EXPORTS ON)

# Configuration Options
option(RS_GUI "Build RetroShare GUI" ON)
option(RS_SERVICE "Build RetroShare Service" ON)
option(RS_FRIENDSERVER "Build RetroShare Friendserver" ON)
option(RS_PLUGINS "Build RetroShare Plugins" OFF)
option(RS_USE_I2P_SAM3 "Enable I2P SAMv3 support" ON)

# The "sam3" target is defined once here, at the top level, because it is linked
# by more than one subproject: libretroshare AND retroshare-gui (which uses libsam3
# directly in gui/settings/ServerPage.cpp). Defining it once avoids a duplicate
# target definition when the whole tree is configured together. If libretroshare is
# reworked to export sam3 as a target, this top-level definition could be dropped.
if(RS_USE_I2P_SAM3)
	add_definitions(-DRS_USE_I2P_SAM3 -DRS_USE_I2P_SAM3_LIBSAM3)

	set(SAM3_DEVEL_DIR "${CMAKE_SOURCE_DIR}/supportlibs/libsam3")
	if(EXISTS "${SAM3_DEVEL_DIR}")
		message(STATUS "libsam3 found at ${SAM3_DEVEL_DIR}, defining target sam3 globally")
		add_library(sam3 STATIC
			"${SAM3_DEVEL_DIR}/src/libsam3/libsam3.c"
			"${SAM3_DEVEL_DIR}/src/libsam3a/libsam3a.c"
		)
		target_include_directories(sam3 PUBLIC 
			"${SAM3_DEVEL_DIR}/src/libsam3"
			"${SAM3_DEVEL_DIR}/src/libsam3a"
		)
		if(WIN32)
			target_link_libraries(sam3 PUBLIC ws2_32)
		endif()
	endif()
endif()

# NOTE: Windows-specific workarounds that used to live here as global
# add_definitions()/add_compile_options() have been moved to the specific targets
# that need them (see the per-target block after the add_subdirectory() calls
# below), so they no longer leak into every target in the tree.

# 1. First add the core library (libretroshare)
add_subdirectory(libretroshare)

# 2. Then add the dependent executables
if(RS_GUI)
	add_subdirectory(retroshare-gui)
	if(RS_PLUGINS)
		add_subdirectory(plugins)
	endif()
endif()

if(RS_SERVICE)
	add_subdirectory(retroshare-service)
endif()

if(RS_FRIENDSERVER)
	add_subdirectory(retroshare-friendserver)
endif()

# ---------------------------------------------------------------------------
# Per-target Windows fixups (applied to the specific targets that need them,
# rather than globally, per review feedback). These run after the subdirectories
# so the targets already exist.
# ---------------------------------------------------------------------------
if(WIN32)
	# libbitdht needs the wide-char Windows API (e.g. MoveFileExW). libretroshare
	# already sets UNICODE/_UNICODE PUBLIC for itself and its consumers, but
	# libbitdht is a dependency (not a consumer) so it needs its own definition.
	if(TARGET bitdht)
		target_compile_definitions(bitdht PRIVATE UNICODE _UNICODE)
	endif()

	# Legacy C submodules trip strict GCC >= 15 diagnostics that are now errors by
	# default; downgrade them to warnings only for those targets. Add other legacy
	# targets here if a future toolchain flags them.
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		foreach(legacy_target sam3 bitdht miniupnpc udp-discovery)
			if(TARGET ${legacy_target})
				target_compile_options(${legacy_target} PRIVATE
					-Wno-error=incompatible-pointer-types
					-Wno-error=int-conversion
					-Wno-error=implicit-function-declaration )
			endif()
		endforeach()
	endif()
endif()
