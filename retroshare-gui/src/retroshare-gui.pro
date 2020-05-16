################################################################################
# uselibresapi.pri                                                             #
# Copyright (C) 2018, Retroshare team <retroshare.team@gmailcom>               #
#                                                                              #
# This program is free software: you can redistribute it and/or modify         #
# it under the terms of the GNU Affero General Public License as               #
# published by the Free Software Foundation, either version 3 of the           #
# License, or (at your option) any later version.                              #
#                                                                              #
# This program is distributed in the hope that it will be useful,              #
# but WITHOUT ANY WARRANTY; without even the implied warranty of               #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                #
# GNU Affero General Public License for more details.                          #
#                                                                              #
# You should have received a copy of the GNU Affero General Public License     #
# along with this program.  If not, see <https://www.gnu.org/licenses/>.       #
################################################################################

!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TEMPLATE = app
QT     += network xml 
CONFIG += qt gui uic qrc resources idle 
CONFIG += console
TARGET = retroshare
DEFINES += TARGET=\\\"$${TARGET}\\\"

DEPENDPATH  *= $${PWD} $${RS_INCLUDE_DIR} retroshare-gui
INCLUDEPATH *= $${PWD} retroshare-gui

!include("../../libretroshare/src/use_libretroshare.pri"):error("Including")

rs_webui {
rs_jsonapi {
    HEADERS *= gui/settings/WebuiPage.h
    SOURCES *= gui/settings/WebuiPage.cpp
    FORMS *= gui/settings/WebuiPage.ui
}
}

rs_jsonapi {
    HEADERS *= gui/settings/JsonApiPage.h
    SOURCES *= gui/settings/JsonApiPage.cc
    FORMS *= gui/settings/JsonApiPage.ui
}

# Auto detect installed version of cmark
rs_gui_cmark {
	DEFINES *= USE_CMARK
	no_rs_cross_compiling {
		message("Using compiled cmark")
		CMARK_SRC_PATH=$$clean_path($${RS_SRC_PATH}/supportlibs/cmark)
		CMARK_BUILD_PATH=$$clean_path($${RS_BUILD_PATH}/supportlibs/cmark/build)
		INCLUDEPATH *= $$clean_path($${CMARK_SRC_PATH}/src/)
		DEPENDPATH *= $$clean_path($${CMARK_SRC_PATH}/src/)
		QMAKE_LIBDIR *= $$clean_path($${CMARK_BUILD_PATH}/)
		# Using sLibs would fail as libcmark.a is generated at compile-time
		LIBS *= -L$$clean_path($${CMARK_BUILD_PATH}/src/) -lcmark

		DUMMYCMARKINPUT = FORCE
		CMAKE_GENERATOR_OVERRIDE=""
		win32-g++:CMAKE_GENERATOR_OVERRIDE="-G \"MSYS Makefiles\""
		gencmarklib.name = Generating libcmark.
		gencmarklib.input = DUMMYCMARKINPUT
		gencmarklib.output = $$clean_path($${CMARK_BUILD_PATH}/src/libcmark.a)
		gencmarklib.CONFIG += target_predeps combine
		gencmarklib.variable_out = PRE_TARGETDEPS
		gencmarklib.commands = \
		    cd $${RS_SRC_PATH} && ( \
		    git submodule update --init supportlibs/cmark ; \
		    cd $${CMARK_SRC_PATH} ; \
		    true ) && \
		    mkdir -p $${CMARK_BUILD_PATH} && cd $${CMARK_BUILD_PATH} && \
		    cmake \
		        -DCMAKE_CXX_COMPILER=$$QMAKE_CXX \
		        $${CMAKE_GENERATOR_OVERRIDE} \
		        -DCMAKE_INSTALL_PREFIX=. \
		        -B. \
		        -H$$shell_path($${CMARK_SRC_PATH}) && \
		    $(MAKE)
		QMAKE_EXTRA_COMPILERS += gencmarklib
	} else {
		message("Using systems cmark")
		sLibs *= libcmark
	}
}

FORMS   += TorControl/TorControlWindow.ui
SOURCES += TorControl/TorControlWindow.cpp
HEADERS += TorControl/TorControlWindow.h

#QMAKE_CFLAGS += -fmudflap 
#LIBS *= /usr/lib/gcc/x86_64-linux-gnu/4.4/libmudflap.a /usr/lib/gcc/x86_64-linux-gnu/4.4/libmudflapth.a

greaterThan(QT_MAJOR_VERSION, 4) {
	# Qt 5
        QT     += widgets multimedia printsupport
	linux-* {
		QT += x11extras
	}
} else {
	# Qt 4
	CONFIG += uitools
}

CONFIG += identities
CONFIG += gxsforums
CONFIG += gxschannels
CONFIG += posted
CONFIG += gxsgui
CONFIG += gxscircles

# Other Disabled Bits.
#CONFIG += framecatcher
#CONFIG += blogs

## To enable unfinished services
#CONFIG += wikipoos
#CONFIG += gxsthewire
#CONFIG += gxsphotoshare

DEFINES += RS_RELEASE_VERSION
RCC_DIR = temp/qrc
UI_DIR  = temp/ui
MOC_DIR = temp/moc

################################# Linux ##########################################
# Put lib dir in QMAKE_LFLAGS so it appears before -L/usr/lib
linux-* {
	CONFIG += link_pkgconfig

	#CONFIG += version_detail_bash_script
	QMAKE_CXXFLAGS *= -D_FILE_OFFSET_BITS=64

	PKGCONFIG *= x11 xscrnsaver

	LIBS *= -rdynamic 
	DEFINES *= HAVE_XSS # for idle time, libx screensaver extensions
}

rs_sanitize {
	LIBS *= -lasan -lubsan
}

unix {
	target.path = "$${BIN_DIR}"
	INSTALLS += target

	data_files.path="$${DATA_DIR}/"
	data_files.files=sounds qss
	INSTALLS += data_files

	style_files.path="$${DATA_DIR}/stylesheets"
	style_files.files=gui/qss/chat/Bubble gui/qss/chat/Bubble_Compact
	INSTALLS += style_files

	icon_files.path = "$${PREFIX}/share/icons/hicolor"
	icon_files.files =  ../../data/24x24
	icon_files.files += ../../data/48x48
	icon_files.files += ../../data/64x64
	icon_files.files += ../../data/128x128
	INSTALLS += icon_files

	desktop_files.path = "$${PREFIX}/share/applications"
	desktop_files.files = ../../data/retroshare.desktop
	INSTALLS += desktop_files

	pixmap_files.path = "$${PREFIX}/share/pixmaps"
	pixmap_files.files = ../../data/retroshare.xpm
	INSTALLS += pixmap_files

}

linux-g++ {
	OBJECTS_DIR = temp/linux-g++/obj
}

linux-g++-64 {
	OBJECTS_DIR = temp/linux-g++-64/obj
}

version_detail_bash_script {
	warning("Version detail script is deprecated.")
	warning("Remove references to version_detail_bash_script from all of your build scripts!")
}

#################### Cross compilation for windows under Linux ###################

win32-x-g++ {
		OBJECTS_DIR = temp/win32-x-g++/obj

		LIBS += ../../../../lib/win32-x-g++-v0.5/libssl.a
		LIBS += ../../../../lib/win32-x-g++-v0.5/libcrypto.a
		LIBS += ../../../../lib/win32-x-g++-v0.5/libgpgme.dll.a
		LIBS += ../../../../lib/win32-x-g++-v0.5/libminiupnpc.a
		LIBS += ../../../../lib/win32-x-g++-v0.5/libz.a
		LIBS += -L${HOME}/.wine/drive_c/pthreads/lib -lpthreadGCE2
		LIBS += -lQtUiTools
		LIBS += -lws2_32 -luuid -lole32 -liphlpapi -lcrypt32 -gdi32
		LIBS += -lole32 -lwinmm

		DEFINES *= WINDOWS_SYS WIN32 WIN32_CROSS_UBUNTU

		INCLUDEPATH += ../../../../gpgme-1.1.8/src/
		INCLUDEPATH += ../../../../libgpg-error-1.7/src/

		RC_FILE = gui/images/retroshare_win.rc
}

