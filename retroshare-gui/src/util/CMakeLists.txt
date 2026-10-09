# ------------------------------------------------------------------------ *\
# src/util/CMakeLists.txt
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
	AspectRatioPixmapLabel.h
	ClickableLabel.h
	DateTime.h
	dllexport.h
	dllexport.h
	EventFilter.h
	FontSizeHandler.h
	global.h
	HandleRichText.h
	imageutil.h
	log.h
	misc.h
	MouseEventFilter.h
	NonCopyable.h
	ObjectPainter.h
	PixmapMerging.h
	printpreview.h
	qtthreadsutils.h
	retroshareWin32.h
	RetroStyleLabel.h
	RichTextEdit.h
	RsAction.h
	RsFile.h
	RsNetUtil.h
	RsProtectedTimer.h
	rsqtutildll.h
	RsQtVersion.h
	RsSyntaxHighlighter.h
	RsUserdata.h
	rsutildll.h
	stringutil.h
	Widget.h

	${PRIVATE_FILESET_SOURCES_FILES}
	AspectRatioPixmapLabel.cpp
	ClickableLabel.cpp
	DateTime.cpp
	EventFilter.cpp
	FontSizeHandler.cpp
	HandleRichText.cpp
	imageutil.cpp
	log.cpp
	misc.cpp
	MouseEventFilter.cpp
	ObjectPainter.cpp
	PixmapMerging.cpp
	printpreview.cpp
	retroshareWin32.cpp
	RetroStyleLabel.cpp
	RichTextEdit.cpp
	RsAction.cpp
	RsFile.cpp
	RsNetUtil.cpp
	RsProtectedTimer.cpp
	RsSyntaxHighlighter.cpp
	stringutil.cpp
	WidgetBackgroundImage.cpp
	Widget.cpp

	${PRIVATE_FILESET_UI_FILES}
	RichTextEdit.ui

	${PRIVATE_FILESET_RESOURCES_FILES}
)

if(RS_GXSGUI)
	target_sources(${PROJECT_NAME}
		${PUBLIC_FILESET_HEADERS_FILES}

		${PRIVATE_FILESET_private_HEADERS_FILES}
		TokenQueue.h
		RsGxsUpdateBroadcast.h

		${PRIVATE_FILESET_SOURCES_FILES}
		TokenQueue.cpp
		RsGxsUpdateBroadcast.cpp

		${PRIVATE_FILESET_UI_FILES}

		${PRIVATE_FILESET_RESOURCES_FILES}
	)
endif(RS_GXSGUI)

if(RS_FRAMECATCHER)
	target_sources(${PROJECT_NAME}
		${PUBLIC_FILESET_HEADERS_FILES}

		${PRIVATE_FILESET_private_HEADERS_FILES}
		framecatcher.h

		${PRIVATE_FILESET_SOURCES_FILES}
		framecatcher.cpp

		${PRIVATE_FILESET_UI_FILES}

		${PRIVATE_FILESET_RESOURCES_FILES}
	)
	target_compile_definitions(${PROJECT_NAME} PUBLIC CHANNELS_FRAME_CATCHER)
endif(RS_FRAMECATCHER)
