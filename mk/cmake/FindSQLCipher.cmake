## ---------------------------------------------------------------------- ##
 # mk/cmake/FindSQLCipher.cmake
 # This file is part of RetorShare.
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

find_package(PkgConfig)
if(PkgConfig_FOUND)
  pkg_check_modules(SQLCipher IMPORTED_TARGET sqlcipher)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SQLCipher
  REQUIRED_VARS SQLCipher_FOUND
  VERSION_VAR SQLCipher_VERSION
  HANDLE_VERSION_RANGE
)

if(SQLCipher_FOUND)
  add_library(SQLCipher::SQLCipher ALIAS PkgConfig::SQLCipher)
endif()
