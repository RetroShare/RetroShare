TEMPLATE = lib
CONFIG += staticlib
DESTDIR = lib

HEADERS	= pdn.h \
	    checkers.h echeckers.h rcheckers.h \
	    field.h toplevel.h view.h history.h board.h \
	    newgamedlg.h \
	    common.h \
	    player.h humanplayer.h computerplayer.h
	   

SOURCES	= pdn.cc \
	    checkers.cc echeckers.cc rcheckers.cc \
	    field.cc toplevel.cc view.cc history.cc board.cc \
	    newgamedlg.cc \
	    humanplayer.cc computerplayer.cc

RESOURCES = qcheckers.qrc


TARGET		= qcheckers
#PREFIX		= $$system(grep 'define PREFIX' common.h | cut -d'"' -f2)
#SHARE_PATH	= $$system(grep 'define SHARE_PATH' common.h | cut -d'"' -f2)

TRANSLATIONS	= i18n/kcheckers_de.ts i18n/kcheckers_fr.ts
#		i18n/kcheckers_ru.ts

target.path	= $$PREFIX/bin
INSTALLS	+= target


#
# This hack is needed for i18n support.
#
share.path	+= $$PREFIX/share/kcheckers
share.files	+= kcheckers.pdn COPYING AUTHORS ChangeLog README themes i18n/*
INSTALLS	+= share

