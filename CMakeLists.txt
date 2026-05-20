# CMakeLists.txt for RetroShare Monorepo Build
#
# SPDX-License-Identifier: AGPL-3.0-or-later

cmake_minimum_required(VERSION 3.5.0)
project(retroshare-monorepo)

# Configuration Options
option(RS_GUI "Build RetroShare GUI" ON)
option(RS_SERVICE "Build RetroShare Service" ON)
option(RS_FRIENDSERVER "Build RetroShare Friendserver" ON)
option(RS_PLUGINS "Build RetroShare Plugins" OFF)

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
