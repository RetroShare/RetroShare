# ------------------------------------------------------------------------ *\
# mk/cmake/Findsam3.cmake
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

cmake_minimum_required(VERSION 3.0...4.4)

find_library(SAM3_LIBRARY NAMES libsam3.a)
find_path(SAM3_INCLUDE_SYNCHRONOUS NAMES libsam3.h PATH_SUFFIXES src/libsam3)
find_path(SAM3_INCLUDE_ASYNCHRONOUS NAMES libsam3a.h PATH_SUFFIXES src/libsam3a)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(${CMAKE_FIND_PACKAGE_NAME}
  REQUIRED_VARS SAM3_LIBRARY SAM3_INCLUDE_SYNCHRONOUS SAM3_INCLUDE_ASYNCHRONOUS
)

if(${CMAKE_FIND_PACKAGE_NAME}_FOUND AND NOT TARGET sam3::libsam3)
  add_library(sam3::libsam3 STATIC IMPORTED)
  set_target_properties(sam3::libsam3 PROPERTIES
    IMPORTED_LOCATION "${SAM3_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES
      "${SAM3_INCLUDE_SYNCHRONOUS};${SAM3_INCLUDE_ASYNCHRONOUS}"
  )

  if(WIN32)
    target_link_libraries(sam3::libsam3 INTERFACE ws2_32)
  endif()
endif()
