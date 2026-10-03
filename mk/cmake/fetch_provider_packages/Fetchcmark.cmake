# ------------------------------------------------------------------------ *\
# mk/cmake/fetch_provider_packages/FetchBitDHT.cmake
# This file is part of libRetroShare.
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

cmake_minimum_required(VERSION 3.24...4.4)

FetchContent_MakeAvailable(${FETCH_PROVIDER_PACKAGE_NAME})
set(${FETCH_PROVIDER_PACKAGE_NAME}_FOUND TRUE)

if(NOT TARGET cmark::cmark)
  add_library(cmark::cmark ALIAS cmark)
endif()
