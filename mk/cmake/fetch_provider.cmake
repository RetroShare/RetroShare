## ------------------------------------------------------------------------- ##
 # mk/cmake/fetchcontent_provider.cmake
 # This file is part of RetroShare.
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

# 3.7 : if(VERSION_GREATER_EQUAL)
cmake_minimum_required(VERSION 3.7...4.4)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
	set(FETCH_DEPENDENCY_PROVIDER_DEFAULT ON)
else()
	set(FETCH_DEPENDENCY_PROVIDER_DEFAULT OFF)
endif()
set(FETCH_DEPENDENCY_PROVIDER ${FETCH_DEPENDENCY_PROVIDER_DEFAULT} CACHE BOOL
	"use the Fetch dependency provider to get project dependencies"
)

set(FETCH_DEPENDENCY_PROVIDER_MODE NORMAL CACHE STRING
	"operation mode for the Fetch dependency provider \
		(CMake will always fall back to the built-in find_package)"
)
set_property(CACHE FETCH_DEPENDENCY_PROVIDER_MODE PROPERTY STRINGS
	NORMAL DISABLED FIND_ONLY FETCH_ONLY FIND_FIRST FETCH_FIRST
)

set(FETCH_DEPENDENCY_PROVIDER_ORDER FIND_FIRST CACHE STRING
	"operation mode for the Fetch dependency provider \
		(CMake will always fall back to the built-in find_package)"
)
set_property(CACHE FETCH_DEPENDENCY_PROVIDER_ORDER PROPERTY STRINGS
	FIND_FIRST FETCH_FIRST
)


### configure the default mode / order
if(FETCH_DEPENDENCY_PROVIDER_MODE STREQUAL "NORMAL")
	set(fetch_dependency_provider_find TRUE)
	set(fetch_dependency_provider_fetch TRUE)
elseif(FETCH_DEPENDENCY_PROVIDER_MODE STREQUAL "DISABLED")
	set(fetch_dependency_provider_find FALSE)
	set(fetch_dependency_provider_fetch FALSE)
elseif(FETCH_DEPENDENCY_PROVIDER_MODE STREQUAL "FIND_ONLY")
	set(fetch_dependency_provider_find TRUE)
	set(fetch_dependency_provider_fetch FALSE)
elseif(FETCH_DEPENDENCY_PROVIDER_MODE STREQUAL "FETCH_ONLY")
	set(fetch_dependency_provider_find FALSE)
	set(fetch_dependency_provider_fetch TRUE)
else()
	message(FATAL_ERROR
		"invalid fetch provider mode: ${FETCH_DEPENDENCY_PROVIDER_MODE}"
	)
endif()
if(FETCH_DEPENDENCY_PROVIDER_ORDER STREQUAL "FIND_FIRST")
	set(fetch_dependency_provider_find1st TRUE)
elseif(FETCH_DEPENDENCY_PROVIDER_ORDER STREQUAL "FETCH_FIRST")
	set(fetch_dependency_provider_find1st FALSE)
else()
	message(FATAL_ERROR
		"invalid fetch provider order: ${FETCH_DEPENDENCY_PROVIDER_ORDER}"
	)
endif()


