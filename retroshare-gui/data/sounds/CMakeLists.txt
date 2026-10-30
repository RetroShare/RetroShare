## ------------------------------------------------------------------------- ##
 # src/data/sounds/CMakeLists.txt
 # This file is part of RetroShare-GUI.
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

cmake_minimum_required(VERSION 3.0...4.4)

include(GNUInstallDirs)

set(sounds
	alert.wav
	chat1.wav
	chat2.wav
	file.wav
	ft_complete.wav
	ft_incoming.wav
	incomingchat.wav
	notify.wav
	offline.wav
	online1.wav
	online2.wav
	receive.wav
	send1.wav
	send2.wav
	startup.wav
)

install(FILES ${sounds}
	DESTINATION ${RS_INSTALL_DATADIR}/sounds
	COMPONENT retroshare-gui
)
