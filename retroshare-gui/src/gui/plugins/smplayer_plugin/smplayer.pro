#=== this part is common (similar) for all plugin projects =====================
TEMPLATE      = lib
CONFIG       += plugin debug

# this is directory, where PluginInterface.h is located
INCLUDEPATH  += ../

# and, the result (*.so or *.dll) should appear in this directory
DESTDIR       = ../bin
OBJECTS_DIR = temp/obj
RCC_DIR = temp/qrc
UI_DIR  = temp/ui
MOC_DIR = temp/moc


# the name of the result file; 
TARGET        = $$qtLibraryTarget(smplayer_plugin)

HEADERS     += ../PluginInterface.h  \
               SMPlayerPlugin.h
SOURCES     += SMPlayerPlugin.cpp
                
#===============================================================================
QT += network xml


RESOURCES = icons.qrc

INCLUDEPATH += findsubtitles videopreview mpcgui
DEPENDPATH += findsubtitles videopreview mpcgui

#DEFINES += USE_QXT

DEFINES += DOWNLOAD_SUBS

HEADERS += guiconfig.h \
	config.h \
	constants.h \
	version.h \
	global.h \
	paths.h \
	helper.h \
	colorutils.h \
	translator.h \
	subtracks.h \
	tracks.h \
	titletracks.h \
	extensions.h \
	desktopinfo.h \
	myprocess.h \
	mplayerversion.h \
	mplayerprocess.h \
	infoprovider.h \
	mplayerwindow.h \
	mediadata.h \
	audioequalizerlist.h \
	mediasettings.h \
	assstyles.h \
	preferences.h \
	filesettingsbase.h \
	filesettings.h \
	filesettingshash.cpp \
	images.h \
	inforeader.h \
	deviceinfo.h \
	recents.h \
	urlhistory.h \
	core.h \
	logwindow.h \
	infofile.h \
	seekwidget.h \
	mytablewidget.h \
	shortcutgetter.h \
	actionseditor.h \
	filechooser.h \
	preferencesdialog.h \
	mycombobox.h \
	tristatecombo.h \
	languages.h \
	selectcolorbutton.h \
	prefwidget.h \
	prefgeneral.h \
	prefdrives.h \
	prefinterface.h \
	prefperformance.h \
	prefinput.h \
	prefsubtitles.h \
	prefadvanced.h \
	prefplaylist.h \
	filepropertiesdialog.h \
	playlist.h \
	playlistpreferences.h \
	playlistdock.h \
	verticaltext.h \
	eqslider.h \
	videoequalizer.h \
	audioequalizer.h \
	myslider.h \
	timeslider.h \
	inputdvddirectory.h \
	inputurl.h \
	myaction.h \
	myactiongroup.h \
	myserver.h \
	myclient.h \
	filedialog.h \
	inputmplayerversion.h \
	about.h \
	errordialog.h \
	timedialog.h \
	findsubtitles/simplehttp.h \
	findsubtitles/osparser.h \
	findsubtitles/findsubtitleswindow.h \
	videopreview/videopreview.h \
	videopreview/videopreviewconfigdialog.h \
	basegui.h \
	baseguiplus.h \
	floatingwidget.h \
	widgetactions.h \
	toolbareditor.h \
	defaultgui.h \
	minigui.h \
	mpcgui/mpcgui.h \
	mpcgui/mpcstyles.h \
	smplayer.h \
	clhelp.h


SOURCES	+= version.cpp \
	global.cpp \
	paths.cpp \
	helper.cpp \
	colorutils.cpp \
	translator.cpp \
	subtracks.cpp \
	tracks.cpp \
	titletracks.cpp \
	extensions.cpp \
	desktopinfo.cpp \
	myprocess.cpp \
	mplayerversion.cpp \
	mplayerprocess.cpp \
	infoprovider.cpp \
	mplayerwindow.cpp \
	mediadata.cpp \
	mediasettings.cpp \
	assstyles.cpp \
	preferences.cpp \
	filesettingsbase.cpp \
	filesettings.cpp \
	filesettingshash.cpp \
	images.cpp \
	inforeader.cpp \
	deviceinfo.cpp \
	recents.cpp \
	urlhistory.cpp \
	core.cpp \
	logwindow.cpp \
	infofile.cpp \
	seekwidget.cpp \
	mytablewidget.cpp \
	shortcutgetter.cpp \
	actionseditor.cpp \
	filechooser.cpp \
	preferencesdialog.cpp \
	mycombobox.cpp \
	tristatecombo.cpp \
	languages.cpp \
	selectcolorbutton.cpp \
	prefwidget.cpp \
	prefgeneral.cpp \
	prefdrives.cpp \
	prefinterface.cpp \
	prefperformance.cpp \
	prefinput.cpp \
	prefsubtitles.cpp \
	prefadvanced.cpp \
	prefplaylist.cpp \
	filepropertiesdialog.cpp \
	playlist.cpp \
	playlistpreferences.cpp \
	playlistdock.cpp \
	verticaltext.cpp \
	eqslider.cpp \
	videoequalizer.cpp \
	audioequalizer.cpp \
	myslider.cpp \
	timeslider.cpp \
	inputdvddirectory.cpp \
	inputurl.cpp \
	myaction.cpp \
	myactiongroup.cpp \
	myserver.cpp \
	myclient.cpp \
	filedialog.cpp \
	inputmplayerversion.cpp \
	about.cpp \
	errordialog.cpp \
	timedialog.cpp \
	findsubtitles/simplehttp.cpp \
	findsubtitles/osparser.cpp \
	findsubtitles/findsubtitleswindow.cpp \
	videopreview/videopreview.cpp \
	videopreview/videopreviewconfigdialog.cpp \
	basegui.cpp \
	baseguiplus.cpp \
	floatingwidget.cpp \
	widgetactions.cpp \
	toolbareditor.cpp \
	defaultgui.cpp \
	minigui.cpp \
	mpcgui/mpcgui.cpp \
	mpcgui/mpcstyles.cpp \
	clhelp.cpp \
	smplayer.cpp

