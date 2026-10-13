## ------------------------------------------------------------------------- ##
 # src/data/icons/CMakeLists.txt
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

set(icons
	scalable.svg
	24x24.png
	48x48.png
	64x64.png
	128x128.png
	pixmap.xpm
)

foreach(icon ${icons})
	if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.20)
		cmake_path(GET icon STEM size)
	else()
		get_filename_component(size ${icon} NAME_WE)
	endif()
	set(ICON_INSTALL_DESTINATION_${icon}
		${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}/apps
	)
endforeach()
set(ICON_INSTALL_DESTINATION_pixmap.xpm ${CMAKE_INSTALL_DATADIR}/pixmaps)

foreach(icon ${icons})
	if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.20)
		cmake_path(GET icon EXTENSION ext)
	else()
		get_filename_component(ext ${icon} EXT)
	endif()
	install(
		FILES ${icon}
		DESTINATION ${ICON_INSTALL_DESTINATION_${icon}}
		RENAME ${PROJECT_NAME}.${ext}
		COMPONENT retroshare-gui
	)
endforeach()
