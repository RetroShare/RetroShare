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

unix {
	isEmpty(PREFIX)   { PREFIX = /usr }
	isEmpty(INC_DIR)  { INC_DIR = "$${PREFIX}/include/retroshare06" }
	isEmpty(LIB_DIR)  { LIB_DIR = "$${PREFIX}/lib" }
	isEmpty(DATA_DIR) { DATA_DIR = "$${PREFIX}/share/RetroShare06" }

	icon_24_files.path = "$${PREFIX}/share/icons/hicolor/24x24/apps"
	icon_24_files.files = build_scripts/Debian+Ubuntu/data/24x24/*
	INSTALLS += icon_24_files

	icon_48_files.path = "$${PREFIX}/share/icons/hicolor/48x48/apps"
	icon_48_files.files = build_scripts/Debian+Ubuntu/data/48x48/*
	INSTALLS += icon_48_files

	icon_64_files.path = "$${PREFIX}/share/icons/hicolor/64x64/apps"
	icon_64_files.files = build_scripts/Debian+Ubuntu/data/64x64/*
	INSTALLS += icon_64_files

	desktop_files.path = "$${PREFIX}/share/applications"
	desktop_files.files = build_scripts/Debian+Ubuntu/retroshare.desktop
	INSTALLS += desktop_files

	pixmap_files.path = "$${PREFIX}/share/pixmaps"
	pixmap_files.files = build_scripts/Debian+Ubuntu/retroshare.xpm
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