#libqxt
contains(DEFINES, USE_QXT) {
	CONFIG  += qxt
	QXT     += core
}

FORMS = inputdvddirectory.ui logwindowbase.ui filepropertiesdialog.ui \
        eqslider.ui seekwidget.ui inputurl.ui \
        preferencesdialog.ui prefgeneral.ui prefdrives.ui prefinterface.ui \
        prefperformance.ui prefinput.ui prefsubtitles.ui prefadvanced.ui \
        prefplaylist.ui \
        about.ui inputmplayerversion.ui errordialog.ui timedialog.ui \
        playlistpreferences.ui filechooser.ui \
        findsubtitles/findsubtitleswindow.ui \
        videopreview/videopreviewconfigdialog.ui

TRANSLATIONS = translations/smplayer_es.ts translations/smplayer_de.ts \
               translations/smplayer_sk.ts translations/smplayer_it.ts \
               translations/smplayer_fr.ts translations/smplayer_zh_CN.ts \
               translations/smplayer_ru_RU.ts translations/smplayer_hu.ts \
               translations/smplayer_en_US.ts translations/smplayer_pl.ts \
               translations/smplayer_ja.ts translations/smplayer_nl.ts \
               translations/smplayer_uk_UA.ts translations/smplayer_pt_BR.ts \
               translations/smplayer_ka.ts translations/smplayer_cs.ts \
               translations/smplayer_bg.ts translations/smplayer_tr.ts \
               translations/smplayer_sv.ts translations/smplayer_sr.ts \
               translations/smplayer_zh_TW.ts translations/smplayer_ro_RO.ts \
               translations/smplayer_pt_PT.ts translations/smplayer_el_GR.ts \
               translations/smplayer_fi.ts translations/smplayer_ko.ts \
               translations/smplayer_mk.ts translations/smplayer_eu.ts \
               translations/smplayer_ca.ts translations/smplayer_sl_SI.ts \
               translations/smplayer_ar_SY.ts translations/smplayer_ku.ts \
               translations/smplayer_gl.ts

contains( DEFINES, DOWNLOAD_SUBS ) {
	INCLUDEPATH += findsubtitles/filedownloader findsubtitles/quazip
	DEPENDPATH += findsubtitles/filedownloader findsubtitles/quazip

	HEADERS += filedownloader.h subchooserdialog.h
	SOURCES += filedownloader.cpp subchooserdialog.cpp

	FORMS += subchooserdialog.ui

	HEADERS += crypt.h \
	           ioapi.h \
	           quazip.h \
	           quazipfile.h \
	           quazipfileinfo.h \
	           quazipnewinfo.h \
	           unzip.h \
	           zip.h

	SOURCES += ioapi.c \
	           quazip.cpp \
	           quazipfile.cpp \
	           quazipnewinfo.cpp \
	           unzip.c \
	           zip.c

	LIBS += -lz
	
	win32 {
		INCLUDEPATH += c:\development\zlib-1.2.3
		LIBS += -Lc:\development\zlib-1.2.3
	}
}

unix {
	UI_DIR = .ui
	MOC_DIR = .moc
	OBJECTS_DIR = .obj

	DEFINES += DATA_PATH=$(DATA_PATH)
	DEFINES += DOC_PATH=$(DOC_PATH)
	DEFINES += TRANSLATION_PATH=$(TRANSLATION_PATH)
	DEFINES += THEMES_PATH=$(THEMES_PATH)
	DEFINES += SHORTCUTS_PATH=$(SHORTCUTS_PATH)
	#DEFINES += NO_DEBUG_ON_CONSOLE

	#DEFINES += KDE_SUPPORT
	#INCLUDEPATH += /opt/kde3/include/
	#LIBS += -lkio -L/opt/kde3/lib/

	#contains( DEFINES, KDE_SUPPORT) {
	# HEADERS += mysystemtrayicon.h
	# SOURCES += mysystemtrayicon.cpp
	#}

	#HEADERS += prefassociations.h winfileassoc.h
	#SOURCES += prefassociations.cpp winfileassoc.cpp
	#FORMS += prefassociations.ui
}

win32 {
	HEADERS += screensaver.h
	SOURCES += screensaver.cpp

	!contains( DEFINES, PORTABLE_APP ) {
		DEFINES += USE_ASSOCIATIONS
	}
	
	contains( DEFINES, USE_ASSOCIATIONS ) {
		HEADERS += prefassociations.h winfileassoc.h
		SOURCES += prefassociations.cpp winfileassoc.cpp
		FORMS += prefassociations.ui
	}

	contains(TEMPLATE,vcapp) {
		LIBS += ole32.lib user32.lib
	} else {
		LIBS += libole32
	}
	
	RC_FILE = smplayer.rc
	DEFINES += NO_DEBUG_ON_CONSOLE
#	debug {
#		CONFIG += console
#	}
}

