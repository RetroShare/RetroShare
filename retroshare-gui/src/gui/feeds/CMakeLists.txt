# ------------------------------------------------------------------------ *\
# src/gui/feeds/CMakeLists.txt
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
	AttachFileItem.h
	BoardsCommentsItem.h
	ChannelsCommentsItem.h
	ChatMsgItem.h
	FeedHolder.h
	FeedItem.h
	GxsCircleItem.h
	MsgItem.h
	NewsFeedUserNotify.h
	PeerItem.h
	SecurityIpItem.h
	SecurityItem.h
	SubFileItem.h
	TLSItem.h

	${PRIVATE_FILESET_SOURCES_FILES}
	BoardsCommentsItem.cpp
	FeedItem.cpp
	FeedHolder.cpp
	GxsCircleItem.cpp
	ChannelsCommentsItem.cpp
	PeerItem.cpp
	MsgItem.cpp
	ChatMsgItem.cpp
	SubFileItem.cpp
	AttachFileItem.cpp
	TLSItem.cpp
	SecurityItem.cpp
	SecurityIpItem.cpp
	NewsFeedUserNotify.cpp

	${PRIVATE_FILESET_UI_FILES}
	AttachFileItem.ui
	BoardsCommentsItem.ui
	ChannelsCommentsItem.ui
	ChatMsgItem.ui
	GxsCircleItem.ui
	MsgItem.ui
	PeerItem.ui
	SecurityIpItem.ui
	SecurityItem.ui
	SubFileItem.ui
	TLSItem.ui

	${PRIVATE_FILESET_RESOURCES_FILES}
)

if(RS_GXSGUI)
	target_sources(${PROJECT_NAME}
		PUBLIC FILE_SET HEADERS FILES

		PRIVATE FILE_SET private_HEADERS FILES
		GxsGroupFeedItem.h
		GxsFeedItem.h

		${PRIVATE_FILESET_SOURCES_FILES}
		GxsGroupFeedItem.cpp
		GxsFeedItem.cpp

		${PRIVATE_FILESET_UI_FILES}

		${PRIVATE_FILESET_RESOURCES_FILES}
	)
endif(RS_GXSGUI)

if(RS_GXSFORUMS)
	target_sources(${PROJECT_NAME}
		PUBLIC FILE_SET HEADERS FILES

		PRIVATE FILE_SET private_HEADERS FILES
		GxsForumGroupItem.h
		GxsForumMsgItem.h

		${PRIVATE_FILESET_SOURCES_FILES}
		GxsForumGroupItem.cpp
		GxsForumMsgItem.cpp

		${PRIVATE_FILESET_UI_FILES}
		GxsForumGroupItem.ui
		GxsForumMsgItem.ui

		${PRIVATE_FILESET_RESOURCES_FILES}
	)
endif(RS_GXSFORUMS)

if(RS_GXSCHANNELS)
	target_sources(${PROJECT_NAME}
		PUBLIC FILE_SET HEADERS FILES

		PRIVATE FILE_SET private_HEADERS FILES
		GxsChannelGroupItem.h
		GxsChannelPostItem.h

		${PRIVATE_FILESET_SOURCES_FILES}
		GxsChannelGroupItem.cpp
		GxsChannelPostItem.cpp

		${PRIVATE_FILESET_UI_FILES}
		GxsChannelGroupItem.ui
		GxsChannelPostItem.ui

		${PRIVATE_FILESET_RESOURCES_FILES}
	)
endif(RS_GXSCHANNELS)

if(RS_GXSPOSTED)
	target_sources(${PROJECT_NAME}
		PUBLIC FILE_SET HEADERS FILES

		PRIVATE FILE_SET private_HEADERS FILES
		PostedGroupItem.h
		BoardsPostItem.h

		${PRIVATE_FILESET_SOURCES_FILES}
		PostedGroupItem.cpp
		BoardsPostItem.cpp

		${PRIVATE_FILESET_UI_FILES}
		PostedGroupItem.ui
		BoardsPostItem.ui

		${PRIVATE_FILESET_RESOURCES_FILES}
	)
endif(RS_GXSPOSTED)

if(RS_GXSTHEWIRE)
	target_sources(${PROJECT_NAME}
		PUBLIC FILE_SET HEADERS FILES

		PRIVATE FILE_SET private_HEADERS FILES
		WireNotifyGroupItem.h
		WireNotifyPostItem.h

		${PRIVATE_FILESET_SOURCES_FILES}
		WireNotifyGroupItem.cpp
		WireNotifyPostItem.cpp

		${PRIVATE_FILESET_UI_FILES}
		WireNotifyGroupItem.ui
		WireNotifyPostItem.ui

		${PRIVATE_FILESET_RESOURCES_FILES}
	)
endif(RS_GXSTHEWIRE)
