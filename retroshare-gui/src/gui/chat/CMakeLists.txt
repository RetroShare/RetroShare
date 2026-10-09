# ------------------------------------------------------------------------ *\
# src/gui/chat/CMakeLists.txt
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
	ChatDialog.h
	ChatLobbyDialog.h
	ChatLobbyUserNotify.h
	ChatStyle.h
	ChatTabWidget.h
	ChatUserNotify.h
	ChatWidget.h
	CreateLobbyDialog.h
	PopupChatDialog.h
	PopupChatWindow.h
	PopupDistantChatDialog.h

	${PRIVATE_FILESET_SOURCES_FILES}

	PopupChatWindow.cpp
	PopupChatDialog.cpp
	PopupDistantChatDialog.cpp
	ChatTabWidget.cpp
	ChatWidget.cpp
	ChatDialog.cpp
	ChatLobbyDialog.cpp
	CreateLobbyDialog.cpp
	ChatStyle.cpp
	ChatUserNotify.cpp
	ChatLobbyUserNotify.cpp

	${PRIVATE_FILESET_UI_FILES}
	ChatLobbyDialog.ui
	ChatTabWidget.ui
	ChatWidget.ui
	CreateLobbyDialog.ui
	PopupChatDialog.ui
	PopupChatWindow.ui

	${PRIVATE_FILESET_RESOURCES_FILES}
)
