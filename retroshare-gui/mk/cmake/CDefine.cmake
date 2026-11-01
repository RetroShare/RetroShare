## ------------------------------------------------------------------------- ##
 # mk/cmake/CDefine.cmake
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

# 3.7 : cmake_parse_arguments(PARSE_ARGV)
# 3.7 : VERSION_GREATER_EQUAL
cmake_minimum_required(VERSION 3.7...4.4)

function(cdefine macro)
	if(CMAKE_VERSION VERSION_GREATER_EQUAL 4.4)
		set(PARSE_ARGN PARSE_ARGN)
	else()
		set(PARSE_ARGN PARSE_ARGV 1)
	endif()
	cmake_parse_arguments(${PARSE_ARGN} cdefine
		"VALID_0;STRING"
		"DEFAULT"
		""
	)
	list(JOIN cdefine_UNPARSED_ARGUMENTS " \\\n\t" value)
	if(cdefine_STRING)
		set(q "\"")
	else()
		set(q )
	endif()
	if(value OR (cdefine_VALID_0 AND value EQUAL 0))
		set(DEFINE_${macro} "#define ${macro} ${q}${value}${q}")
	elseif(DEFINED cdefine_DEFAULT)
		set(DEFINE_${macro} "#define ${macro} ${q}${cdefine_DEFAULT}${q}")
	else()
		set(DEFINE_${macro} "/* #undef ${macro} ${q}${value}${q} */")
	endif()
	set(DEFINE_${macro} ${DEFINE_${macro}} PARENT_SCOPE)
endfunction()
