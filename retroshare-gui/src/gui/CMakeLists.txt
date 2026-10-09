# ------------------------------------------------------------------------ *\
# src/gui/CMakeLists.txt
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

add_subdirectory(advsearch)
add_subdirectory(chat)
if(RS_GXSCIRCLES)
	add_subdirectory(Circles)
endif(RS_GXSCIRCLES)
add_subdirectory(common)
add_subdirectory(connect)
add_subdirectory(elastic)
add_subdirectory(feeds)
add_subdirectory(FileTransfer)
add_subdirectory(groups)
if(RS_GXSGUI)
	add_subdirectory(gxs)
endif(RS_GXSGUI)
if(RS_GXSCHANNELS)
	add_subdirectory(gxschannels)
endif(RS_GXSCHANNELS)
if(RS_GXSFORUMS)
	add_subdirectory(gxsforums)
endif(RS_GXSFORUMS)
add_subdirectory(help)
if(RS_GXSIDENTITIES)
	add_subdirectory(Identity)
endif()
add_subdirectory(images)
add_subdirectory(im_history)
add_subdirectory(msgs)
add_subdirectory(NetworkDialog)
if(RS_GXSCIRCLES)
	add_subdirectory(People)
endif(RS_GXSCIRCLES)
if(RS_GXSPHOTOSHARE)
	add_subdirectory(PhotoShare)
endif(RS_GXSPHOTOSHARE)
add_subdirectory(plugins)
if(RS_GXSPOSTED)
	add_subdirectory(Posted)
endif(RS_GXSPOSTED)
add_subdirectory(profile)
add_subdirectory(qss)
add_subdirectory(settings)
add_subdirectory(statistics)
add_subdirectory(statusbar)
add_subdirectory(style)
if(RS_GXSTHEWIRE)
	add_subdirectory(TheWire)
endif(RS_GXSTHEWIRE)
add_subdirectory(toaster)
if(RS_UNFINISHED)
	add_subdirectory(unfinished)
endif(RS_UNFINISHED)
if(RS_GXSWIKIPOS)
	add_subdirectory(WikiPoos)
endif(RS_GXSWIKIPOS)

target_sources(${PROJECT_NAME}
	${PUBLIC_FILESET_HEADERS_FILES}

	${PRIVATE_FILESET_private_HEADERS_FILES}
	ChatLobbyWidget.h
	QuickStartWizard.h
	RsGUIEventManager.h
	GetStartedDialog.h
	StartDialog.h
	HomePage.h
	NetworkDialog.h
	GenCertDialog.h
	linetypes.h
	mainpagestack.h
	MainWindow.h
	RSHumanReadableDelegate.h
	AboutDialog.h
	AboutWidget.h
	NetworkView.h
	FriendsDialog.h
	ServicePermissionDialog.h
	RemoteDirModel.h
	RetroShareLink.h
	SearchTreeWidget.h
	ShareManager.h
	NewsFeed.h
	SoundManager.h
	HelpDialog.h
	LogoBar.h

	${PRIVATE_FILESET_SOURCES_FILES}
	AboutDialog.cpp
	AboutWidget.cpp
	ChatLobbyWidget.cpp
	FriendsDialog.cpp
	GenCertDialog.cpp
	GetStartedDialog.cpp
	HelpDialog.cpp
	HomePage.cpp
	LogoBar.cpp
	MainPage.cpp
	mainpagestack.cpp
	MainWindow.cpp
	NetworkDialog.cpp
	NetworkView.cpp
	NewsFeed.cpp
	QuickStartWizard.cpp
	RemoteDirModel.cpp
	RetroShareLink.cpp
	RsAutoUpdatePage.cpp
	RsGUIEventManager.cpp
	SearchTreeWidget.cpp
	ServicePermissionDialog.cpp
	#	ShareDialog.cpp
	ShareManager.cpp
	SoundManager.cpp
	StartDialog.cpp

	${PRIVATE_FILESET_UI_FILES}
	AboutDialog.ui
	AboutWidget.ui
	ChatLobbyWidget.ui
	FriendsDialog.ui
	GenCertDialog.ui
	GetStartedDialog.ui
	HelpDialog.ui
	HomePage.ui
	MainWindow.ui
	NetworkDialog.ui
	NetworkView.ui
	NewsFeed.ui
	QuickStartWizard.ui
	ServicePermissionDialog.ui
	ShareManager.ui
	StartDialog.ui

	${PRIVATE_FILESET_RESOURCES_FILES}
	emojione.qrc
	icons.qrc
	images.qrc
)

if(RS_MESSENGER)
	target_sources(${PROJECT_NAME}
		${PRIVATE_FILESET_private_HEADERS_FILES}
		MessengerWindow.h

		${PRIVATE_FILESET_SOURCES_FILES}
		MessengerWindow.cc

		${PRIVATE_FILESET_UI_FILES}
		MessengerWindow.ui
	)
endif(RS_MESSENGER)

# Embedded Friend Server
if(RS_EFS)
	target_sources(${PROJECT_NAME}
		${PRIVATE_FILESET_private_HEADERS_FILES}
		FriendServerControl.h

		${PRIVATE_FILESET_SOURCES_FILES}
		FriendServerControl.cc

		${PRIVATE_FILESET_UI_FILES}
		FriendServerControl.ui
	)
	target_compile_definitions(${PROJECT_NAME} PUBLIC RS_EMBEDED_FRIEND_SERVER)
endif(RS_EFS)
