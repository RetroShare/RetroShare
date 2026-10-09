# ------------------------------------------------------------------------ *\
# src/gui/connect/CMakeLists.txt
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
	ConfCertDialog.h
	ConnectFriendWizard.h
	ConnectProgressDialog.h
	FriendRecommendDialog.h
	PGPKeyDialog.h

	${PRIVATE_FILESET_SOURCES_FILES}
	ConfCertDialog.cpp
	PGPKeyDialog.cpp
	ConnectFriendWizard.cpp
	ConnectProgressDialog.cpp
	FriendRecommendDialog.cpp

	${PRIVATE_FILESET_UI_FILES}
	ConfCertDialog.ui
	ConnectFriendWizard.ui
	ConnectProgressDialog.ui
	FriendRecommendDialog.ui
	PGPKeyDialog.ui

	${PRIVATE_FILESET_RESOURCES_FILES}
)
