/*******************************************************************************
 * retroshare-gui/src/: rsguiversion.h                                         *
 *                                                                             *
 * RetroShare GUI                                                              *
 *                                                                             *
 * Copyright (C) 2026  Retroshare Team <contact@retroshare.cc>                 *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#pragma once

/**
 * @file rsguiversion.h
 *
 * RetroShare GUI own version, independent from libretroshare.
 *
 * @def RS_GUI_VERSION
 * Human readable version string of the RetroShare GUI, injected at compile time
 * by the build system (both qmake and CMake) from `git describe` of the
 * RetroShare super-project, i.e. the repository that hosts retroshare-gui.
 *
 * This is the GUI application's OWN version, distinct from the core engine
 * version exposed by libretroshare (RS_HUMAN_READABLE_VERSION in
 * retroshare/rsversion.h, reported to the GUI through RsInit::libRetroShareVersion()).
 * The GUI displays its own version as "the RetroShare version" and lists
 * libretroshare's version among the other libraries, like any dependency.
 *
 * A default is provided so builds without version injection (e.g. a source
 * tarball without git metadata, or a reduced build) still compile.
 */
#ifndef RS_GUI_VERSION
#	define RS_GUI_VERSION "version not available"
#endif

/** Human readable string describing the RetroShare GUI version */
constexpr auto RS_GUI_HUMAN_READABLE_VERSION = RS_GUI_VERSION;