macro(fetch_provide_dependency method package)
	if("${method}" STREQUAL "FIND_PACKAGE")
		cmake_parse_arguments(fetch_provider_${package}
			""
			""
			""
			${ARGN}
		)
	elseif("${method}" STREQUAL "FETCHCONTENT_MAKEAVAILABLE_SERIAL")
		message(FATAL_ERROR
			"Fetch dependency provider does support method ${method}"
		)
	else()
		message(FATAL_ERROR
			"Unrecognized method for fetch_provide_dependency: ${method}"
		)
	endif()
	set(FETCH_PROVIDER_${package}_MODE DEFAULT CACHE STRING
		"control how the Fetch dependency provider handles ${package}"
	)
	set_property(CACHE FETCH_PROVIDER_${package}_MODE PROPERTY STRINGS
		DEFAULT NORMAL DISABLED FIND_ONLY FETCH_ONLY FIND_FIRST FETCH_FIRST
	)
	set(FETCH_PROVIDER_${package}_ORDER DEFAULT CACHE STRING
		"control how the Fetch dependency provider handles ${package}"
	)
	set_property(CACHE FETCH_PROVIDER_${package}_ORDER PROPERTY STRINGS
		DEFAULT FIND_FIRST FETCH_FIRST
	)
	set(fetch_provider_${package}_find ${fetch_dependency_provider_find})
	set(fetch_provider_${package}_fetch ${fetch_dependency_provider_fetch})
	set(fetch_provider_${package}_find1st ${fetch_dependency_provider_find1st})
	if(FETCH_PROVIDER_${package}_MODE STREQUAL "DEFAULT")
	elseif(FETCH_PROVIDER_${package}_MODE STREQUAL "NORMAL")
		set(fetch_provider_${package}_find TRUE)
		set(fetch_provider_${package}_fetch TRUE)
	elseif(FETCH_PROVIDER_${package}_MODE STREQUAL "DISABLED")
		set(fetch_provider_${package}_find FALSE)
		set(fetch_provider_${package}_fetch FALSE)
	elseif(FETCH_PROVIDER_${package}_MODE STREQUAL "FIND_ONLY")
		set(fetch_provider_${package}_find TRUE)
		set(fetch_provider_${package}_fetch FALSE)
	elseif(FETCH_PROVIDER_${package}_MODE STREQUAL "FETCH_ONLY")
		set(fetch_provider_${package}_fetch TRUE)
		set(fetch_provider_${package}_find FALSE)
	else()
		message(FATAL_ERROR
			"invalid fetch provider mode: ${FETCH_PROVIDER_${package}_MODE}"
		)
	endif()
	if(FETCH_PROVIDER_${package}_ORDER STREQUAL "DEFAULT")
	elseif(FETCH_PROVIDER_${package}_ORDER STREQUAL "FIND_FIRST")
		set(fetch_provider_${package}_find1st TRUE)
	elseif(FETCH_PROVIDER_${package}_ORDER STREQUAL "FETCH_FIRST")
		set(fetch_provider_${package}_find1st FALSE)
	else()
		message(FATAL_ERROR
			"invalid fetch provider order: ${FETCH_PROVIDER_${package}_ORDER}"
		)
	endif()
	set(${package}_FOUND FALSE)
	set(fetch_provider_${package}_findargs
		${fetch_provider_${package}_UNPARSED_ARGUMENTS}
	)
	list(REMOVE_ITEM fetch_provider_${package}_findargs REQUIRED)
	if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
		list(APPEND fetch_provider_${package}_findargs BYPASS_PROVIDER)
	endif()
	if(CMAKE_VERSION VERSION_GREATER_EQUAL 4.0)
		list(APPEND fetch_provider_${package}_findargs OPTIONAL)
	endif()
	if(fetch_provider_${package}_find AND fetch_provider_${package}_find1st)
		find_package(${package} ${fetch_provider_${package}_findargs})
	endif()
	if(NOT ${package}_FOUND AND fetch_provider_${package}_fetch)
		# message(STATUS "Fetching ${package}...")
		set(FETCH_PROVIDER_PACKAGE_NAME ${package})
		set(FETCH_${package}_ARGS
			${fetch_provider_${package}_UNPARSED_ARGUMENTS}
		)
		include(Fetch${package} OPTIONAL
			RESULT_VARIABLE fetch_provider_${package}_fetchfile
		)
		if(NOT fetch_provider_${package}_fetchfile)
			# message(STATUS "No fetch module found for ${package}.")
		elseif(NOT ${package}_FOUND)
			message(WARNING
				"Fetch module '${fetch_provider_${package}_fetchfile}'"
				" failed to set `${package}_FOUND`."
				" Either the package could not be fetched (unusual),"
				" or the module has a bug."
			)
		else()
			# message(STATUS "Successfully fetched ${package}.")
		endif()
	endif()
	if(NOT ${package}_FOUND AND
		fetch_provider_${package}_find AND
		NOT fetch_provider_${package}_find1st
	)
		find_package(${package} ${fetch_provider_${package}_findargs})
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
