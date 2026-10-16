## ------------------------------------------------------------------------- ##
 # mk/cmake/fetch_provider_packages/FetchRetroShare.cmake
 # This file is part of RetroShare-FriendServer.
 #
 # Copyright (C) 2026      David Bears <dbear4q@gmail.com>
 #
 # SPDX-License-Identifier: AGPL-3.0-or-later
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
## ------------------------------------------------------------------------- ##

# 3.11: FetchContent
# 3.14: FetchContent_MakeAvailable()
cmake_minimum_required(VERSION 3.14...4.4)

set(FETCH_RetroShare_GIT_TAG "" CACHE STRING
	"git tag to use for fetching RetroShare"
)
mark_as_advanced(FETCH_RetroShare_GIT_TAG)
if(NOT FETCH_RetroShare_GIT_TAG)
	set(FETCH_RetroShare_GIT_TAG origin/master)
endif()

include(FetchContent)
FetchContent_Declare(RetroShare
	GIT_REPOSITORY "https://github.com/RetroShare/libretroshare.git"
	GIT_TAG ${FETCH_RetroShare_GIT_TAG}
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(RetroShare)
set(RetroShare_FOUND TRUE)
