# ------------------------------------------------------------------------ *\
# mk/cmake/EnvironmentModification.cmake
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

# 3.12: list(TRANSFORM)
cmake_minimum_required(VERSION 3.12...4.4)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 4.2)
	function(EnvironmentModification)
	endfunction()
elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 3.25)
	function(EnvironmentModification outvar)
		list(TRANSFORM ARGN PREPEND --modify\;)
		set(${outvar} ${CMAKE_COMMAND} -E env ${ARGN} -- PARENT_SCOPE)
	endfunction(EnvironmentModification)
else()
	function(EnvironmentModification outvar)
		list(TRANSFORM ARGN REPLACE "^([^=]+)=set:(.*)\$" "\\1=\\2")
		list(TRANSFORM ARGN REPLACE "^([^=]+)=unset:(.*)\$" "--unset=\\1")
		set(${outvar} ${CMAKE_COMMAND} -E env ${ARGN} -- PARENT_SCOPE)
	endfunction(EnvironmentModification)
endif()
