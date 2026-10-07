# ------------------------------------------------------------------------ *\
# src/gui/Posted/CMakeLists.txt
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

# 3.1 : target_sources()
# 3.13: CMP0076
# 3.23: target_sources(FILE_SET)
cmake_minimum_required(VERSION 3.23...4.4)

target_sources(${PROJECT_NAME}
	PUBLIC FILE_SET HEADERS FILES

	PRIVATE FILE_SET private_HEADERS FILES
	PostedDialog.h
	PostedListWidgetWithModel.h
	PostedPostsModel.h
	BoardPostDisplayWidget.h
	PostedItem.h
	PostedCardView.h
	PostedGroupDialog.h
	PostedCreatePostDialog.h
	PhotoView.h
	PostedUserNotify.h
	BoardPostImageHelper.h

	${PRIVATE_FILESET_SOURCES_FILES}
	PostedDialog.cpp
	PostedListWidgetWithModel.cpp
	BoardPostDisplayWidget.cpp
	PostedPostsModel.cpp
	PostedItem.cpp
	PostedCardView.cpp
	PostedGroupDialog.cpp
	PostedCreatePostDialog.cpp
	PhotoView.cpp
	PostedUserNotify.cpp
	BoardPostImageHelper.cpp

	${PRIVATE_FILESET_UI_FILES}
	PostedListWidgetWithModel.ui
	BoardPostDisplayWidget_compact.ui
	BoardPostDisplayWidget_card.ui
	PostedItem.ui
	PostedCardView.ui
	PostedCreatePostDialog.ui
	PhotoView.ui

	${PRIVATE_FILESET_RESOURCES_FILES}
	Posted_images.qrc
)
