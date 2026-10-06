# CMakeLists.txt for the RetroShare super-project (root aggregate build)
#
# Copyright (C) 2026       David Bears <dbear4q@gmail.com>
#
# SPDX-License-Identifier: AGPL-3.0-or-later

cmake_minimum_required(VERSION 3.24...4.4)

list(PREPEND CMAKE_PROJECT_TOP_LEVEL_INCLUDES
	"${CMAKE_CURRENT_SOURCE_DIR}/mk/cmake/fetch_provider.cmake"
)

project(retroshare-superproject)

# Configuration Options
option(RS_GUI "Build RetroShare GUI" ON)
option(RS_SERVICE "Build RetroShare Service" ON)
option(RS_FRIENDSERVER "Build RetroShare Friendserver" ON)
option(RS_PLUGINS "Build RetroShare Plugins" OFF)
option(RS_USE_I2P_SAM3 "Enable I2P SAMv3 support" ON)
set(RS_PLUGIN_BINARY_DIR ${PROJECT_BINARY_DIR}/plugins CACHE PATH
	"directory to place plugin shared objects at build time"
	" (does not affect install location)"
)


# Default to Release build type if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE "Release" CACHE
		STRING "Choose the type of build (Debug or Release)" FORCE
	)
endif()


### some misc global properties

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# export symbols for plugin/module linking
set(CMAKE_ENABLE_EXPORTS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(APPLE)
	# On macOS, allow undefined symbols in shared modules/libraries at link time
	# because they will be resolved at runtime by the host executable (retroshare-gui)
	set(CMAKE_SHARED_LINKER_FLAGS
		"${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup"
	)
	set(CMAKE_MODULE_LINKER_FLAGS
		"${CMAKE_MODULE_LINKER_FLAGS} -undefined dynamic_lookup"
	)
endif()


################################################################################
### dependencies

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/mk/cmake")
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
include(mk/cmake/fetch_provider_declare.cmake)

if(RS_GXSWIKIPOS)
	add_subdirectory(supportlibs/pegmarkdown)
endif()

################################################################################
### add subdirectories

if(RS_GUI)
	add_subdirectory(retroshare-gui)

	# I guess it's not possible to use plugins without the GUI?
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

if(RS_WEBUI)
	## TODO: integrate retroshare-webui with find_package
	add_subdirectory(retroshare-webui)
endif()
