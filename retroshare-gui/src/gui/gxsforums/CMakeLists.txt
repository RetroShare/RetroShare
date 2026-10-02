# ------------------------------------------------------------------------ *\
# src/gui/gxsforums/CMakeLists.txt
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
	GxsForumsDialog.h
	GxsForumGroupDialog.h
	CreateGxsForumMsg.h
	GxsForumThreadWidget.h
	GxsForumModel.h
	GxsForumUserNotify.h

	${PRIVATE_FILESET_SOURCES_FILES}
	GxsForumsDialog.cpp
	GxsForumGroupDialog.cpp
	CreateGxsForumMsg.cpp
	GxsForumThreadWidget.cpp
	GxsForumModel.cpp
	GxsForumUserNotify.cpp

	${PRIVATE_FILESET_UI_FILES}
	CreateGxsForumMsg.ui
	GxsForumThreadWidget.ui

	${PRIVATE_FILESET_RESOURCES_FILES}
)
