## ------------------------------------------------------------------------- ##
 # mk/cmake/RuntimeDependenciesInstallScript.cmake
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
# 3.7 : if(VERSION_GREATER_EQUAL)
# 3.10: CMP0070
cmake_minimum_required(VERSION 3.10...4.4)

include(GNUInstallDirs)
if(CMAKE_IMPORT_LIBRARY_SUFFIX)
	# DLL platform -- DLLs go in runtime directory
	set(CMAKE_INSTALL_SODIR ${CMAKE_INSTALL_BINDIR})
else()
	# non-DLL platform -- SOs go in library directory
	set(CMAKE_INSTALL_SODIR ${CMAKE_INSTALL_LIBDIR})
endif()

if(CMAKE_VERSION VERSION_LESS 3.17)
	set(runtime_dependencies_install_script-list_dir ${CMAKE_CURRENT_LIST_DIR})
endif()

function(runtime_dependencies_install_script output_file)
	cmake_parse_arguments(PARSE_ARGV 1 RUNTIME_DEPENDENCY
		""
		"DESTINATION;UNRESOLVED;CONFLICT"
		"EXECUTABLES;LIBRARIES;MODULES;DIRECTORIES"
	)
	if(NOT DEFINED RUNTIME_DEPENDENCY_DESTINATION)
		set(RUNTIME_DEPENDENCY_DESTINATION ${CMAKE_INSTALL_SODIR})
	endif()
	if(NOT DEFINED RUNTIME_DEPENDENCY_UNRESOLVED)
		set(RUNTIME_DEPENDENCY_UNRESOLVED FATAL_ERROR)
	endif()
	if(NOT DEFINED RUNTIME_DEPENDENCY_CONFLICT)
		set(RUNTIME_DEPENDENCY_CONFLICT WARNING)
	endif()

	if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.20)
		cmake_path(ABSOLUTE_PATH output_file
			BASE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		)
	else()
		get_filename_component(output_file
			${output_file}
			ABSOLUTE
			BASE_DIR ${CMAKE_CURRENT_BINARY_DIR}
		)
	endif()

	set(templateName RuntimeDependenciesInstallScript-template.cmake.in)
	if(CMAKE_VERSION VERSION_LESS 3.17)
		set(CMAKE_CURRENT_FUNCTION_LIST_DIR
			${runtime_dependencies_install_script-list_dir}
		)
	endif()
	configure_file(
		${CMAKE_CURRENT_FUNCTION_LIST_DIR}/${templateName}
		${output_file}.gen
		@ONLY
	)
	file(GENERATE
		OUTPUT ${output_file}
		INPUT ${output_file}.gen
	)
endfunction()
