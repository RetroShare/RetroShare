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
TARGET        = $$qtLibraryTarget(linkscloud_plugin)

HEADERS     += ../PluginInterface.h  \
               LinksPlugin.h
SOURCES     += LinksPlugin.cpp
                
#===============================================================================

INCLUDEPATH += ../../libretroshare/src/

#=== and here are definitions, specific for this program =======================

DEPENDPATH += . \

HEADERS    += LinksDialog.h \
			  AddLinksDialog.h	

FORMS      += LinksDialog.ui \
			  AddLinksDialog.ui

SOURCES    += LinksDialog.cpp \
			  AddLinksDialog.cpp

RESOURCES  += ../../retroshare-gui/src/gui/images.qrc

                          
#===============================================================================

win32 {

    #LIBS += ../../libretroshare/src/lib/libretroshare.a
    #LIBS += -L"../../../../lib"
    #LIBS += -lssl -lcrypto -lgpgme -lpthreadGC2d -lminiupnpc -lz
    #LIBS += -luuid -lole32 -liphlpapi -lcrypt32-cygwin -lgdi32
    #LIBS += -lole32 -lwinmm 

    DEFINES += WINDOWS_SYS
    
    #GPG_ERROR_DIR = ../../../../libgpg-error-1.7
    #GPGME_DIR  = ../../../../gpgme-1.1.8
    #INCLUDEPATH += . $${GPGME_DIR}/src $${GPG_ERROR_DIR}/src
} 
