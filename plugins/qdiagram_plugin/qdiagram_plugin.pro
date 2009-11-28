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
TARGET        = $$qtLibraryTarget(qdiagram_plugin)

HEADERS     += ../PluginInterface.h  \
               DiagramPlugin.h
SOURCES     += DiagramPlugin.cpp
                
#===============================================================================


HEADERS += diagrampathitem.h \
    diagramdrawitem.h \
    mainwindow.h \
    diagramitem.h \
    diagramscene.h \
    diagramtextitem.h
SOURCES += diagrampathitem.cpp \
    diagramdrawitem.cpp \
    mainwindow.cpp \
    diagramitem.cpp \
    diagramtextitem.cpp \
    diagramscene.cpp
RESOURCES = qdiagram.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/graphicsview/diagramscene
sources.files = $$SOURCES \
    $$HEADERS \
    $$RESOURCES \
    $$FORMS \
    diagramscene.pro \
    images
sources.path = $$[QT_INSTALL_EXAMPLES]/graphicsview/diagramscene
INSTALLS += target \
    sources
