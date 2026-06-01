# CMakeLists.txt for RetroShare Monorepo Build
#
# SPDX-License-Identifier: AGPL-3.0-or-later

cmake_minimum_required(VERSION 3.5.0)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(retroshare-monorepo)

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
	endif()
endif()

# Global configurations for Windows builds
if(WIN32)
	# Force Unicode mappings globally for all Windows targets (fixes MoveFileEx in libbitdht)
	add_definitions(-DUNICODE -D_UNICODE)

	# Downgrade strict GCC 15 errors to warnings for old submodules
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		add_compile_options(
			-Wno-error=incompatible-pointer-types
			-Wno-error=int-conversion
			-Wno-error=implicit-function-declaration
		)
	endif()
endif()

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
