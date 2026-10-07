# ------------------------------------------------------------------------ *\
# src/gui/settings/CMakeLists.txt
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
	AboutPage.h
	AddFileAssociationDialog.h
	AppearancePage.h
	ChannelPage.h
	ChatPage.h
	CryptoPage.h
	FileAssociationsPage.h
	ForumPage.h
	GeneralPage.h
	GroupFrameSettingsWidget.h
	MessagePage.h
	NewTag.h
	NotifyPage.h
	PeoplePage.h
	PluginItem.h
	PluginsPage.h
	PostedPage.h
	rsettings.h
	rsettingswin.h
	RsharePeerSettings.h
	rsharesettings.h
	RSPermissionMatrixWidget.h
	ServerPage.h
	ServicePermissionsPage.h
	SoundPage.h
	TransferPage.h

	${PRIVATE_FILESET_SOURCES_FILES}
	RSPermissionMatrixWidget.cpp
	rsharesettings.cpp
	RsharePeerSettings.cpp
	rsettings.cpp
	rsettingswin.cpp
	GeneralPage.cpp
	AboutPage.cpp
	ServerPage.cpp
	NotifyPage.cpp
	CryptoPage.cpp
	PeoplePage.cpp
	MessagePage.cpp
	NewTag.cpp
	ForumPage.cpp
	PluginsPage.cpp
	PluginItem.cpp
	AppearancePage.cpp
	FileAssociationsPage.cpp
	SoundPage.cpp
	TransferPage.cpp
	ChatPage.cpp
	ChannelPage.cpp
	PostedPage.cpp
	ServicePermissionsPage.cpp
	AddFileAssociationDialog.cpp
	GroupFrameSettingsWidget.cpp

	${PRIVATE_FILESET_UI_FILES}
	AboutPage.ui
	AppearancePage.ui
	ChannelPage.ui
	ChatPage.ui
	CryptoPage.ui
	ForumPage.ui
	GeneralPage.ui
	GroupFrameSettingsWidget.ui
	MessagePage.ui
	NewTag.ui
	NotifyPage.ui
	PeoplePage.ui
	PluginItem.ui
	PluginsPage.ui
	PostedPage.ui
	ServerPage.ui
	ServicePermissionsPage.ui
	settingsw.ui
	SoundPage.ui
	TransferPage.ui

	${PRIVATE_FILESET_RESOURCES_FILES}
)

if(RS_JSON_API)
	target_sources(${PROJECT_NAME}
		PRIVATE FILE_SET private_HEADERS FILES
		JsonApiPage.h

		${PRIVATE_FILESET_SOURCES_FILES}
		JsonApiPage.cc

		${PRIVATE_FILESET_UI_FILES}
		JsonApiPage.ui
	)
endif(RS_JSON_API)

if(RS_WEBUI)
	target_sources(${PROJECT_NAME}
		PRIVATE FILE_SET private_HEADERS FILES
		WebuiPage.h

		${PRIVATE_FILESET_SOURCES_FILES}
		WebuiPage.cpp

		${PRIVATE_FILESET_UI_FILES}
		WebuiPage.ui
	)
endif(RS_WEBUI)
