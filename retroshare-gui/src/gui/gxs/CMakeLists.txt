# ------------------------------------------------------------------------ *\
# src/gui/gxs/CMakeLists.txt
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
cmake_minimum_required(VERSION 3.13...4.4)

target_sources(${PROJECT_NAME}
	${PUBLIC_FILESET_HEADERS_FILES}

	${PRIVATE_FILESET_private_HEADERS_FILES}
	GxsGroupDialog.h
	GxsIdDetails.h
	GxsIdChooser.h
	GxsIdLabel.h
	GxsCircleChooser.h
	GxsCircleLabel.h
	GxsIdTreeWidgetItem.h
	GxsCommentTreeWidget.h
	GxsCommentContainer.h
	GxsCommentDialog.h
	GxsCreateCommentDialog.h
	GxsGroupFrameDialog.h
	GxsMessageFrameWidget.h
	GxsMessageFramePostWidget.h
	GxsGroupShareKey.h
	GxsUserNotify.h
	GxsFeedWidget.h
	GxsStatisticsProvider.h

	${PRIVATE_FILESET_SOURCES_FILES}
	GxsGroupDialog.cpp
	GxsIdDetails.cpp
	GxsIdChooser.cpp
	GxsIdLabel.cpp
	GxsCircleChooser.cpp
	GxsGroupShareKey.cpp
	GxsCircleLabel.cpp
	GxsIdTreeWidgetItem.cpp
	GxsCommentTreeWidget.cpp
	GxsCommentContainer.cpp
	GxsCommentDialog.cpp
	GxsCreateCommentDialog.cpp
	GxsGroupFrameDialog.cpp
	GxsMessageFrameWidget.cpp
	GxsMessageFramePostWidget.cpp
	GxsUserNotify.cpp
	GxsFeedWidget.cpp
	GxsStatisticsProvider.cpp

	${PRIVATE_FILESET_UI_FILES}
	GxsGroupDialog.ui
	GxsCommentContainer.ui
	GxsCommentDialog.ui
	GxsCreateCommentDialog.ui
	GxsGroupFrameDialog.ui
	GxsGroupShareKey.ui

	${PRIVATE_FILESET_RESOURCES_FILES}
)

if(RS_GXSWIKIPOS)
	target_sources(${PROJECT_NAME}
		${PUBLIC_FILESET_HEADERS_FILES}

		${PRIVATE_FILESET_private_HEADERS_FILES}
		WikiGroupDialog.h
		RsGxsUpdateBroadcastBase.h
		RsGxsUpdateBroadcastWidget.h
		RsGxsUpdateBroadcastPage.h

		${PRIVATE_FILESET_SOURCES_FILES}
		WikiGroupDialog.cpp
		RsGxsUpdateBroadcastBase.cpp
		RsGxsUpdateBroadcastWidget.cpp
		RsGxsUpdateBroadcastPage.cpp

		${PRIVATE_FILESET_UI_FILES}

		${PRIVATE_FILESET_RESOURCES_FILES}
	)
endif(RS_GXSWIKIPOS)
