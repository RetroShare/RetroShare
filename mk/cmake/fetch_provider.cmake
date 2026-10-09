# ------------------------------------------------------------------------ *\
# mk/cmake/fetchcontent_provider.cmake
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

include(CMakeDependentOption)
cmake_dependent_option(FETCH_DEPENDENCY_PROVIDER
	"Use the Fetch dependency provider to get project dependencies"
	ON
	"CMAKE_VERSION VERSION_GREATER_EQUAL 3.24"
	OFF
)

macro(fetch_provide_dependency method package)
	set(FETCH_PROVIDER_${package} NORMAL CACHE STRING
		"control how the Fetch dependency provider handles ${package}"
	)
	set_property(CACHE FETCH_PROVIDER_${package} PROPERTY STRINGS
		NORMAL FIND_ONLY FETCH_ONLY
	)
	set(fetch_provider_${package}_findargs
		${ARGN} BYPASS_PROVIDER
	)
	list(REMOVE_ITEM fetch_provider_${package}_findargs REQUIRED)
	if(CMAKE_VERSION VERSION_GREATER_EQUAL 4.0)
		list(APPEND fetch_provider_${package}_findargs OPTIONAL)
	endif()
	if(NOT FETCH_PROVIDER_${package} STREQUAL FETCH_ONLY)
		find_package(${package} ${fetch_provider_${package}_findargs})
	endif()
	if(NOT ${package}_FOUND AND NOT FETCH_PROVIDER_${package} STREQUAL FIND_ONLY)
		message(STATUS "Fetching ${package}...")
		set(FETCH_PROVIDER_PACKAGE_NAME ${package})
		include(Fetch${package} OPTIONAL
			RESULT_VARIABLE fetch_provider_${package}_fetchfile
		)
		if(NOT fetch_provider_${package}_fetchfile)
			message(STATUS "No fetch module found for ${package}.")
		elseif(NOT ${package}_FOUND)
			message(WARNING
				"The fetch module for ${package} was not able to get the package."
			)
		else()
			message(STATUS "Successfully fetched ${package}.")
		endif()
	endif()
endmacro(fetch_provide_dependency)

if(FETCH_DEPENDENCY_PROVIDER)
	if(CMAKE_VERSION VERSION_LESS 3.24)
		message(FATAL_ERROR
			"Dependency providers are not supported in CMake <3.24."
			" Please set FETCH_DEPENDENCY_PROVIDER=OFF"
			" or use a more recent version of CMake."
		)
	endif()

	cmake_language(SET_DEPENDENCY_PROVIDER fetch_provide_dependency
		SUPPORTED_METHODS FIND_PACKAGE
	)
	message(STATUS "registered Fetch dependency provider")
endif()
