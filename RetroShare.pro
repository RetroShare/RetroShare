TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS += \
        openpgpsdk/src/openpgpsdk.pro \
        supportlibs/pegmarkdown/pegmarkdown.pro \
        libbitdht/src/libbitdht.pro \
        libretroshare/src/libretroshare.pro \
		libresapi/src/libresapi.pro	\
        retroshare-gui/src/retroshare-gui.pro \
        retroshare-nogui/src/retroshare-nogui.pro \
        plugins/plugins.pro