#################################### Windows #####################################

win32-g++ {
	CONFIG(debug, debug|release) {
		# show console output
		CONFIG += console
	} else {
		CONFIG -= console
	}

	# Switch on extra warnings
	QMAKE_CFLAGS += -Wextra
	QMAKE_CXXFLAGS += -Wextra

	CONFIG(debug, debug|release) {
	} else {
		# Tell linker to use ASLR protection
		QMAKE_LFLAGS += -Wl,-dynamicbase
		# Tell linker to use DEP protection
		QMAKE_LFLAGS += -Wl,-nxcompat
	}

    # Fix linking error (ld.exe: Error: export ordinal too large) due to too
    # many exported symbols.
    QMAKE_LFLAGS+=-Wl,--exclude-libs,ALL

	# Switch off optimization for release version
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O0
	QMAKE_CFLAGS_RELEASE -= -O2
	QMAKE_CFLAGS_RELEASE += -O0

	# Switch on optimization for debug version
	#QMAKE_CXXFLAGS_DEBUG += -O2
	#QMAKE_CFLAGS_DEBUG += -O2

	OBJECTS_DIR = temp/obj

    dLib = ws2_32 gdi32 uuid ole32 iphlpapi crypt32 winmm
    LIBS *= $$linkDynamicLibs(dLib)

	RC_FILE = gui/images/retroshare_win.rc

	# export symbols for the plugins
	LIBS += -Wl,--export-all-symbols,--out-implib,lib/libretroshare-gui.a

	# create lib directory
	isEmpty(QMAKE_SH) {
		QMAKE_PRE_LINK = $(CHK_DIR_EXISTS) lib $(MKDIR) lib
	} else {
		QMAKE_PRE_LINK = $(CHK_DIR_EXISTS) lib || $(MKDIR) lib
	}

	greaterThan(QT_MAJOR_VERSION, 4) {
		# Qt 5
		RC_INCLUDEPATH += $$_PRO_FILE_PWD_/../../libretroshare/src
	} else {
		# Qt 4
		QMAKE_RC += --include-dir=$$_PRO_FILE_PWD_/../../libretroshare/src
	}
}

##################################### MacOS ######################################

macx {
	# ENABLE THIS OPTION FOR Univeral Binary BUILD.
	#CONFIG += ppc x86
	#QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.4
	QMAKE_INFO_PLIST = Info.plist
	mac_icon.files = $$files($$PWD/rsMacIcon.icns)
	mac_icon.path = Contents/Resources
	QMAKE_BUNDLE_DATA += mac_icon
#	mac_webui.files = $$files($$PWD/../../libresapi/src/webui)
#	mac_webui.path = Contents/Resources
#	QMAKE_BUNDLE_DATA += mac_webui

	CONFIG += version_detail_bash_script
        LIBS += -lssl -lcrypto -lz 
        #LIBS += -lssl -lcrypto -lz -lgpgme -lgpg-error -lassuan
	for(lib, LIB_DIR):exists($$lib/libminiupnpc.a){ LIBS += $$lib/libminiupnpc.a}
	LIBS += -framework CoreFoundation
	LIBS += -framework Security
	LIBS += -framework Carbon

	for(lib, LIB_DIR):LIBS += -L"$$lib"
	for(bin, BIN_DIR):LIBS += -L"$$bin"

	DEPENDPATH += . $$INC_DIR
	INCLUDEPATH += . $$INC_DIR

	#DEFINES *= MAC_IDLE # for idle feature
	CONFIG -= uitools
}

##################################### FreeBSD ######################################

freebsd-* {
	INCLUDEPATH *= /usr/local/include/gpgme
	LIBS *= -lssl
	LIBS *= -lgpgme
	LIBS *= -lupnp
	LIBS *= -lgnome-keyring

	LIBS += -lsqlite3
}

##################################### Haiku ######################################

haiku-* {
	PRE_TARGETDEPS *= ../../libretroshare/src/lib/libretroshare.a
	PRE_TARGETDEPS *= ../../openpgpsdk/src/lib/libops.a

	LIBS *= ../../libretroshare/src/lib/libretroshare.a
	LIBS *= ../../openpgpsdk/src/lib/libops.a -lbz2 -lbsd
	LIBS *= -lssl -lcrypto -lnetwork
	LIBS *= -lgpgme
	LIBS *= -lupnp
	LIBS *= -lz
	LIBS *= -lixml

	LIBS += ../../supportlibs/pegmarkdown/lib/libpegmarkdown.a
	LIBS += -lsqlite3
}

##################################### OpenBSD ######################################

openbsd-* {
	INCLUDEPATH *= /usr/local/include

	LIBS *= -lssl -lcrypto
	LIBS *= -lgpgme
	LIBS *= -lupnp
	LIBS *= -lgnome-keyring
	LIBS += -lsqlite3
	LIBS *= -rdynamic
}

wikipoos {
	PRE_TARGETDEPS *= $$OUT_PWD/../../supportlibs/pegmarkdown/lib/libpegmarkdown.a
	LIBS *= $$OUT_PWD/../../supportlibs/pegmarkdown/lib/libpegmarkdown.a
	LIBS *= -lglib-2.0
}

# Tor controller

HEADERS += 	TorControl/AddOnionCommand.h \
           	TorControl/AuthenticateCommand.h \
           	TorControl/CryptoKey.h \
           	TorControl/GetConfCommand.h \
           	TorControl/HiddenService.h \
           	TorControl/PendingOperation.h  \
           	TorControl/ProtocolInfoCommand.h \
           	TorControl/SecureRNG.h \
           	TorControl/SetConfCommand.h \
           	TorControl/Settings.h \
           	TorControl/StrUtil.h \
           	TorControl/TorControl.h \
           	TorControl/TorControlCommand.h \
           	TorControl/TorControlSocket.h \
           	TorControl/TorManager.h \
           	TorControl/TorProcess.h \
           	TorControl/TorProcess_p.h \
           	TorControl/TorSocket.h \
           	TorControl/Useful.h

SOURCES += 	TorControl/AddOnionCommand.cpp \
				TorControl/AuthenticateCommand.cpp \
				TorControl/GetConfCommand.cpp \
				TorControl/HiddenService.cpp \
				TorControl/ProtocolInfoCommand.cpp \
				TorControl/SetConfCommand.cpp \
				TorControl/TorControlCommand.cpp \
				TorControl/TorControl.cpp \
				TorControl/TorControlSocket.cpp \
				TorControl/TorManager.cpp \
				TorControl/TorProcess.cpp \
				TorControl/TorSocket.cpp \
				TorControl/CryptoKey.cpp         \
				TorControl/PendingOperation.cpp  \
				TorControl/SecureRNG.cpp         \
				TorControl/Settings.cpp          \
				TorControl/StrUtil.cpp        

# Input
HEADERS +=  rshare.h \
            retroshare-gui/configpage.h \
            retroshare-gui/RsAutoUpdatePage.h \
            retroshare-gui/mainpage.h \
            gui/notifyqt.h \
            control/bandwidthevent.h \
            control/eventtype.h \
            gui/QuickStartWizard.h \
            gui/StartDialog.h \
            gui/HomePage.h\
            gui/NetworkDialog.h \
            gui/GenCertDialog.h \
            gui/linetypes.h \
            gui/mainpagestack.h \
            gui/MainWindow.h \
            gui/RSHumanReadableDelegate.h \
            gui/AboutDialog.h \
            gui/AboutWidget.h \
            gui/NetworkView.h \
            gui/FriendsDialog.h \
            gui/ServicePermissionDialog.h \
            gui/RemoteDirModel.h \
            gui/RetroShareLink.h \
            gui/SearchTreeWidget.h \
            gui/ShareManager.h \
