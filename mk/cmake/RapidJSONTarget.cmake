## ---------------------------------------------------------------------- ##
 # mk/cmake/RapidJSONTarget.cmake
 # This file is part of RetroShare.
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

# This module defines the `RapidJSON` target, which was not defined by older
# versions of RapidJSON up to and including 1.1.0.

cmake_minimum_required(VERSION 3.24...4.4)

if(NOT TARGET RapidJSON AND RAPIDJSON_INCLUDE_DIRS)
  add_library(RapidJSON INTERFACE IMPORTED)
  set_target_properties(RapidJSON PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${RAPIDJSON_INCLUDE_DIRS}"
  )
endif()
