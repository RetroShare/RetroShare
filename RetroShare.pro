!include("retroshare.pri"): error("Could not include file retroshare.pri")

TEMPLATE = subdirs
#CONFIG += tests

SUBDIRS += \
        openpgpsdk \
        libbitdht \
        libretroshare \
        libresapi \
        retroshare_gui \
        retroshare_nogui \
        plugins

openpgpsdk.file = openpgpsdk/src/openpgpsdk.pro

libbitdht.file = libbitdht/src/libbitdht.pro

libretroshare.file = libretroshare/src/libretroshare.pro
libretroshare.depends = openpgpsdk libbitdht

libresapi.file = libresapi/src/libresapi.pro
libresapi.depends = libretroshare

retroshare_gui.file = retroshare-gui/src/retroshare-gui.pro
retroshare_gui.depends = libretroshare libresapi
retroshare_gui.target = retroshare-gui

retroshare_nogui.file = retroshare-nogui/src/retroshare-nogui.pro
retroshare_nogui.depends = libretroshare libresapi
retroshare_nogui.target = retroshare-nogui

plugins.file = plugins/plugins.pro
plugins.depends = retroshare_gui
plugins.target = plugins

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
