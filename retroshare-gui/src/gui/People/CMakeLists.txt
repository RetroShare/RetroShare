# ------------------------------------------------------------------------ *\
# src/gui/People/CMakeLists.txt
# This file is part of RetroShare-GUI.
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

target_sources(${PROJECT_NAME}
	PUBLIC FILE_SET HEADERS FILES

	PRIVATE FILE_SET private_HEADERS FILES
	PeopleDialog.h
	CircleWidget.h
	IdentityWidget.h

	${PRIVATE_FILESET_SOURCES_FILES}
	PeopleDialog.cpp
	CircleWidget.cpp
	IdentityWidget.cpp

	${PRIVATE_FILESET_UI_FILES}
	PeopleDialog.ui
	CircleWidget.ui
	IdentityWidget.ui

	${PRIVATE_FILESET_RESOURCES_FILES}
)
