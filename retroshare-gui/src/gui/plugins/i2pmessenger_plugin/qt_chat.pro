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
TARGET        = $$qtLibraryTarget(i2pmessenger_plugin)

HEADERS     += ../PluginInterface.h  \
               I2PMessengerPlugin.h
SOURCES     += I2PMessengerPlugin.cpp
                
#===============================================================================


#CONFIG += qt release

QT += network xml
#TEMPLATE = app

DEPENDPATH += . \
              gui \
              src

INCLUDEPATH += . \

SOURCES +=  gui/form_Main.cpp \
            src/Core.cpp \
            gui/form_newUser.cpp \
            gui/form_DebugMessages.cpp \
            src/User.cpp \
            src/ConnectionI2P.cpp \
            src/I2PSamMessageAnalyser.cpp \
            src/DebugMessageManager.cpp \
            src/UserConnectThread.cpp \
            src/Protocol.cpp \
            gui/form_chatwidget.cpp \
            gui/form_rename.cpp \
  	    src/PacketManager.cpp \
            gui/form_settingsgui.cpp \
  gui/form_HelpDialog.cpp \
  src/FileTransferSend.cpp \
  src/FileTransferRecive.cpp \
  gui/form_fileSend.cpp \
  gui/form_fileRecive.cpp

HEADERS +=  gui/form_Main.h \
            src/Core.h \
            gui/form_newUser.h \
            gui/form_DebugMessages.h \
            src/User.h \
            src/ConnectionI2P.h \
            src/I2PSamMessageAnalyser.h \
            src/DebugMessageManager.h \
            src/UserConnectThread.h \
            src/Protocol.h \
            gui/form_chatwidget.h \
            gui/form_rename.h \
	    src/PacketManager.h \
  gui/form_settingsgui.h \
  gui/form_HelpDialog.h \
  src/FileTransferSend.h \
  src/FileTransferRecive.h \
  gui/form_fileSend.h \
  gui/form_fileRecive.h




FORMS +=    gui/form_Main.ui \
            gui/form_DebugMessages.ui \
            gui/form_newUser.ui \
            gui/form_chatwidget.ui \
            gui/form_rename.ui \
            gui/form_settingsgui.ui \
    gui/form_HelpDialog.ui \
    gui/form_fileSend.ui \
    gui/form_fileRecive.ui

RESOURCES += gui/resourcen.qrc




