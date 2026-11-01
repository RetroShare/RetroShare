## ------------------------------------------------------------------------- ##
 # src/data/stylesheets/app/CMakeLists.txt
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

cmake_minimum_required(VERSION 3.0...4.4)

include(GNUInstallDirs)

target_sources(${PROJECT_NAME}
	${PRIVATE_FILESET_RESOURCES_FILES}
	stylesheets.qrc
	Standard_Dark/Standard_Dark.qrc
	Standard_Light/Standard_Light.qrc
)

install(FILES
	blue.qss
	groove.qss
	orangesurfer.qss
	qdarkstyle.qss
	qdarkstyle-v2.qss
	qlive.qss
	redscorpion.qss
	retroclassic.qss
	silver.qss
	uus.qss
	yaba.qss
	yeah.qss
	DESTINATION ${RS_INSTALL_DATADIR}/qss
	COMPONENT retroshare-gui
)

install(DIRECTORY
	blacknight
	blue
	orangesurfer
	qdarkstyle
	qlive
	redscorpion
	silver
	uus
	yaba
	yeah
	DESTINATION ${RS_INSTALL_DATADIR}/qss
	COMPONENT retroshare-gui
)
