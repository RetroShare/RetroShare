## ------------------------------------------------------------------------- ##
 # mk/cmake/IncludeNrsMacro.cmake
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
# 3.7 : if(VERSION_GREATER_EQUAL)
# 3.10: string(PREPEND)
cmake_minimum_required(VERSION 3.10...4.4)

include(SearchModule)

macro(include_nrs file)
	cmake_parse_arguments(include_nrs
		"NO_RETURN_SCOPE;OPTIONAL;NO_POLICY_SCOPE;NO_DIAGNOSTIC_SCOPE"
		"RESULT_VARIABLE"
		""
		${ARGN}
	)

	if(NOT include_nrs_NO_RETURN_SCOPE)
		include(${ARGV})
	else()

	if(NOT include_nrs_OPTIONAL)
		set(include_nrs_required REQUIRED)
	else()
		set(include_nrs_required "")
	endif()
	search_module("${file}" include_nrs_file ${include_nrs_required})
	if(DEFINED include_nrs_RESULT_VARIABLE)
		set(${include_nrs_RESULT_VARIABLE} ${include_nrs_file})
	endif()

	if(include_nrs_file)
		set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
			${include_nrs_file}
		)
		file(READ ${include_nrs_file} include_nrs_content)
	else()
		set(include_nrs_content "")
	endif()

	set(include_nrs_scopes)
	if(NOT include_nrs_NO_POLICY_SCOPE)
		list(APPEND include_nrs_scopes POLICIES)
	endif()
	if(CMAKE_VERSION VERSION_GREATER_EQUAL 4.4 AND
		NOT include_nrs_NO_DIAGNOSTIC_SCOPE
	)
		list(APPEND include_nrs_scopes DIAGNOSTICS)
	endif()

	if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.18)
		cmake_language(EVAL CODE
			"macro(include_nrs_macro)\n"
				"${include_nrs_content}\n"
			"endmacro()\n"
		)
	else()
		file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/include_nrs_eval.cmake
			"macro(include_nrs_macro)\n"
				"${include_nrs_content}\n"
			"endmacro()\n"
		)
		include(${CMAKE_CURRENT_BINARY_DIR}/include_nrs_eval.cmake)
	endif()

	if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.25)
		block(SCOPE_FOR ${include_nrs_scopes})
			include_nrs_macro()
		endblock()
	else()
		if(include_nrs_scopes)
			message(WARNING
				"include_nrs doesn't support policy or diagnostic scopes for "
				"CMake <3.25. "
				"Implying `NO_POLICY_SCOPE` and `NO_DIAGNOSTIC_SCOPE`."
			)
		endif()

		include_nrs_macro()
	endif()

	endif(NOT include_nrs_NO_RETURN_SCOPE)
endmacro()
