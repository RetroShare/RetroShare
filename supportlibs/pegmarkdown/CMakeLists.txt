# ------------------------------------------------------------------------ *\
# CMakeLists.txt
# This file is part of pegmarkdown
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

cmake_minimum_required(VERSION 3.24...4.4)

project(pegmarkdown
  LANGUAGES C
)

add_library(pegmarkdown STATIC
  GLibFacade.c
	markdown_lib.c
  markdown_output.c
	markdown_parser.c
	parsing_functions.c
	odf.c
	utility_functions.c
)

target_sources(pegmarkdown
  PUBLIC FILE_SET HEADERS
  BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
  FILES
  GLibFacade.h
  markdown_lib.h
	markdown_peg.h
	odf.h
	parsing_functions.h
	utility_functions.h
)

target_compile_definitions(pegmarkdown PRIVATE _GNU_SOURCE)


################################################################################
### install

set(BUILD_CONFIG_DIR "${PROJECT_BINARY_DIR}/cmake")
set(INSTALL_CONFIG_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
set(TARGET_NAMESPACE pegmarkdown)
set(INSTALL_COMPONENT COMPONENT pegmarkdown)
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

install(TARGETS ${PROJECT_NAME}
	EXPORT ${PROJECT_NAME}_export
	${INSTALL_COMPONENT}
	FILE_SET HEADERS
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/pegmarkdown"
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
