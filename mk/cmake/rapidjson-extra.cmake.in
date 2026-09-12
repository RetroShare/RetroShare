# ------------------------------------------------------------------------ *\
# mk/cmake/rnp-extra.cmake.in
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

# INSTALL(TARGETS RapidJSON EXPORT RapidJSON-targets)
# export(EXPORT RapidJSON-targets
#   FILE "${RapidJSON_BINARY_DIR}/RapidJSON-targets.cmake"
# )
#
# include("${RapidJSON_BINARY_DIR}/RapidJSONConfig.cmake")

target_include_directories(RapidJSON INTERFACE
  "$<BUILD_INTERFACE:${RapidJSON_SOURCE_DIR}/include>"
)
