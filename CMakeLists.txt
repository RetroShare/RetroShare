## ------------------------------------------------------------------------- ##
 # CMakeLists.txt
 # This file is part of RetroShare-SuperProject.
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

# 3.7 : if(VERSION_GREATER_EQUAL)
cmake_minimum_required(VERSION 3.7...4.4)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
	list(PREPEND CMAKE_PROJECT_TOP_LEVEL_INCLUDES
		"${CMAKE_CURRENT_SOURCE_DIR}/mk/cmake/fetch_provider.cmake"
	)
else()
	include(mk/cmake/fetch_provider.cmake)
endif()

project(retroshare-superproject)

# Configuration Options
option(RS_GUI "Build RetroShare GUI" ON)
option(RS_SERVICE "Build RetroShare Service" ON)
option(RS_FRIENDSERVER "Build RetroShare Friendserver" ON)
option(RS_PLUGINS "Build RetroShare Plugins" OFF)
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

list(APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}/mk/cmake"
	"${PROJECT_SOURCE_DIR}/mk/cmake/fetch_provider_packages"
)
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)

# avoid using system libretroshare unless the user specifically asks for it
set(FETCH_PROVIDER_RetroShare_MODE FETCH_ONLY CACHE STRING
	"control how the Fetch dependency provider handles RetroShare"
)

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
	add_subdirectory(retroshare-webui)
endif()
