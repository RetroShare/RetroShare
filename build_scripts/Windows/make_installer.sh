#!/bin/sh
###
#
#
#

QTGUI_DIR=../devel/retroshare-package-v0.4.09b/src/svn-retroshare/retroshare-gui/src
INST_DIR=../devel/retroshare-package-v0.4.09b/src/svn-retroshare/build_scripts/Windows/
EXEC=release/Retroshare.exe

BIN_DIR=./release
NSIS_EXE="/cygdrive/c/Program\ Files/NSIS/makensis.exe"

cp $QTGUI_DIR/$EXEC $BIN_DIR
echo cp $QTGUI_DIR/$EXEC $BIN_DIR

# copy skin files.
cp -r $QTGUI_DIR/qss/* $BIN_DIR/qss
echo 'cp -r $QTGUI_DIR/qss/* $BIN_DIR/qss'
#
cp -r $QTGUI_DIR/skin/* $BIN_DIR/skin
echo 'cp $QTGUI_DIR/skin/* $BIN_DIR/skin'
#
cp -r $QTGUI_DIR/emoticons/* $BIN_DIR/emoticons/
echo cp -r $QTGUI_DIR/emoticons/* $BIN_DIR/emoticons/

cp -r $QTGUI_DIR/style/* $BIN_DIR/style
echo 'cp -r $QTGUI_DIR/style/* $BIN_DIR/style'

./stripSVN.sh release
echo ./stripSVN.sh release

cp $INST_DIR/retroshare.nsi ./
echo cp $QTGUI_DIR/retroshare.nsi ./

cp $QTGUI_DIR/license/* ./license/
echo cp $QTGUI_DIR/license/* ./license/

cp $QTGUI_DIR/gui/images/splash.bmp ./gui/images/
echo cp $QTGUI_DIR/gui/images/splash.bmp ./gui/images/

#cp $QTGUI_DIR/release/changelog.txt ./release/changelog.txt
#echo cp $QTGUI_DIR/release/changelog.txt ./release/changelog.txt

"/cygdrive/c/Program Files/NSIS/makensis.exe" retroshare.nsi
#$NSIS_EXE retroshare.nsi
