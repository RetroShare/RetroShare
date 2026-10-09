## ---------------------------------------------------------------------- ##
 # mk/cmake/FindX11.cmake
 # This file is part of RetroShare-GUI.
 #
 # Copyright (C) 2026      David Bears <dbear4q@gmail.com>
 #
 # This program is free software; you can redistribute it and/or modify
 # it under the terms of the GNU General Public License as published by
 # the Free Software Foundation; either version 2 of the License, or
 # (at your option) any later version.
 #
 # This program is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 # GNU General Public License for more details.
 #
 # You should have received a copy of the GNU General Public License along
 # with this program; if not, write to the Free Software Foundation, Inc.,
 # 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
## ---------------------------------------------------------------------- ##

# 3.7 : pkg_check_modules(IMPORTED_TARGET)
# 3.11: add_library(ALIAS <IMPORTED GLOBAL>)
# 3.13: pkg_check_modules(IMPORTED_TARGET GLOBAL)
cmake_minimum_required(VERSION 3.13...4.4)

find_package(PkgConfig)
if(PkgConfig_FOUND)
  pkg_check_modules(X11 IMPORTED_TARGET GLOBAL x11)
endif()

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
	set(HANDLE_VERSION_RANGE HANDLE_VERSION_RANGE)
else()
  set(HANDLE_VERSION_RANGE)
endif()
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(X11
  REQUIRED_VARS X11_FOUND
  VERSION_VAR X11_VERSION
  ${HANDLE_VERSION_RANGE}
)

if(X11_FOUND AND NOT TARGET X11::X11)
  add_library(X11::X11 ALIAS PkgConfig::X11)
endif()