#            gui/ShareDialog.h \
#            gui/SFListDelegate.h \
            gui/SoundManager.h \
            gui/HelpDialog.h \
            gui/LogoBar.h \
            gui/common/AvatarDialog.h \
            gui/FileTransfer/SearchDialog.h \
            gui/FileTransfer/SharedFilesDialog.h \
            gui/FileTransfer/xprogressbar.h \
            gui/FileTransfer/DetailsDialog.h \
            gui/FileTransfer/FileTransferInfoWidget.h \
            gui/FileTransfer/DLListDelegate.h \
            gui/FileTransfer/ULListDelegate.h \
            gui/FileTransfer/TransfersDialog.h \
            gui/FileTransfer/BannedFilesDialog.h \
            gui/statistics/TurtleRouterDialog.h \
            gui/statistics/TurtleRouterStatistics.h \
            gui/statistics/dhtgraph.h \
            gui/statistics/BandwidthGraphWindow.h \
            gui/statistics/turtlegraph.h \
            gui/statistics/BandwidthStatsWidget.h \
            gui/statistics/DhtWindow.h \
            gui/statistics/GlobalRouterStatistics.h \
            gui/statistics/GxsTransportStatistics.h \
            gui/statistics/StatisticsWindow.h \
            gui/statistics/BwCtrlWindow.h \
            gui/statistics/RttStatistics.h \
            gui/FileTransfer/TransferUserNotify.h \
            gui/plugins/PluginInterface.h \
            gui/im_history/ImHistoryBrowser.h \
            gui/im_history/IMHistoryItemDelegate.h \
            gui/im_history/IMHistoryItemPainter.h \
            lang/languagesupport.h \
            util/RsProtectedTimer.h \
            util/stringutil.h \
            util/RsNetUtil.h \
            util/DateTime.h \
            util/RetroStyleLabel.h \
            util/dllexport.h \
            util/NonCopyable.h \
            util/rsutildll.h \
            util/dllexport.h \
            util/global.h \
            util/rsqtutildll.h \
#            util/Interface.h \
            util/PixmapMerging.h \
            util/MouseEventFilter.h \
            util/EventFilter.h \
            util/Widget.h \
            util/RsAction.h \
            util/RsUserdata.h \
            util/printpreview.h \
            util/log.h \
            util/misc.h \
            util/HandleRichText.h \
            util/ObjectPainter.h \
            util/QtVersion.h \
            util/RsFile.h \
            util/qtthreadsutils.h \
            util/ClickableLabel.h \
            util/AspectRatioPixmapLabel.h \
            gui/profile/ProfileWidget.h \
            gui/profile/ProfileManager.h \
            gui/profile/StatusMessage.h \
            gui/chat/PopupChatWindow.h \
            gui/chat/PopupChatDialog.h \
            gui/chat/PopupDistantChatDialog.h \
            gui/chat/ChatTabWidget.h \
            gui/chat/ChatWidget.h \
            gui/chat/ChatDialog.h \
            gui/ChatLobbyWidget.h \
            gui/chat/ChatLobbyDialog.h \
            gui/chat/CreateLobbyDialog.h \
            gui/chat/ChatStyle.h \
            gui/chat/ChatUserNotify.h \
            gui/chat/ChatLobbyUserNotify.h \
            gui/connect/ConfCertDialog.h \
            gui/connect/PGPKeyDialog.h \
            gui/connect/FriendRecommendDialog.h \
            gui/msgs/MessagesDialog.h \
            gui/msgs/MessageInterface.h \
            gui/msgs/MessageComposer.h \
            gui/msgs/MessageWindow.h \
            gui/msgs/MessageWidget.h \
            gui/msgs/MessageModel.h \
            gui/msgs/TagsMenu.h \
            gui/msgs/textformat.h \
            gui/msgs/MessageUserNotify.h \
            gui/images/retroshare_win.rc.h \
            gui/settings/RSPermissionMatrixWidget.h \
            gui/settings/rsharesettings.h \
            gui/settings/RsharePeerSettings.h \
            gui/settings/rsettings.h \
            gui/settings/rsettingswin.h \
            gui/settings/GeneralPage.h \
            gui/settings/PeoplePage.h \
            gui/settings/AboutPage.h \
            gui/settings/ServerPage.h \
            gui/settings/NotifyPage.h \
            gui/settings/CryptoPage.h \
            gui/settings/MessagePage.h \
            gui/settings/NewTag.h \
            gui/settings/ForumPage.h \
            gui/settings/PluginsPage.h \
            gui/settings/PluginItem.h \
            gui/settings/AppearancePage.h \
            gui/settings/FileAssociationsPage.h \
            gui/settings/SoundPage.h \
            gui/settings/TransferPage.h \
            gui/settings/ChatPage.h \
            gui/settings/ChannelPage.h \
            gui/settings/PostedPage.h \
            gui/settings/ServicePermissionsPage.h \
            gui/settings/AddFileAssociationDialog.h \
            gui/settings/GroupFrameSettingsWidget.h \
            gui/toaster/ToasterItem.h \
            gui/toaster/MessageToaster.h \
            gui/toaster/OnlineToaster.h \
            gui/toaster/DownloadToaster.h \
            gui/toaster/ChatToaster.h \
            gui/toaster/GroupChatToaster.h \
            gui/toaster/ChatLobbyToaster.h \
            gui/toaster/FriendRequestToaster.h \
            gui/common/RsButtonOnText.h \
            gui/common/RSGraphWidget.h \
            gui/common/ElidedLabel.h \
            gui/common/vmessagebox.h \
            gui/common/RsUrlHandler.h \
            gui/common/RsCollectionDialog.h \
            gui/common/rwindow.h \
            gui/common/rshtml.h \
            gui/common/AvatarDefs.h \
            gui/common/GroupFlagsWidget.h \
            gui/common/GroupSelectionBox.h \
            gui/common/GroupChooser.h \
            gui/common/StatusDefs.h \
            gui/common/TagDefs.h \
            gui/common/GroupDefs.h \
            gui/common/Emoticons.h \
            gui/common/RSListWidgetItem.h \
            gui/common/RSTextEdit.h \
            gui/common/RSPlainTextEdit.h \
            gui/common/RSTreeWidget.h \
            gui/common/RSTreeWidgetItem.h \
            gui/common/RSFeedWidget.h \
            gui/common/RSTabWidget.h \
            gui/common/RSElidedItemDelegate.h \
            gui/common/RSItemDelegate.h \
            gui/common/PeerDefs.h \
            gui/common/FilesDefs.h \
            gui/common/PopularityDefs.h \
            gui/common/RsBanListDefs.h \
            gui/common/GroupTreeWidget.h \
            gui/common/RSTreeView.h \
            gui/common/AvatarWidget.h \
            gui/common/FriendListModel.h \
            gui/common/NewFriendList.h \
            gui/common/FriendSelectionWidget.h \
            gui/common/FriendSelectionDialog.h \
            gui/common/HashBox.h \
            gui/common/LineEditClear.h \
            gui/common/DropLineEdit.h \
            gui/common/RSTextBrowser.h \
            gui/common/RSImageBlockWidget.h \
            gui/common/FeedNotify.h \
            gui/common/UserNotify.h \
            gui/common/HeaderFrame.h \
            gui/common/MimeTextEdit.h \
            gui/common/UIStateHelper.h \
            gui/common/FloatingHelpBrowser.h \
            gui/common/SubscribeToolButton.h \
            gui/common/RsBanListToolButton.h \
            gui/common/FlowLayout.h \
            gui/common/PictureFlow.h \
            gui/common/StyledLabel.h \
            gui/common/StyledElidedLabel.h \
            gui/common/ToasterNotify.h \
            gui/style/RSStyle.h \
            gui/style/StyleDialog.h \
            gui/help/browser/helpbrowser.h \
            gui/help/browser/helptextbrowser.h \
            gui/statusbar/peerstatus.h \
            gui/statusbar/natstatus.h \
            gui/statusbar/dhtstatus.h \
            gui/statusbar/torstatus.h \
            gui/statusbar/ratesstatus.h \
            gui/statusbar/hashingstatus.h \
            gui/statusbar/discstatus.h \
            gui/statusbar/SoundStatus.h \
            gui/statusbar/OpModeStatus.h \
            gui/statusbar/ToasterDisable.h \
            gui/statusbar/SysTrayStatus.h \
            gui/advsearch/advancedsearchdialog.h \
            gui/advsearch/expressionwidget.h \
            gui/advsearch/guiexprelement.h \
            gui/elastic/graphwidget.h \
            gui/elastic/edge.h \
            gui/elastic/arrow.h \
            gui/elastic/elnode.h \
            gui/NewsFeed.h \
            gui/feeds/FeedItem.h \
            gui/feeds/FeedHolder.h \
            gui/feeds/GxsCircleItem.h \
            gui/feeds/PeerItem.h \
            gui/feeds/MsgItem.h \
            gui/feeds/ChatMsgItem.h \
            gui/feeds/SubFileItem.h \
            gui/feeds/AttachFileItem.h \
            gui/feeds/SecurityItem.h \
            gui/feeds/SecurityIpItem.h \
            gui/feeds/NewsFeedUserNotify.h \
            gui/connect/ConnectFriendWizard.h \
            gui/connect/ConnectProgressDialog.h \
            gui/groups/CreateGroup.h \
            gui/GetStartedDialog.h \
            gui/statistics/BWGraph.h \
            util/RsSyntaxHighlighter.h \
            util/imageutil.h \
            util/RichTextEdit.h \
            gui/NetworkDialog/pgpid_item_model.h \
            gui/NetworkDialog/pgpid_item_proxy.h \
            gui/common/RsCollection.h \
            util/retroshareWin32.h
