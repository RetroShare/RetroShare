# RetroShare main qmake build script
#
# Copyright (C) 2004-2019, Retroshare Team <contact@retroshare.cc>
# Copyright (C) 2016-2019, Gioacchino Mazzurco <gio@eigenlab.org>
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

CONFIG += c++14

!include("retroshare.pri"): error("Could not include file retroshare.pri")

TEMPLATE = subdirs

SUBDIRS += openpgpsdk
openpgpsdk.file = openpgpsdk/src/openpgpsdk.pro

rs_jsonapi:isEmpty(JSONAPI_GENERATOR_EXE) {
    SUBDIRS += jsonapi-generator
    jsonapi-generator.file = jsonapi-generator/src/jsonapi-generator.pro
    libretroshare.depends += jsonapi-generator
}

SUBDIRS += libbitdht
libbitdht.file = libbitdht/src/libbitdht.pro
libretroshare.depends += openpgpsdk libbitdht

SUBDIRS += libretroshare
libretroshare.file = libretroshare/src/libretroshare.pro

retroshare_gui {
    SUBDIRS += retroshare_gui
    retroshare_gui.file = retroshare-gui/src/retroshare-gui.pro
    retroshare_gui.target = retroshare_gui
    retroshare_gui.depends = libretroshare
}

retroshare_service {
    SUBDIRS += retroshare_service
    retroshare_service.file = retroshare-service/src/retroshare-service.pro
    retroshare_service.depends = libretroshare
    retroshare_service.target = retroshare_service
}

retroshare_friendserver {
    SUBDIRS += retroshare_friendserver
    retroshare_friendserver.file = retroshare-friendserver/src/retroshare-friendserver.pro
    retroshare_friendserver.depends = libretroshare
    retroshare_friendserver.target = retroshare_friendserver
}
retroshare_plugins {
    SUBDIRS += plugins
    plugins.file = plugins/plugins.pro
    plugins.depends = retroshare_gui
    plugins.target = plugins
}

rs_webui {
    SUBDIRS += retroshare-webui
    retroshare-webui.file = retroshare-webui/webui.pro
    retroshare-webui.target = rs_webui
    retroshare-webui.depends = retroshare_gui
}

wikipoos {
    SUBDIRS += pegmarkdown
    pegmarkdown.file = supportlibs/pegmarkdown/pegmarkdown.pro
    retroshare_gui.depends += pegmarkdown
}

tests {
    SUBDIRS += librssimulator
    librssimulator.file = tests/librssimulator/librssimulator.pro

    SUBDIRS += unittests
    unittests.file = tests/unittests/unittests.pro
    unittests.depends = libretroshare librssimulator
    unittests.target = unittests
}
