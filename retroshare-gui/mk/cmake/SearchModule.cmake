## ------------------------------------------------------------------------- ##
 # mk/cmake/SearchModule.cmake
 # This file is part of RetroShare-GUI.
 #
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

# 3.5 : cmake_parse_arguments()
# 3.7 : cmake_parse_arguments(PARSE_ARGV)
# 3.7 : if(VERSION_GREATER_EQUAL)
cmake_minimum_required(VERSION 3.7...4.4)

function(search_module module result)
	cmake_parse_arguments(PARSE_ARGV 0 arg
		"REQUIRED"
		""
		""
	)
	if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.15)
		list(POP_FRONT arg_UNPARSED_ARGUMENTS module result)
	endif()

	if(EXISTS "${module}")
		if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.20)
			cmake_path(ABSOLUTE_PATH module
				OUTPUT_VARIABLE ${result}
				NORMALIZE
			)
		else()
			get_filename_component(${result} "${module}" ABSOLUTE)
		endif()
		set(${result} ${${result}} PARENT_SCOPE)
		return()
	endif()

	set(search_path ${CMAKE_MODULE_PATH})
	set(cmake_root_modules ${CMAKE_ROOT}/Modules)
	if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.20)
		cmake_path(IS_PREFIX cmake_root_modules ${CMAKE_CURRENT_LIST_DIR} in_root)
		if(in_root)
			list(PREPEND search_path ${cmake_root_modules})
		else()
			list(APPEND search_path ${cmake_root_modules})
		endif()
	else()
		string(FIND ${CMAKE_CURRENT_LIST_DIR}/ ${cmake_root_modules}/ in_root)
		if(in_root EQUAL 0)
			set(search_path ${cmake_root_modules} ${search_path})
		else()
			list(APPEND search_path ${cmake_root_modules})
		endif()
	endif()

	foreach(path ${search_path})
		if(EXISTS "${path}/${module}.cmake")
			set(${result} "${path}/${module}.cmake")
			return()
		endif()
	endforeach()

	if(arg_REQUIRED)
		message(FATAL_ERROR "module does not exist: ${module}")
	endif()

	set(${result} "${module}-NOTFOUND")
endfunction()