#            gui/ForumsDialog.h \
#            gui/forums/ForumDetails.h \
#            gui/forums/EditForumDetails.h \
#            gui/forums/CreateForum.h \
#            gui/forums/CreateForumMsg.h \
#            gui/forums/ForumUserNotify.h \
#            gui/feeds/ForumNewItem.h \
#            gui/feeds/ForumMsgItem.h \
#            gui/ChannelFeed.h \
#            gui/feeds/ChanNewItem.h \
#            gui/feeds/ChanMsgItem.h \
#            gui/channels/CreateChannel.h \
#            gui/channels/ChannelDetails.h \
#            gui/channels/CreateChannelMsg.h \
#            gui/channels/EditChanDetails.h \
#            gui/channels/ChannelUserNotify.h \

FORMS +=    gui/StartDialog.ui \
            gui/HomePage.ui\
            gui/GenCertDialog.ui \
            gui/AboutDialog.ui \
            gui/AboutWidget.ui \
            gui/QuickStartWizard.ui \
            gui/NetworkDialog.ui \
            gui/common/AvatarDialog.ui \
            gui/FileTransfer/TransfersDialog.ui \
            gui/FileTransfer/DetailsDialog.ui \
            gui/FileTransfer/SearchDialog.ui \
            gui/FileTransfer/SharedFilesDialog.ui \
            gui/FileTransfer/BannedFilesDialog.ui \
            gui/MainWindow.ui \
            gui/NetworkView.ui \
            gui/FriendsDialog.ui \
            gui/ShareManager.ui \
#            gui/ShareDialog.ui \
            gui/help/browser/helpbrowser.ui \
            gui/HelpDialog.ui \
            gui/ServicePermissionDialog.ui \
            gui/profile/ProfileWidget.ui \
            gui/profile/StatusMessage.ui \
            gui/profile/ProfileManager.ui \
            gui/chat/PopupChatWindow.ui \
            gui/chat/PopupChatDialog.ui \
            gui/chat/ChatTabWidget.ui \
            gui/chat/ChatWidget.ui \
            gui/chat/ChatLobbyDialog.ui \
            gui/chat/CreateLobbyDialog.ui \
            gui/ChatLobbyWidget.ui \
            gui/connect/ConfCertDialog.ui \
            gui/connect/PGPKeyDialog.ui \
            gui/connect/ConnectFriendWizard.ui \
            gui/connect/ConnectProgressDialog.ui \
            gui/connect/FriendRecommendDialog.ui \
            gui/msgs/MessagesDialog.ui \
            gui/msgs/MessageComposer.ui \
            gui/msgs/MessageWindow.ui\
            gui/msgs/MessageWidget.ui\
            gui/settings/settingsw.ui \
            gui/settings/GeneralPage.ui \
            gui/settings/ServerPage.ui \
            gui/settings/NotifyPage.ui \
            gui/settings/PeoplePage.ui \
            gui/settings/CryptoPage.ui \
            gui/settings/MessagePage.ui \
            gui/settings/NewTag.ui \
            gui/settings/ForumPage.ui \
            gui/settings/AboutPage.ui \
            gui/settings/PluginsPage.ui \
            gui/settings/AppearancePage.ui \
            gui/settings/TransferPage.ui \
            gui/settings/SoundPage.ui \
            gui/settings/ChatPage.ui \
            gui/settings/ChannelPage.ui \
            gui/settings/PostedPage.ui \
            gui/settings/ServicePermissionsPage.ui \
            gui/settings/PluginItem.ui \
            gui/settings/GroupFrameSettingsWidget.ui \
            gui/toaster/MessageToaster.ui \
            gui/toaster/OnlineToaster.ui \
            gui/toaster/DownloadToaster.ui \
            gui/toaster/ChatToaster.ui \
            gui/toaster/GroupChatToaster.ui \
            gui/toaster/ChatLobbyToaster.ui \
            gui/toaster/FriendRequestToaster.ui \
            gui/advsearch/AdvancedSearchDialog.ui \
            gui/advsearch/expressionwidget.ui \
            gui/NewsFeed.ui \
            gui/feeds/GxsCircleItem.ui \
            gui/feeds/PeerItem.ui \
            gui/feeds/MsgItem.ui \
            gui/feeds/ChatMsgItem.ui \
            gui/feeds/SubFileItem.ui \
            gui/feeds/AttachFileItem.ui \
            gui/feeds/SecurityItem.ui \
            gui/feeds/SecurityIpItem.ui \
            gui/im_history/ImHistoryBrowser.ui \
            gui/groups/CreateGroup.ui \
            gui/common/GroupTreeWidget.ui \
            gui/common/AvatarWidget.ui \
            gui/common/NewFriendList.ui \
            gui/common/FriendSelectionWidget.ui \
            gui/common/HashBox.ui \
            gui/common/RSImageBlockWidget.ui \
            gui/common/RsCollectionDialog.ui \
            gui/common/HeaderFrame.ui \
            gui/common/RSFeedWidget.ui \
            gui/style/StyleDialog.ui \
            gui/statistics/BandwidthGraphWindow.ui \
            gui/statistics/BandwidthStatsWidget.ui \
            gui/statistics/DhtWindow.ui \
            gui/statistics/TurtleRouterDialog.ui \
            gui/statistics/TurtleRouterStatistics.ui \
            gui/statistics/GlobalRouterStatistics.ui \
            gui/statistics/GxsTransportStatistics.ui \
            gui/statistics/StatisticsWindow.ui \
            gui/statistics/BwCtrlWindow.ui \
            gui/statistics/RttStatistics.ui \
            gui/GetStartedDialog.ui \
            util/RichTextEdit.ui


