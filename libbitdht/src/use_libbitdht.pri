# RetroShare main qmake build script
#
# Copyright (C) 2004-2019, Retroshare Team <contact@retroshare.cc>
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the
# Free Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.
# See the GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <https://www.gnu.org/licenses/>.
#
# SPDX-FileCopyrightText: Retroshare Team <contact@retroshare.cc>
# SPDX-License-Identifier: LGPL-3.0-or-later

DEPENDPATH *= $$system_path($$clean_path($${PWD}/../../libbitdht/src))
INCLUDEPATH  *= $$system_path($$clean_path($${PWD}/../../libbitdht/src))
LIBS *= -L$$system_path($$clean_path($${OUT_PWD}/../../libbitdht/src/lib/)) -lbitdht

!equals(TARGET, bitdht):PRE_TARGETDEPS *= $$system_path($$clean_path($${OUT_PWD}/../../libbitdht/src/lib/libbitdht.a))
