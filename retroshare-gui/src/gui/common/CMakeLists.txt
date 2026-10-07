# ------------------------------------------------------------------------ *\
# src/gui/common/CMakeLists.txt
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
	AvatarDefs.h
	AvatarDialog.h
	AvatarWidget.h
	DropLineEdit.h
	ElidedLabel.h
	Emoticons.h
	FeedNotify.h
	FilesDefs.h
	FloatingHelpBrowser.h
	FlowLayout.h
	FriendListModel.h
	FriendSelectionDialog.h
	FriendSelectionWidget.h
	GroupChooser.h
	GroupDefs.h
	GroupFlagsWidget.h
	GroupSelectionBox.h
	GroupTreeWidget.h
	HashBox.h
	HeaderFrame.h
	LineEditClear.h
	MimeTextEdit.h
	NewFriendList.h
	PeerDefs.h
	PictureFlow.h
	PopularityDefs.h
	RsBanListDefs.h
	RsBanListToolButton.h
	RsButtonOnText.h
	RsCollectionDialog.h
	RsCollection.h
	RsCollectionModel.h
	RSComboBox.h
	RSElidedItemDelegate.h
	RSFeedWidget.h
	RSGraphWidget.h
	rshtml.h
	RSImageBlockWidget.h
	RSItemDelegate.h
	RSListWidgetItem.h
	RSPlainTextEdit.h
	RSTabWidget.h
	RSTextBrowser.h
	RSTextEdit.h
	RSTreeView.h
	RSTreeWidget.h
	RSTreeWidgetItem.h
	rwindow.h
	StatusDefs.h
	SubscribeToolButton.h
	TagDefs.h
	ToasterNotify.h
	UIStateHelper.h
	UserNotify.h
	vmessagebox.h

	${PRIVATE_FILESET_SOURCES_FILES}
	RsButtonOnText.cpp
	RSGraphWidget.cpp
	ElidedLabel.cpp
	vmessagebox.cpp
	RsCollectionDialog.cpp
	RsCollectionModel.cpp
	RsCollection.cpp
	RsUrlHandler.cpp
	rwindow.cpp
	rshtml.cpp
	AvatarDefs.cpp
	AvatarDialog.cpp
	GroupFlagsWidget.cpp
	GroupSelectionBox.cpp
	GroupChooser.cpp
	StatusDefs.cpp
	TagDefs.cpp
	GroupDefs.cpp
	Emoticons.cpp
	RSComboBox.cpp
	RSListWidgetItem.cpp
	RSTextEdit.cpp
	RSPlainTextEdit.cpp
	RSTreeWidget.cpp
	RSTreeWidgetItem.cpp
	RSFeedWidget.cpp
	RSTabWidget.cpp
	RSElidedItemDelegate.cpp
	RSItemDelegate.cpp
	PeerDefs.cpp
	FilesDefs.cpp
	PopularityDefs.cpp
	RsBanListDefs.cpp
	GroupTreeWidget.cpp
	RSTreeView.cpp
	AvatarWidget.cpp
	FriendListModel.cpp
	NewFriendList.cpp
	FriendSelectionWidget.cpp
	FriendSelectionDialog.cpp
	HashBox.cpp
	LineEditClear.cpp
	DropLineEdit.cpp
	RSTextBrowser.cpp
	RSImageBlockWidget.cpp
	FeedNotify.cpp
	UserNotify.cpp
	HeaderFrame.cpp
	MimeTextEdit.cpp
	UIStateHelper.cpp
	FloatingHelpBrowser.cpp
	SubscribeToolButton.cpp
	RsBanListToolButton.cpp
	FlowLayout.cpp
	PictureFlow.cpp
	ToasterNotify.cpp

	${PRIVATE_FILESET_UI_FILES}
	AvatarDialog.ui
	AvatarWidget.ui
	FriendSelectionWidget.ui
	GroupTreeWidget.ui
	HashBox.ui
	HeaderFrame.ui
	NewFriendList.ui
	RsCollectionDialog.ui
	RSFeedWidget.ui
	RSImageBlockWidget.ui

	${PRIVATE_FILESET_RESOURCES_FILES}
)

if(RS_MESSENGER)
	target_sources(${PROJECT_NAME}
		PRIVATE FILE_SET private_HEADERS FILES
		src/gui/common/FriendList.h

		${PRIVATE_FILESET_SOURCES_FILES}
		src/gui/common/FriendList.cc

		${PRIVATE_FILESET_UI_FILES}
		src/gui/common/FriendList.ui
	)
endif(RS_MESSENGER)

if(APPLE)
	target_sources(${PROJECT_NAME}
		${PRIVATE_FILESET_SOURCES_FILES}
		MacDockIconHandler.mm
	)
	set_source_files_properties(MacDockIconHandler.mm PROPERTIES
		COMPILE_FLAGS "-x objective-c++"
	)
endif()