#            gui/ForumsDialog.ui \
#            gui/forums/CreateForum.ui \
#            gui/forums/CreateForumMsg.ui \
#            gui/forums/ForumDetails.ui \
#            gui/forums/EditForumDetails.ui \
#            gui/feeds/ForumNewItem.ui \
#            gui/feeds/ForumMsgItem.ui \
#            gui/ChannelFeed.ui \
#            gui/channels/CreateChannel.ui \
#            gui/channels/CreateChannelMsg.ui \
#            gui/channels/ChannelDetails.ui \
#            gui/channels/EditChanDetails.ui \
#            gui/feeds/ChanNewItem.ui \
#            gui/feeds/ChanMsgItem.ui \

SOURCES +=  main.cpp \
            rshare.cpp \
            gui/notifyqt.cpp \
            gui/AboutDialog.cpp \
            gui/AboutWidget.cpp \
            gui/QuickStartWizard.cpp \
            gui/StartDialog.cpp \
            gui/HomePage.cpp\
            gui/GenCertDialog.cpp \
            gui/NetworkDialog.cpp \
            gui/mainpagestack.cpp \
            gui/MainWindow.cpp \
            gui/NetworkView.cpp \
            gui/FriendsDialog.cpp \
            gui/ServicePermissionDialog.cpp \
            gui/RemoteDirModel.cpp \
            gui/RsAutoUpdatePage.cpp \
            gui/RetroShareLink.cpp \
            gui/SearchTreeWidget.cpp \
            gui/ShareManager.cpp \
#            gui/ShareDialog.cpp \
#            gui/SFListDelegate.cpp \
            gui/SoundManager.cpp \
            gui/im_history/ImHistoryBrowser.cpp \
            gui/im_history/IMHistoryItemDelegate.cpp \
            gui/im_history/IMHistoryItemPainter.cpp \
            gui/help/browser/helpbrowser.cpp \
            gui/help/browser/helptextbrowser.cpp \
            gui/FileTransfer/SearchDialog.cpp \
            gui/FileTransfer/SharedFilesDialog.cpp \
            gui/FileTransfer/TransfersDialog.cpp \
            gui/FileTransfer/FileTransferInfoWidget.cpp \
            gui/FileTransfer/DLListDelegate.cpp \
            gui/FileTransfer/ULListDelegate.cpp \
            gui/FileTransfer/xprogressbar.cpp \
            gui/FileTransfer/DetailsDialog.cpp \
            gui/FileTransfer/TransferUserNotify.cpp \
            gui/FileTransfer/BannedFilesDialog.cpp \
            gui/MainPage.cpp \
            gui/HelpDialog.cpp \
            gui/LogoBar.cpp \
            lang/languagesupport.cpp \
            util/RsProtectedTimer.cpp \
            util/stringutil.cpp \
            util/RsNetUtil.cpp \
            util/DateTime.cpp \
            util/RetroStyleLabel.cpp \
            util/WidgetBackgroundImage.cpp \
#            util/NonCopyable.cpp \
            util/PixmapMerging.cpp \
            util/MouseEventFilter.cpp \
            util/EventFilter.cpp \
            util/Widget.cpp \
            util/RsAction.cpp \
            util/printpreview.cpp \
            util/log.cpp \
            util/misc.cpp \
            util/HandleRichText.cpp \
            util/ObjectPainter.cpp \
            util/RsFile.cpp \
            util/RichTextEdit.cpp \
            util/ClickableLabel.cpp \
            util/AspectRatioPixmapLabel.cpp \
            gui/profile/ProfileWidget.cpp \
            gui/profile/StatusMessage.cpp \
            gui/profile/ProfileManager.cpp \
            gui/chat/PopupChatWindow.cpp \
            gui/chat/PopupChatDialog.cpp \
            gui/chat/PopupDistantChatDialog.cpp \
            gui/chat/ChatTabWidget.cpp \
            gui/chat/ChatWidget.cpp \
            gui/chat/ChatDialog.cpp \
            gui/ChatLobbyWidget.cpp \
            gui/chat/ChatLobbyDialog.cpp \
            gui/chat/CreateLobbyDialog.cpp \
            gui/chat/ChatStyle.cpp \
            gui/chat/ChatUserNotify.cpp \
            gui/chat/ChatLobbyUserNotify.cpp \
            gui/connect/ConfCertDialog.cpp \
            gui/connect/PGPKeyDialog.cpp \
            gui/msgs/MessagesDialog.cpp \
            gui/msgs/MessageComposer.cpp \
            gui/msgs/MessageWidget.cpp \
            gui/msgs/MessageWindow.cpp \
            gui/msgs/MessageModel.cpp \
            gui/msgs/TagsMenu.cpp \
            gui/msgs/MessageUserNotify.cpp \
            gui/common/RsButtonOnText.cpp \
            gui/common/RSGraphWidget.cpp \
            gui/common/ElidedLabel.cpp \
            gui/common/vmessagebox.cpp \
            gui/common/RsCollectionDialog.cpp \
            gui/common/RsUrlHandler.cpp \
            gui/common/rwindow.cpp \
            gui/common/rshtml.cpp \
            gui/common/AvatarDefs.cpp \
            gui/common/AvatarDialog.cpp \
            gui/common/GroupFlagsWidget.cpp \
            gui/common/GroupSelectionBox.cpp \
            gui/common/GroupChooser.cpp \
            gui/common/StatusDefs.cpp \
            gui/common/TagDefs.cpp \
            gui/common/GroupDefs.cpp \
            gui/common/Emoticons.cpp \
            gui/common/RSListWidgetItem.cpp \
            gui/common/RSTextEdit.cpp \
            gui/common/RSPlainTextEdit.cpp \
            gui/common/RSTreeWidget.cpp \
            gui/common/RSTreeWidgetItem.cpp \
            gui/common/RSFeedWidget.cpp \
            gui/common/RSTabWidget.cpp \
            gui/common/RSElidedItemDelegate.cpp \
            gui/common/RSItemDelegate.cpp \
            gui/common/PeerDefs.cpp \
            gui/common/FilesDefs.cpp \
            gui/common/PopularityDefs.cpp \
            gui/common/RsBanListDefs.cpp \
            gui/common/GroupTreeWidget.cpp \
            gui/common/RSTreeView.cpp \
            gui/common/AvatarWidget.cpp \
            gui/common/FriendListModel.cpp \
            gui/common/NewFriendList.cpp \
            gui/common/FriendSelectionWidget.cpp \
            gui/common/FriendSelectionDialog.cpp \
            gui/common/HashBox.cpp \
            gui/common/LineEditClear.cpp \
            gui/common/DropLineEdit.cpp \
            gui/common/RSTextBrowser.cpp \
            gui/common/RSImageBlockWidget.cpp \
            gui/common/FeedNotify.cpp \
            gui/common/UserNotify.cpp \
            gui/common/HeaderFrame.cpp \
            gui/common/MimeTextEdit.cpp \
            gui/common/UIStateHelper.cpp \
            gui/common/FloatingHelpBrowser.cpp \
            gui/common/SubscribeToolButton.cpp \
            gui/common/RsBanListToolButton.cpp \
            gui/common/FlowLayout.cpp \
            gui/common/PictureFlow.cpp \
            gui/common/StyledLabel.cpp \
            gui/common/StyledElidedLabel.cpp \
            gui/common/ToasterNotify.cpp \
            gui/style/RSStyle.cpp \
            gui/style/StyleDialog.cpp \
            gui/settings/RSPermissionMatrixWidget.cpp \
            gui/settings/rsharesettings.cpp \
            gui/settings/RsharePeerSettings.cpp \
            gui/settings/rsettings.cpp \
            gui/settings/rsettingswin.cpp \
            gui/settings/GeneralPage.cpp \
            gui/settings/AboutPage.cpp \
            gui/settings/ServerPage.cpp \
            gui/settings/NotifyPage.cpp \
            gui/settings/CryptoPage.cpp \
            gui/settings/PeoplePage.cpp \
            gui/settings/MessagePage.cpp \
            gui/settings/NewTag.cpp \
            gui/settings/ForumPage.cpp \
            gui/settings/PluginsPage.cpp \
            gui/settings/PluginItem.cpp \
            gui/settings/AppearancePage.cpp \
            gui/settings/FileAssociationsPage.cpp \
            gui/settings/SoundPage.cpp \
            gui/settings/TransferPage.cpp \
            gui/settings/ChatPage.cpp \
            gui/settings/ChannelPage.cpp \
            gui/settings/PostedPage.cpp \
            gui/settings/ServicePermissionsPage.cpp \
            gui/settings/AddFileAssociationDialog.cpp \
            gui/settings/GroupFrameSettingsWidget.cpp \
            gui/statusbar/peerstatus.cpp \
            gui/statusbar/natstatus.cpp \
            gui/statusbar/dhtstatus.cpp \
            gui/statusbar/torstatus.cpp \
            gui/statusbar/ratesstatus.cpp \
            gui/statusbar/hashingstatus.cpp \
            gui/statusbar/discstatus.cpp \
            gui/statusbar/SoundStatus.cpp \
            gui/statusbar/OpModeStatus.cpp \
            gui/statusbar/ToasterDisable.cpp \
            gui/statusbar/SysTrayStatus.cpp \
            gui/toaster/ToasterItem.cpp \
            gui/toaster/MessageToaster.cpp \
            gui/toaster/DownloadToaster.cpp \
            gui/toaster/OnlineToaster.cpp \
            gui/toaster/ChatToaster.cpp \
            gui/toaster/GroupChatToaster.cpp \
            gui/toaster/ChatLobbyToaster.cpp \
            gui/toaster/FriendRequestToaster.cpp \
            gui/advsearch/advancedsearchdialog.cpp \
            gui/advsearch/expressionwidget.cpp \
            gui/advsearch/guiexprelement.cpp \
            gui/elastic/graphwidget.cpp \
            gui/elastic/edge.cpp \
            gui/elastic/arrow.cpp \
            gui/elastic/elnode.cpp \
            gui/NewsFeed.cpp \
            gui/feeds/FeedItem.cpp \
            gui/feeds/FeedHolder.cpp \
            gui/feeds/GxsCircleItem.cpp \
            gui/feeds/PeerItem.cpp \
            gui/feeds/MsgItem.cpp \
            gui/feeds/ChatMsgItem.cpp \
            gui/feeds/SubFileItem.cpp \
            gui/feeds/AttachFileItem.cpp \
            gui/feeds/SecurityItem.cpp \
            gui/feeds/SecurityIpItem.cpp \
            gui/feeds/NewsFeedUserNotify.cpp \
            gui/connect/ConnectFriendWizard.cpp \
            gui/connect/ConnectProgressDialog.cpp \
            gui/connect/FriendRecommendDialog.cpp \
            gui/groups/CreateGroup.cpp \
            gui/GetStartedDialog.cpp \
            gui/statistics/BandwidthGraphWindow.cpp \
            gui/statistics/BandwidthStatsWidget.cpp \
            gui/statistics/DhtWindow.cpp \
            gui/statistics/TurtleRouterDialog.cpp \
            gui/statistics/TurtleRouterStatistics.cpp \
            gui/statistics/GlobalRouterStatistics.cpp \
            gui/statistics/GxsTransportStatistics.cpp \
            gui/statistics/StatisticsWindow.cpp \
            gui/statistics/BwCtrlWindow.cpp \
            gui/statistics/RttStatistics.cpp \
            gui/statistics/BWGraph.cpp \
    util/RsSyntaxHighlighter.cpp \
    util/imageutil.cpp \
    gui/NetworkDialog/pgpid_item_model.cpp \
    gui/NetworkDialog/pgpid_item_proxy.cpp \
    gui/common/RsCollection.cpp \
    util/retroshareWin32.cpp
