## ------------------------------------------------------------------------- ##
 # mk/cmake/RedirectFindPackage.cmake
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

cmake_minimum_required(VERSION 3.0...4.4)

function(redirect_find_package Name)
	string(TOLOWER ${Name} name)
	if(CMAKE_VERSION VERSION_LESS 3.24)
		# With the lack of CMAKE_FIND_PACKAGE_REDIRECTS_DIR, this isn't fully
		# reliable. We just set the <Package>_ROOT variables and hope it sticks.
		# `find_package` in CMake <3.15 unconditionally defaults to module-mode, so
		# that's a bother.
		set(CMAKE_FIND_PACKAGE_REDIRECTS_DIR ${CMAKE_BINARY_DIR}/pkgRedirects)
		set(CMAKE_FIND_PACKAGE_REDIRECTS_DIR
			${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}
			PARENT_SCOPE
		)
		set(FIND_PACKAGE_REDIRECTS_DIR_CREATED
			$ENV{FIND_PACKAGE_REDIRECTS_DIR_CREATED}
		)
		if(NOT FIND_PACKAGE_REDIRECTS_DIR_CREATED)
			file(REMOVE_RECURSE ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR})
			set(ENV{FIND_PACKAGE_REDIRECTS_DIR_CREATED} TRUE)
		endif()
		set(${Name}_ROOT ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR} PARENT_SCOPE)
		set(ENV{${Name}_ROOT} ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR})
		set(FIND_PACKAGE_PREFER_CONFIG TRUE)
	endif()
	message(STATUS "setting up redirects for ${Name}")
	if(
		NOT EXISTS ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/${name}-config.cmake AND
		NOT EXISTS ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/${Name}Config.cmake
	)
		file(WRITE ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/${name}-config.cmake
			"message(VERBOSE \"redirecting find_package(${Name})\")\n"
			"include(\${CMAKE_CURRENT_LIST_DIR}/${name}-extra.cmake OPTIONAL)\n"
			"include(\${CMAKE_CURRENT_LIST_DIR}/${Name}Extra.cmake OPTIONAL)\n"
		)
	endif()
	if(
		NOT EXISTS ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/${name}-config-version.cmake AND
		NOT EXISTS ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/${Name}ConfigVersion.cmake
	)
		file(WRITE ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/${name}-config-version.cmake
			"set(PACKAGE_VERSION_EXACT TRUE)\n"
			"set(PACKAGE_VERSION_COMPATIBLE TRUE)\n"
		)
	endif()
endfunction()
