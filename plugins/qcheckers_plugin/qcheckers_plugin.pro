#=== this part is common (similar) for all plugin projects =====================
TEMPLATE      = lib
CONFIG       += plugin release

# this is directory, where PluginInterface.h is located
INCLUDEPATH  += ../

# and, the result (*.so or *.dll) should appear in this directory
DESTDIR       = ../bin
OBJECTS_DIR = temp/obj
RCC_DIR = temp/qrc
UI_DIR  = temp/ui
MOC_DIR = temp/moc


# the name of the result file; 
TARGET        = $$qtLibraryTarget(qcheckers_plugin)

HEADERS     += ../PluginInterface.h  \
               QCheckersPlugin.h
SOURCES     += QCheckersPlugin.cpp
                
#===============================================================================

HEADERS	+= pdn.h \
	    checkers.h echeckers.h rcheckers.h \
	    field.h toplevel.h view.h history.h board.h \
	    newgamedlg.h \
	    common.h \
	    player.h humanplayer.h computerplayer.h
	   

SOURCES	+= pdn.cc \
	    checkers.cc echeckers.cc rcheckers.cc \
	    field.cc toplevel.cc view.cc history.cc board.cc \
	    newgamedlg.cc \
	    humanplayer.cc computerplayer.cc

RESOURCES = qcheckers.qrc


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

