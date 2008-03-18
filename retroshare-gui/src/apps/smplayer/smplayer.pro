TEMPLATE = lib
CONFIG += staticlib
DESTDIR = lib
#TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= release
#CONFIG	+= qt warn_on release

QT += network
#QT +=  opengl 

RESOURCES = icons.qrc

HEADERS	+= config.h \
	constants.h \
	svn_revision.h \
	version.h \
	global.h \
	helper.h \
	translator.h \
	subtracks.h \
	trackdata.h \
	tracks.h \
	extensions.h \
	desktopinfo.h \
	myprocess.h \
	mplayerversion.h \
	mplayerprocess.h \
	mplayerwindow.h \
	mediadata.h \
	mediasettings.h \
	preferences.h \
	images.h \
	inforeader.h \
	recents.h \
	core.h \
	logwindow.h \
	infofile.h \
	encodings.h \
	seekwidget.h \
	mytablewidget.h \
	shortcutgetter.h \
	actionseditor.h \
	preferencesdialog.h \
	mycombobox.h \
	tristatecombo.h \
	prefwidget.h \
	prefgeneral.h \
	prefdrives.h \
	prefinterface.h \
	prefperformance.h \
	prefinput.h \
	prefsubtitles.h \
	prefadvanced.h \
	filepropertiesdialog.h \
	playlist.h \
	playlistdock.h \
	verticaltext.h \
	eqslider.h \
	videoequalizer.h \
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
	basegui.h \
	baseguiplus.h \
	floatingwidget.h \
	widgetactions.h \
	defaultgui.h \
	minigui.h \
	smplayer.h \
	clhelp.h


SOURCES	+= version.cpp \
	global.cpp \
	helper.cpp \
	translator.cpp \
	subtracks.cpp \
	trackdata.cpp \
	tracks.cpp \
	extensions.cpp \
	desktopinfo.cpp \
	myprocess.cpp \
	mplayerversion.cpp \
	mplayerprocess.cpp \
	mplayerwindow.cpp \
	mediadata.cpp \
	mediasettings.cpp \
	preferences.cpp \
	images.cpp \
	inforeader.cpp \
	recents.cpp \
	core.cpp \
	logwindow.cpp \
	infofile.cpp \
	encodings.cpp \
	seekwidget.cpp \
	mytablewidget.cpp \
	shortcutgetter.cpp \
	actionseditor.cpp \
	preferencesdialog.cpp \
	mycombobox.cpp \
	tristatecombo.cpp \
	prefwidget.cpp \
	prefgeneral.cpp \
	prefdrives.cpp \
	prefinterface.cpp \
	prefperformance.cpp \
	prefinput.cpp \
	prefsubtitles.cpp \
	prefadvanced.cpp \
	filepropertiesdialog.cpp \
	playlist.cpp \
	playlistdock.cpp \
	verticaltext.cpp \
	eqslider.cpp \
	videoequalizer.cpp \
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
	basegui.cpp \
	baseguiplus.cpp \
	floatingwidget.cpp \
	widgetactions.cpp \
	defaultgui.cpp \
	minigui.cpp \
	clhelp.cpp \
	smplayer.cpp \
	main.cpp

FORMS = inputdvddirectory.ui logwindowbase.ui filepropertiesdialog.ui \
        eqslider.ui seekwidget.ui inputurl.ui \
        preferencesdialog.ui prefgeneral.ui prefdrives.ui prefinterface.ui \
        prefperformance.ui prefinput.ui prefsubtitles.ui prefadvanced.ui \
        about.ui inputmplayerversion.ui errordialog.ui

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
               translations/smplayer_mk.ts translations/smplayer_eu.ts

unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj

  DEFINES += DATA_PATH=$(DATA_PATH)
  DEFINES += DOC_PATH=$(DOC_PATH)
  DEFINES += TRANSLATION_PATH=$(TRANSLATION_PATH)
  DEFINES += CONF_PATH=$(CONF_PATH)
  DEFINES += THEMES_PATH=$(THEMES_PATH)
  DEFINES += SHORTCUTS_PATH=$(SHORTCUTS_PATH)
  #DEFINES += NO_DEBUG_ON_CONSOLE

  #DEFINES += KDE_SUPPORT
  #INCLUDEPATH += /opt/kde3/include/
  #LIBS += -lkio -L/opt/kde3/lib/

  #contains( DEFINES, KDE_SUPPORT) {
  #	HEADERS += mysystemtrayicon.h
  #	SOURCES += mysystemtrayicon.cpp
  #}

  #HEADERS += 	prefassociations.h winfileassoc.h
  #SOURCES += 	prefassociations.cpp winfileassoc.cpp
  #FORMS += prefassociations.ui
}

win32 {
	HEADERS += 	prefassociations.h winfileassoc.h screensaver.h
	SOURCES += 	prefassociations.cpp winfileassoc.cpp screensaver.cpp
	FORMS += prefassociations.ui

	LIBS += libole32
	
	RC_FILE = smplayer.rc
	DEFINES += NO_DEBUG_ON_CONSOLE
#	debug {
#		CONFIG += console
#	}
}

