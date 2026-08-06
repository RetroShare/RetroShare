# ------------------------------------------------------------------------ *\
# mk/cmake/udp-discovery-extra.cmake.in
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

add_library(udp-discovery-cpp::udp-discovery ALIAS udp-discovery)

target_include_directories(udp-discovery PUBLIC
	"${udp-discovery-cpp_SOURCE_DIR}"
)


# Legacy C submodules trip strict GCC >= 15 diagnostics that are now errors by
# default; downgrade them to warnings for now.
if(WIN32)
	target_compile_options(udp-discovery PRIVATE
	  -Wno-error=incompatible-pointer-types
	  -Wno-error=int-conversion
	  -Wno-error=implicit-function-declaration
	)
endif()