#            gui/ForumsDialog.cpp \
#            gui/forums/ForumDetails.cpp \
#            gui/forums/EditForumDetails.cpp \
#            gui/forums/CreateForum.cpp \
#            gui/forums/CreateForumMsg.cpp \
#            gui/forums/ForumUserNotify.cpp \
#            gui/feeds/ForumNewItem.cpp \
#            gui/feeds/ForumMsgItem.cpp \
#            gui/ChannelFeed.cpp \
#            gui/channels/CreateChannel.cpp \
#            gui/channels/CreateChannelMsg.cpp \
#            gui/channels/ChannelDetails.cpp \
#            gui/channels/EditChanDetails.cpp \
#            gui/channels/ChannelUserNotify.cpp \
#            gui/feeds/ChanNewItem.cpp \
#            gui/feeds/ChanMsgItem.cpp \

RESOURCES += gui/images.qrc gui/icons.qrc lang/lang.qrc gui/help/content/content.qrc gui/emojione.qrc

TRANSLATIONS +=  \
            lang/retroshare_ca_ES.ts \
            lang/retroshare_cs.ts \
            lang/retroshare_da.ts \
            lang/retroshare_de.ts \
            lang/retroshare_el.ts \
            lang/retroshare_en.ts \
            lang/retroshare_es.ts \
            lang/retroshare_fi.ts \
            lang/retroshare_fr.ts \
            lang/retroshare_hu.ts \
            lang/retroshare_it.ts \
            lang/retroshare_ja_JP.ts \
            lang/retroshare_nl.ts \
            lang/retroshare_ko.ts \
            lang/retroshare_pl.ts \
            lang/retroshare_ru.ts \
            lang/retroshare_sv.ts \
            lang/retroshare_tr.ts \
            lang/retroshare_zh_CN.ts

unfinishedtranslations {

       TRANSLATIONS +=  \
            lang/retroshare_bg.ts \
            lang/retroshare_af.ts  \
            lang/retroshare_pt.ts  \
            lang/retroshare_sl.ts \
            lang/retroshare_sr.ts \
            lang/retroshare_zh_TW.ts

}

messenger {
     SOURCES +=       gui/MessengerWindow.cpp \
	 				  gui/common/FriendList.cpp
     HEADERS +=       gui/MessengerWindow.h \
	 				  gui/common/FriendList.h 
     FORMS   +=       gui/MessengerWindow.ui \
	 				  gui/common/FriendList.ui

	 DEFiNES += MESSENGER_WINDOW
}

# Shifted Qt4.4 dependancies to here.
#    qmake CONFIG=pluginmgr

pluginmgr {

        SOURCES += gui/PluginsPage.cpp \
            	gui/PluginManagerWidget.cpp \
            	gui/PluginManager.cpp

        HEADERS += gui/PluginsPage.h  \
            	gui/PluginManagerWidget.h \
            	gui/PluginManager.h

        DEFINES *= PLUGINMGR

}

idle {

HEADERS += idle/idle.h

SOURCES += idle/idle.cpp \
	   idle/idle_platform.cpp
}

framecatcher {

HEADERS += util/framecatcher.h

SOURCES += util/framecatcher.cpp

LIBS += -lxine

DEFINES *= CHANNELS_FRAME_CATCHER

}


# BELOW IS GXS Unfinished Services.
	
