## ------------------------------------------------------------------------- ##
 # plugins/CMakeLists.txt
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
## ------------------------------------------------------------------------- ##

# 3.24: CMAKE_FIND_PACKAGE_REDIRECTS_DIR
cmake_minimum_required(VERSION 3.24...4.4)

if(RS_GUI)
	# don't accidentally find an installed version of RetroShare-GUI
	file(WRITE ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/RetroShare-GUIConfig.cmake
		"message(STATUS \"using RetroShare-GUI from this build\")"
	)

	add_subdirectory(FeedReader)
	add_subdirectory(VOIP)
endif()
