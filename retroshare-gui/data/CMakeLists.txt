## ------------------------------------------------------------------------- ##
 # src/data/CMakeLists.txt
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

# 3.7 : if(VERSION_GREATER_EQUAL)
cmake_minimum_required(VERSION 3.7...4.4)

include(GNUInstallDirs)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.4")
	target_sources(${PROJECT_NAME}
		PRIVATE FILE_SET resources TYPE SOURCES
		BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	)
	set(PRIVATE_FILESET_RESOURCES_FILES PRIVATE FILE_SET resources FILES)
else()
	set(PRIVATE_FILESET_RESOURCES_FILES PRIVATE)
endif()

add_subdirectory(logos)
add_subdirectory(sounds)
add_subdirectory(stylesheets/app)
add_subdirectory(stylesheets/chat)

target_sources(${PROJECT_NAME}
	${PRIVATE_FILESET_RESOURCES_FILES}
	emojione/emojione.qrc
	icons/icons.qrc
	images/images.qrc
)

install(
	FILES retroshare-gui.desktop
	DESTINATION "${CMAKE_INSTALL_DATADIR}/applications/"
	COMPONENT retroshare-gui
)
