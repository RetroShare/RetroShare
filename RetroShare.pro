!include("retroshare.pri"): error("Could not include file retroshare.pri")

TEMPLATE = subdirs

SUBDIRS += \
        openpgpsdk \
        libbitdht \
        libretroshare \
        libresapi \
        pegmarkdown \
        retroshare_gui \
        retroshare_nogui \
        plugins

openpgpsdk.file = openpgpsdk/src/openpgpsdk.pro

libbitdht.file = libbitdht/src/libbitdht.pro

libretroshare.file = libretroshare/src/libretroshare.pro
libretroshare.depends = openpgpsdk libbitdht

libresapi.file = libresapi/src/libresapi.pro
libresapi.depends = libretroshare

pegmarkdown.file = supportlibs/pegmarkdown/pegmarkdown.pro

retroshare_gui.file = retroshare-gui/src/retroshare-gui.pro
retroshare_gui.depends = libretroshare libresapi pegmarkdown

retroshare_nogui.file = retroshare-nogui/src/retroshare-nogui.pro
retroshare_nogui.depends = libretroshare libresapi

plugins.file = plugins/plugins.pro
plugins.depends = retroshare_gui

unix {
	icon_files.path = "$${PREFIX}/share/icons/hicolor"
	icon_files.files = data/24x24
	icon_files.files += data/48x48
	icon_files.files += data/64x64
	icon_files.files += data/128x128
	INSTALLS += icon_files

	desktop_files.path = "$${PREFIX}/share/applications"
	desktop_files.files = data/retroshare06.desktop
	INSTALLS += desktop_files

	pixmap_files.path = "$${PREFIX}/share/pixmaps"
	pixmap_files.files = data/retroshare06.xpm
	INSTALLS += pixmap_files

	data_files.path = "$${DATA_DIR}"
	data_files.files = libbitdht/src/bitdht/bdboot.txt
	INSTALLS += data_files

	webui_files.path = "$${DATA_DIR}/webui"
	webui_files.files = libresapi/src/webfiles/*
	INSTALLS += webui_files

	webui_img_files.path = "$${DATA_DIR}/webui/img"
	webui_img_files.files = retroshare-gui/src/gui/images/logo/logo_splash.png
	INSTALLS += webui_img_files
}
