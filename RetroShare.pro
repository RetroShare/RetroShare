################################################################################
# Retroshare.pro                                                               #
# Copyright (C) 2018, Retroshare team <retroshare.team@gmailcom>               #
#                                                                              #
# This program is free software: you can redistribute it and/or modify         #
# it under the terms of the GNU Affero General Public License as               #
# published by the Free Software Foundation, either version 3 of the           #
# License, or (at your option) any later version.                              #
#                                                                              #
# This program is distributed in the hope that it will be useful,              #
# but WITHOUT ANY WARRANTY; without even the implied warranty of               #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                #
# GNU Affero General Public License for more details.                          #
#                                                                              #
# You should have received a copy of the GNU Affero General Public License     #
# along with this program.  If not, see <https://www.gnu.org/licenses/>.       #
################################################################################

!include("retroshare.pri"): error("Could not include file retroshare.pri")
TEMPLATE = subdirs
VERSION = 0.6.5.0
SUBDIRS += openpgpsdk
openpgpsdk.file = openpgpsdk/src/openpgpsdk.pro

rs_jsonapi {
    SUBDIRS += jsonapi-generator
    jsonapi-generator.file = jsonapi-generator/src/jsonapi-generator.pro
    libretroshare.depends += jsonapi-generator
}

SUBDIRS += libbitdht
libbitdht.file = libbitdht/src/libbitdht.pro
libretroshare.depends = openpgpsdk libbitdht

SUBDIRS += libretroshare
libretroshare.file = libretroshare/src/libretroshare.pro

SUBDIRS += libresapi
libresapi.file = libresapi/src/libresapi.pro
libresapi.depends = libretroshare

retroshare_gui {
    SUBDIRS += retroshare_gui
    retroshare_gui.file = retroshare-gui/src/retroshare-gui.pro
    retroshare_gui.depends = libretroshare libresapi
    retroshare_gui.target = retroshare_gui
}

retroshare_nogui {
    SUBDIRS += retroshare_nogui
    retroshare_nogui.file = retroshare-nogui/src/retroshare-nogui.pro
    retroshare_nogui.depends = libretroshare libresapi
    retroshare_nogui.target = retroshare_nogui
}

retroshare_android_service {
    SUBDIRS += retroshare_android_service
    retroshare_android_service.file = retroshare-android-service/src/retroshare-android-service.pro
    retroshare_android_service.depends = libresapi
    retroshare_android_service.target = retroshare_android_service
}

retroshare_android_notify_service {
    SUBDIRS += retroshare_android_notify_service
    retroshare_android_notify_service.file = retroshare-android-notify-service/src/retroshare-android-notify-service.pro
    retroshare_android_notify_service.depends = retroshare_android_service
    retroshare_android_notify_service.target = retroshare_android_notify_service
}

retroshare_qml_app {
    SUBDIRS += retroshare_qml_app
    retroshare_qml_app.file = retroshare-qml-app/src/retroshare-qml-app.pro
    retroshare_qml_app.depends = retroshare_android_service
    retroshare_qml_app.target = retroshare_qml_app

    android-g++ {
        retroshare_qml_app.depends += retroshare_android_notify_service
    }
}

retroshare_plugins {
    SUBDIRS += plugins
    plugins.file = plugins/plugins.pro
    plugins.depends = retroshare_gui
    plugins.target = plugins
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
