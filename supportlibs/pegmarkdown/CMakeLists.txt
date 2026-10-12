## ------------------------------------------------------------------------- ##
 # CMakeLists.txt
 # This file is part of pegmarkdown
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

# 3.1 : target_sources()
# 3.11: add_library() ... target_sources()
cmake_minimum_required(VERSION 3.11...4.4)

project(pegmarkdown
	LANGUAGES C
)

add_library(pegmarkdown STATIC)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.23)
	target_sources(${PROJECT_NAME}
		PUBLIC FILE_SET HEADERS
		BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}

		PRIVATE FILE_SET private_HEADERS TYPE HEADERS
		BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	)
	set(PUBLIC_FILESET_HEADERS_FILES PUBLIC FILE_SET HEADERS FILES)
	set(PRIVATE_FILESET_private_HEADERS_FILES
		PRIVATE FILE_SET private_HEADERS FILES
	)
else()
	set(PUBLIC_FILESET_HEADERS_FILES PRIVATE)
	set(PRIVATE_FILESET_private_HEADERS_FILES PRIVATE)
	target_include_directories(${PROJECT_NAME}
		PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	)
endif()

if(CMAKE_VERSION VERSION_GREATER_EQUAL 4.4)
	target_sources(${PROJECT_NAME}
		PRIVATE FILE_SET SOURCES
	)
	set(PRIVATE_FILESET_SOURCES_FILES PRIVATE FILE_SET SOURCES FILES)
else()
	set(PRIVATE_FILESET_SOURCES_FILES PRIVATE)
endif()

target_sources(pegmarkdown
	${PUBLIC_FILESET_HEADERS_FILES}
	GLibFacade.h
	markdown_lib.h
	markdown_peg.h
	odf.h
	parsing_functions.h
	utility_functions.h

	${PRIVATE_FILESET_SOURCES_FILES}
	GLibFacade.c
	markdown_lib.c
	markdown_output.c
	markdown_parser.c
	parsing_functions.c
	odf.c
	utility_functions.c
)

target_compile_definitions(pegmarkdown PRIVATE _GNU_SOURCE)


################################################################################
### install

include(GNUInstallDirs)
set(BUILD_CONFIG_DIR "${PROJECT_BINARY_DIR}/cmake")
set(INSTALL_CONFIG_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
set(TARGET_NAMESPACE pegmarkdown)
set(INSTALL_COMPONENT COMPONENT pegmarkdown)
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.23)
	set(FILESET_HEADERS_INSTALL
		FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/pegmarkdown
	)
else()
	set(FILESET_HEADERS_INSTALL
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/pegmarkdown
  )
	get_target_property(${PROJECT_NAME}_SOURCES ${PROJECT_NAME} SOURCES)
	foreach(source ${${PROJECT_NAME}_SOURCES})
		string(TOLOWER ${source} sourceLower)
		if(NOT sourceLower MATCHES "\\.(h|hh|h\\+\\+|hpp)\$")
			continue()
		endif()
		foreach(base_ ${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR})
			string(FIND ${source} ${base_}/ basePos)
			if(basePos EQUAL 0)
				set(base ${base_})
				break()
			endif()
		endforeach()
		if(NOT basePos EQUAL 0)
			message(SEND_ERROR "couldn't find base for header ${source}")
			continue()
		endif()

		get_filename_component(dest ${source} DIRECTORY)
		file(RELATIVE_PATH dest ${base} ${dest})
		install(FILES ${source}
			DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/pegmarkdown/${dest}
			${INSTALL_COMPONENT}
		)
	endforeach()
endif()

install(TARGETS ${PROJECT_NAME}
	EXPORT ${PROJECT_NAME}_export
	${INSTALL_COMPONENT}
	${FILESET_HEADERS_INSTALL}
	BUNDLE
		DESTINATION ${CMAKE_INSTALL_BINDIR}
)


## config for install tree
install(EXPORT ${PROJECT_NAME}_export
	DESTINATION "${INSTALL_CONFIG_DIR}"
	FILE ${PROJECT_NAME_LOWER}-targets.cmake
	NAMESPACE ${TARGET_NAMESPACE}::
	${INSTALL_COMPONENT}
)
include(CMakePackageConfigHelpers)
configure_package_config_file(
	pegmarkdown-config.cmake.in
	"${BUILD_CONFIG_DIR}/${PROJECT_NAME_LOWER}-config-install.cmake"
	INSTALL_DESTINATION "${INSTALL_CONFIG_DIR}"
)
install(FILES
	"${BUILD_CONFIG_DIR}/${PROJECT_NAME_LOWER}-config-install.cmake"
	DESTINATION "${INSTALL_CONFIG_DIR}"
	RENAME ${PROJECT_NAME_LOWER}-config.cmake
	${INSTALL_COMPONENT}
)


## config for build tree
export(EXPORT ${PROJECT_NAME}_export
  FILE "${BUILD_CONFIG_DIR}/${PROJECT_NAME_LOWER}-targets.cmake"
  NAMESPACE ${TARGET_NAMESPACE}::
)
include(CMakePackageConfigHelpers)
configure_package_config_file(
	pegmarkdown-config.cmake.in
  "${BUILD_CONFIG_DIR}/${PROJECT_NAME_LOWER}-config.cmake"
  INSTALL_DESTINATION "${BUILD_CONFIG_DIR}"
	INSTALL_PREFIX "${PROJECT_BINARY_DIR}"
)


## Alias target for super-builds (brought in via add_subdirectory)
add_library(${TARGET_NAMESPACE}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