unfinished_services {
	
	DEPENDPATH += gui/unfinished \
	
	HEADERS += gui/unfinished/ApplicationWindow.h \

#		gui/unfinished/CalDialog.h \
#		gui/unfinished/ExampleDialog.h \
#		gui/unfinished/GamesDialog.h \
#		gui/unfinished/profile/ProfileView.h \
#		gui/unfinished/profile/ProfileEdit.h
#		gui/unfinished/StatisticDialog.h \
#		gui/unfinished/PhotoDialog.h \
#		gui/unfinished/PhotoShow.h \
	
	FORMS += gui/unfinished/ApplicationWindow.ui \

#		gui/unfinished/CalDialog.ui \
#		gui/unfinished/ExampleDialog.ui \
#		gui/unfinished/GamesDialog.ui \
#		gui/unfinished/profile/ProfileView.ui \
#		gui/unfinished/profile/ProfileEdit.ui
#		gui/unfinished/StatisticDialog.ui \
#		gui/unfinished/PhotoDialog.ui \
#		gui/unfinished/PhotoShow.ui \
	
	SOURCES += gui/unfinished/ApplicationWindow.cpp \

#		gui/unfinished/CalDialog.cpp \
#		gui/unfinished/ExampleDialog.cpp \
#		gui/unfinished/GamesDialog.cpp \
#		gui/unfinished/profile/ProfileView.cpp \
#		gui/unfinished/profile/ProfileEdit.cpp
#		gui/unfinished/StatisticDialog.cpp \
#		gui/unfinished/PhotoDialog.cpp \
#		gui/unfinished/PhotoShow.cpp \
	
	DEFINES *= UNFINISHED
}
	
	
gxsphotoshare {
	#DEFINES += RS_USE_PHOTOSHARE # to enable in unfinished.
	DEFINES += RS_USE_PHOTO # enable in MainWindow

	HEADERS += \
		gui/PhotoShare/AlbumGroupDialog.h \
		gui/PhotoShare/AlbumExtra.h \
		gui/PhotoShare/PhotoDrop.h \
		gui/PhotoShare/AlbumItem.h \
		gui/PhotoShare/AlbumDialog.h \
		gui/PhotoShare/PhotoItem.h \
		gui/PhotoShare/PhotoShareItemHolder.h \
		gui/PhotoShare/PhotoShare.h \
		gui/PhotoShare/PhotoSlideShow.h \
		gui/PhotoShare/PhotoDialog.h

	FORMS += \
		gui/PhotoShare/AlbumExtra.ui \
		gui/PhotoShare/PhotoItem.ui \
		gui/PhotoShare/PhotoDialog.ui \
		gui/PhotoShare/AlbumItem.ui \
		gui/PhotoShare/AlbumDialog.ui \
		gui/PhotoShare/PhotoShare.ui \
		gui/PhotoShare/PhotoSlideShow.ui

	SOURCES += \
		gui/PhotoShare/AlbumGroupDialog.cpp \
		gui/PhotoShare/AlbumExtra.cpp \
		gui/PhotoShare/PhotoItem.cpp \
		gui/PhotoShare/PhotoDialog.cpp \
		gui/PhotoShare/PhotoDrop.cpp \
		gui/PhotoShare/AlbumItem.cpp \
		gui/PhotoShare/AlbumDialog.cpp \
		gui/PhotoShare/PhotoShareItemHolder.cpp \
		gui/PhotoShare/PhotoShare.cpp \
		gui/PhotoShare/PhotoSlideShow.cpp

	RESOURCES += gui/PhotoShare/Photo_images.qrc

}
	
	
wikipoos {
	DEFINES += RS_USE_WIKI
	
	DEPENDPATH += ../../supportlibs/pegmarkdown
	INCLUDEPATH += ../../supportlibs/pegmarkdown
	
	HEADERS += gui/WikiPoos/WikiDialog.h \
		gui/WikiPoos/WikiAddDialog.h \
		gui/WikiPoos/WikiEditDialog.h \
		gui/gxs/WikiGroupDialog.h \

	FORMS += gui/WikiPoos/WikiDialog.ui \
		gui/WikiPoos/WikiAddDialog.ui \
		gui/WikiPoos/WikiEditDialog.ui \
	
	SOURCES += gui/WikiPoos/WikiDialog.cpp \
		gui/WikiPoos/WikiAddDialog.cpp \
		gui/WikiPoos/WikiEditDialog.cpp \
		gui/gxs/WikiGroupDialog.cpp \

	RESOURCES += gui/WikiPoos/Wiki_images.qrc

}
	
	
	
gxsthewire {
	
	DEFINES += RS_USE_WIRE

	HEADERS += gui/TheWire/PulseItem.h \
		gui/TheWire/PulseDetails.h \
		gui/TheWire/WireDialog.h \
		gui/TheWire/WireGroupItem.h \
		gui/TheWire/WireGroupDialog.h \
		gui/TheWire/PulseAddDialog.h \
	
	FORMS += gui/TheWire/PulseItem.ui \
		gui/TheWire/PulseDetails.ui \
		gui/TheWire/WireGroupItem.ui \
		gui/TheWire/WireDialog.ui \
		gui/TheWire/PulseAddDialog.ui \
	
	SOURCES += gui/TheWire/PulseItem.cpp \
		gui/TheWire/PulseDetails.cpp \
		gui/TheWire/WireDialog.cpp \
		gui/TheWire/WireGroupItem.cpp \
		gui/TheWire/WireGroupDialog.cpp \
		gui/TheWire/PulseAddDialog.cpp \

	RESOURCES += gui/TheWire/TheWire_images.qrc
}

identities {
	
	HEADERS +=  \
		gui/Identity/IdDialog.h \
		gui/Identity/IdEditDialog.h \
		gui/Identity/IdDetailsDialog.h \
	
	FORMS += gui/Identity/IdDialog.ui \
		gui/Identity/IdEditDialog.ui \
		gui/Identity/IdDetailsDialog.ui \

	SOURCES +=  \
		gui/Identity/IdDialog.cpp \
		gui/Identity/IdEditDialog.cpp \
		gui/Identity/IdDetailsDialog.cpp \
	
}
	
