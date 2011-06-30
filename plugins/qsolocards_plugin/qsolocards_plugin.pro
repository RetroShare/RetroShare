# -------------------------------------------------
# Project created by QtCreator 2009-03-07T14:27:09
# -------------------------------------------------
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
TARGET        = $$qtLibraryTarget(qsolocards_plugin)

HEADERS     += ../PluginInterface.h  \
               QSoloCardsPlugin.h
SOURCES     += QSoloCardsPlugin.cpp
                
#===============================================================================
SOURCES += main.cpp \
    mainwindow.cpp \
    PlayingCard.cpp \
    CardDeck.cpp \
    CardPixmaps.cpp \
    CardStack.cpp \
    DragCardStack.cpp \
    VCardStack.cpp \
    CardAnimationLock.cpp \
    StackToStackAniMove.cpp \
    DealAnimation.cpp \
    FlipAnimation.cpp \
    StackToStackFlipAni.cpp \
    GameBoard.cpp \
    CardMoveRecord.cpp \
    About.cpp \
    Help.cpp \
    GameMgr.cpp \
    SpiderBoard.cpp \
    SpiderHomeStack.cpp \
    SpiderStack.cpp \
    KlondikeStack.cpp \
    KlondikeFlipStack.cpp \
    KlondikeHomeStack.cpp \
    KlondikeBoard.cpp \
    FreeCellBoard.cpp \
    FreeCellDeck.cpp \
    FreeCellStack.cpp \
    FreeCellHome.cpp \
    FreeCellFree.cpp \
    Spider3DeckBoard.cpp \
    SpideretteBoard.cpp \
    YukonBoard.cpp
HEADERS += mainwindow.h \
    PlayingCard.h \
    CardDeck.h \
    CardPixmaps.h \
    CardStack.h \
    DragCardStack.h \
    VCardStack.h \
    CardAnimationLock.h \
    StackToStackAniMove.h \
    DealAnimation.h \
    FlipAnimation.h \
    StackToStackFlipAni.h \
    GameBoard.h \
    CardMoveRecord.h \
    About.h \
    Help.h \
    GameMgr.h \
    SpiderBoard.h \
    SpiderHomeStack.h \
    SpiderStack.h \
    KlondikeStack.h \
    KlondikeFlipStack.h \
    KlondikeHomeStack.h \
    KlondikeBoard.h \
    FreeCellBoard.h \
    FreeCellDeck.h \
    FreeCellStack.h \
    FreeCellHome.h \
    FreeCellFree.h \
    Spider3DeckBoard.h \
    SpideretteBoard.h \
    YukonBoard.h

QT += svg
RESOURCES = QSoloCards.qrc

mac:QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.4u.sdk
mac:CONFIG+=x86 ppc

VER_MAJ = 0 
VER_MIN = 99 
VER_PAT = 1
QMAKE_CXXFLAGS_DEBUG = -DVER_MAJ=$$VER_MAJ \
    -DVER_MIN=$$VER_MIN \
    -DVER_PAT=$$VER_PAT
QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CXXFLAGS_DEBUG
mac:ICON = QSoloCards.icns