gxscircles {
	DEFINES += RS_USE_CIRCLES
#	DEFINES += RS_USE_NEW_PEOPLE_DIALOG

	HEADERS +=  \
		gui/Circles/CirclesDialog.h \
		gui/Circles/CreateCircleDialog.h \
	
	FORMS += gui/Circles/CirclesDialog.ui \
		gui/Circles/CreateCircleDialog.ui \
	
	SOURCES +=  \
		gui/Circles/CirclesDialog.cpp \
		gui/Circles/CreateCircleDialog.cpp \

	HEADERS += gui/People/PeopleDialog.h
	HEADERS += gui/People/CircleWidget.h
	HEADERS += gui/People/IdentityWidget.h

	FORMS   += gui/People/PeopleDialog.ui 
	FORMS   += gui/People/CircleWidget.ui
	FORMS   += gui/People/IdentityWidget.ui

	SOURCES += gui/People/PeopleDialog.cpp 
	SOURCES += gui/People/CircleWidget.cpp
	SOURCES += gui/People/IdentityWidget.cpp

#HEADERS += gui/People/IdentityItem.h
#HEADERS += gui/People/CircleItem.h
#HEADERS += gui/People/GroupListView.h
#SOURCES += gui/People/GroupListView.cpp
#SOURCES += gui/People/IdentityItem.cpp
#SOURCES += gui/People/CircleItem.cpp


}
	
	
gxsforums {

	HEADERS += gui/gxsforums/GxsForumsDialog.h \
		gui/gxsforums/GxsForumGroupDialog.h \
		gui/gxsforums/CreateGxsForumMsg.h \
		gui/gxsforums/GxsForumThreadWidget.h \
		gui/gxsforums/GxsForumModel.h \
		gui/gxsforums/GxsForumUserNotify.h \
		gui/gxsforums/ForumMessageView.h \
		gui/feeds/GxsForumGroupItem.h \
		gui/feeds/GxsForumMsgItem.h

	FORMS += gui/gxsforums/CreateGxsForumMsg.ui \
		gui/gxsforums/GxsForumThreadWidget.ui \
		gui/gxsforums/ForumMessageView.ui \
		gui/feeds/GxsForumGroupItem.ui \
		gui/feeds/GxsForumMsgItem.ui

	SOURCES += gui/gxsforums/GxsForumsDialog.cpp \
		gui/gxsforums/GxsForumGroupDialog.cpp \
		gui/gxsforums/CreateGxsForumMsg.cpp \
		gui/gxsforums/GxsForumThreadWidget.cpp \
		gui/gxsforums/GxsForumModel.cpp \
		gui/gxsforums/GxsForumUserNotify.cpp \
		gui/gxsforums/ForumMessageView.cpp \
		gui/feeds/GxsForumGroupItem.cpp \
		gui/feeds/GxsForumMsgItem.cpp
}
	
	
gxschannels {
	
	HEADERS += gui/gxschannels/GxsChannelDialog.h \
		gui/gxschannels/GxsChannelGroupDialog.h \
		gui/gxschannels/CreateGxsChannelMsg.h \
		gui/gxschannels/GxsChannelPostsWidget.h \
		gui/gxschannels/GxsChannelFilesWidget.h \
		gui/gxschannels/GxsChannelFilesStatusWidget.h \
		gui/feeds/GxsChannelGroupItem.h \
		gui/feeds/GxsChannelPostItem.h \
		gui/gxschannels/GxsChannelUserNotify.h
	
	FORMS += gui/gxschannels/GxsChannelPostsWidget.ui \
		gui/gxschannels/GxsChannelFilesWidget.ui \
		gui/gxschannels/GxsChannelFilesStatusWidget.ui \
		gui/gxschannels/CreateGxsChannelMsg.ui \
		gui/feeds/GxsChannelGroupItem.ui \
		gui/feeds/GxsChannelPostItem.ui
	
	SOURCES += gui/gxschannels/GxsChannelDialog.cpp \
		gui/gxschannels/GxsChannelPostsWidget.cpp \
		gui/gxschannels/GxsChannelFilesWidget.cpp \
		gui/gxschannels/GxsChannelFilesStatusWidget.cpp \
		gui/gxschannels/GxsChannelGroupDialog.cpp \
		gui/gxschannels/CreateGxsChannelMsg.cpp \
		gui/feeds/GxsChannelGroupItem.cpp \
		gui/feeds/GxsChannelPostItem.cpp \
		gui/gxschannels/GxsChannelUserNotify.cpp
}
	
	
posted {
	
	HEADERS += gui/Posted/PostedDialog.h \
		gui/Posted/PostedListWidget.h \
		gui/Posted/PostedItem.h \
		gui/Posted/PostedCardView.h \
		gui/Posted/PostedGroupDialog.h \
		gui/feeds/PostedGroupItem.h \
		gui/Posted/PostedCreatePostDialog.h \
		gui/Posted/PhotoView.h \
		gui/Posted/PostedUserNotify.h

		#gui/Posted/PostedCreateCommentDialog.h \
		#gui/Posted/PostedComments.h \
	
	FORMS += gui/Posted/PostedListWidget.ui \
		gui/feeds/PostedGroupItem.ui \
		gui/Posted/PostedItem.ui \
		gui/Posted/PostedCardView.ui \
		gui/Posted/PostedCreatePostDialog.ui \
		gui/Posted/PhotoView.ui 
		#gui/Posted/PostedDialog.ui \
		#gui/Posted/PostedComments.ui \
		#gui/Posted/PostedCreateCommentDialog.ui
	
	SOURCES += gui/Posted/PostedDialog.cpp \
		gui/Posted/PostedListWidget.cpp \
		gui/feeds/PostedGroupItem.cpp \
		gui/Posted/PostedItem.cpp \
		gui/Posted/PostedCardView.cpp \
		gui/Posted/PostedGroupDialog.cpp \
		gui/Posted/PostedCreatePostDialog.cpp \
		gui/Posted/PhotoView.cpp \
		gui/Posted/PostedUserNotify.cpp

		#gui/Posted/PostedDialog.cpp \
		#gui/Posted/PostedComments.cpp \
		#gui/Posted/PostedCreateCommentDialog.cpp
	
	RESOURCES += gui/Posted/Posted_images.qrc
}
	
gxsgui {
	
	HEADERS += gui/gxs/GxsGroupDialog.h \
		gui/gxs/GxsIdDetails.h \
		gui/gxs/GxsIdChooser.h \
		gui/gxs/GxsIdLabel.h \
		gui/gxs/GxsCircleChooser.h \
		gui/gxs/GxsCircleLabel.h \
		gui/gxs/GxsIdTreeWidgetItem.h \
		gui/gxs/GxsCommentTreeWidget.h \
		gui/gxs/GxsCommentContainer.h \
		gui/gxs/GxsCommentDialog.h \
		gui/gxs/GxsCreateCommentDialog.h \
		gui/gxs/GxsGroupFrameDialog.h \
		gui/gxs/GxsMessageFrameWidget.h \
		gui/gxs/GxsMessageFramePostWidget.h \
		gui/gxs/GxsGroupFeedItem.h \
		gui/gxs/GxsFeedItem.h \
		gui/gxs/GxsGroupShareKey.h \
		gui/gxs/GxsUserNotify.h \
		gui/gxs/GxsFeedWidget.h \
		util/TokenQueue.h \
		util/RsGxsUpdateBroadcast.h \
	
#		gui/gxs/GxsMsgDialog.h \
	
	FORMS += gui/gxs/GxsGroupDialog.ui \
		gui/gxs/GxsCommentContainer.ui \
		gui/gxs/GxsCommentDialog.ui \
		gui/gxs/GxsCreateCommentDialog.ui \
		gui/gxs/GxsGroupFrameDialog.ui\
		gui/gxs/GxsGroupShareKey.ui 
#		gui/gxs/GxsMsgDialog.ui \
#		gui/gxs/GxsCommentTreeWidget.ui 
	
	SOURCES += gui/gxs/GxsGroupDialog.cpp \
		gui/gxs/GxsIdDetails.cpp \
		gui/gxs/GxsIdChooser.cpp \
		gui/gxs/GxsIdLabel.cpp \
		gui/gxs/GxsCircleChooser.cpp \
		gui/gxs/GxsGroupShareKey.cpp \
		gui/gxs/GxsCircleLabel.cpp \
		gui/gxs/GxsIdTreeWidgetItem.cpp \
		gui/gxs/GxsCommentTreeWidget.cpp \
		gui/gxs/GxsCommentContainer.cpp \
		gui/gxs/GxsCommentDialog.cpp \
		gui/gxs/GxsCreateCommentDialog.cpp \
		gui/gxs/GxsGroupFrameDialog.cpp \
		gui/gxs/GxsMessageFrameWidget.cpp \
		gui/gxs/GxsMessageFramePostWidget.cpp \
		gui/gxs/GxsGroupFeedItem.cpp \
		gui/gxs/GxsFeedItem.cpp \
		gui/gxs/GxsUserNotify.cpp \
		gui/gxs/GxsFeedWidget.cpp \
		util/TokenQueue.cpp \
		util/RsGxsUpdateBroadcast.cpp \
	
#		gui/gxs/GxsMsgDialog.cpp \
	
	
}


wikipoos {
	HEADERS += \
		gui/gxs/RsGxsUpdateBroadcastBase.h \
		gui/gxs/RsGxsUpdateBroadcastWidget.h \
		gui/gxs/RsGxsUpdateBroadcastPage.h 

	SOURCES += \
		gui/gxs/RsGxsUpdateBroadcastBase.cpp \
		gui/gxs/RsGxsUpdateBroadcastWidget.cpp \
		gui/gxs/RsGxsUpdateBroadcastPage.cpp \
}
