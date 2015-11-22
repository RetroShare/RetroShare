!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TEMPLATE = app
QT     += network xml
CONFIG += qt gui uic qrc resources idle bitdht
CONFIG += link_prl
TARGET = RetroShare06

# Plz never commit the .pro with these flags enabled.
# Use this flag when developping new features only.
#
#CONFIG += unfinished
#CONFIG += debug

#QMAKE_CFLAGS += -fmudflap 
#LIBS *= /usr/lib/gcc/x86_64-linux-gnu/4.4/libmudflap.a /usr/lib/gcc/x86_64-linux-gnu/4.4/libmudflapth.a

greaterThan(QT_MAJOR_VERSION, 4) {
	# Qt 5
	QT     += uitools widgets multimedia printsupport
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

# Other Disabled Bits.
#CONFIG += framecatcher
#CONFIG += blogs

DEFINES += RS_RELEASE_VERSION
RCC_DIR = temp/qrc
UI_DIR  = temp/ui
MOC_DIR = temp/moc

#CONFIG += debug
debug {
	QMAKE_CFLAGS += -g
	QMAKE_CXXFLAGS -= -O2
	QMAKE_CXXFLAGS += -O0
	QMAKE_CFLAGS -= -O2
	QMAKE_CFLAGS += -O0
}

DEPENDPATH *= retroshare-gui
INCLUDEPATH *= retroshare-gui

# treat warnings as error for better removing
#QMAKE_CFLAGS += -Werror
#QMAKE_CXXFLAGS += -Werror

################################# Linux ##########################################
# Put lib dir in QMAKE_LFLAGS so it appears before -L/usr/lib
linux-* {
	CONFIG += link_pkgconfig
	#CONFIG += version_detail_bash_script
	QMAKE_CXXFLAGS *= -D_FILE_OFFSET_BITS=64

	PKGCONFIG *= x11 xscrnsaver

	LIBS *= -rdynamic
	DEFINES *= HAVE_XSS # for idle time, libx screensaver extensions
	DEFINES *= UBUNTU
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
	desktop_files.files = ../../data/retroshare06.desktop
	INSTALLS += desktop_files

	pixmap_files.path = "$${PREFIX}/share/pixmaps"
	pixmap_files.files = ../../data/retroshare06.xpm
	INSTALLS += pixmap_files

}

linux-g++ {
	OBJECTS_DIR = temp/linux-g++/obj
}

linux-g++-64 {
	OBJECTS_DIR = temp/linux-g++-64/obj
}

version_detail_bash_script {
	linux-* {
		DEFINES += ADD_LIBRETROSHARE_VERSION_INFO
		QMAKE_EXTRA_TARGETS += write_version_detail
		PRE_TARGETDEPS = write_version_detail
		write_version_detail.commands = ./version_detail.sh
	}
	win32 {
		QMAKE_EXTRA_TARGETS += write_version_detail
		PRE_TARGETDEPS = write_version_detail
		write_version_detail.commands = $$PWD/version_detail.bat
	}
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

win32 {
	CONFIG(debug, debug|release) {
		# show console output
		CONFIG += console
	}

	# Switch on extra warnings
	QMAKE_CFLAGS += -Wextra
	QMAKE_CXXFLAGS += -Wextra

	# solve linker warnings because of the order of the libraries
	QMAKE_LFLAGS += -Wl,--start-group

	# Switch off optimization for release version
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O0
	QMAKE_CFLAGS_RELEASE -= -O2
	QMAKE_CFLAGS_RELEASE += -O0

	# Switch on optimization for debug version
	#QMAKE_CXXFLAGS_DEBUG += -O2
	#QMAKE_CFLAGS_DEBUG += -O2

	OBJECTS_DIR = temp/obj
	#LIBS += -L"D/Qt/2009.03/qt/plugins/imageformats"
	#QTPLUGIN += qjpeg

	for(lib, LIB_DIR):LIBS += -L"$$lib"
	for(bin, BIN_DIR):LIBS += -L"$$bin"

	LIBS += -lssl -lcrypto -lpthread -lminiupnpc -lz -lws2_32
	LIBS += -luuid -lole32 -liphlpapi -lcrypt32 -lgdi32
	LIBS += -lwinmm
	RC_FILE = gui/images/retroshare_win.rc

	# export symbols for the plugins
	LIBS += -Wl,--export-all-symbols,--out-implib,lib/libretroshare-gui.a

	# create lib directory
	QMAKE_PRE_LINK = $(CHK_DIR_EXISTS) lib $(MKDIR) lib

	DEFINES *= WINDOWS_SYS WIN32_LEAN_AND_MEAN _USE_32BIT_TIME_T

	DEPENDPATH += . $$INC_DIR
	INCLUDEPATH += . $$INC_DIR

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

	CONFIG += version_detail_bash_script
        LIBS += -lssl -lcrypto -lz 
        #LIBS += -lssl -lcrypto -lz -lgpgme -lgpg-error -lassuan
	LIBS += /usr/local/lib/libminiupnpc.a
	LIBS += -framework CoreFoundation
	LIBS += -framework Security
	LIBS += -framework Carbon
	INCLUDEPATH += . /usr/local/include
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



############################## Common stuff ######################################

# On Linux systems that alredy have libssl and libcrypto it is advisable
# to rename the patched version of SSL to something like libsslxpgp.a and libcryptoxpg.a

# ###########################################

DEPENDPATH += . ../../libretroshare/src/
INCLUDEPATH += ../../libretroshare/src/

PRE_TARGETDEPS *= ../../libretroshare/src/lib/libretroshare.a
LIBS *= ../../libretroshare/src/lib/libretroshare.a

wikipoos {
	PRE_TARGETDEPS *= ../../supportlibs/pegmarkdown/lib/libpegmarkdown.a
	LIBS *= ../../supportlibs/pegmarkdown/lib/libpegmarkdown.a
}

# webinterface
DEPENDPATH += ../../libresapi/src
INCLUDEPATH += ../../libresapi/src
PRE_TARGETDEPS *= ../../libresapi/src/lib/libresapi.a
LIBS += ../../libresapi/src/lib/libresapi.a

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
            gui/NetworkDialog.h \
            gui/GenCertDialog.h \
            gui/linetypes.h \
            gui/mainpagestack.h \
            gui/MainWindow.h \
            gui/RSHumanReadableDelegate.h \
            gui/AboutDialog.h \
            gui/NetworkView.h \
            gui/MessengerWindow.h \
            gui/FriendsDialog.h \
            gui/ServicePermissionDialog.h \
            gui/RemoteDirModel.h \
            gui/RetroShareLink.h \
            gui/SearchTreeWidget.h \
            gui/SearchDialog.h \
            gui/SharedFilesDialog.h \
            gui/ShareManager.h \
            gui/ShareDialog.h \
#            gui/SFListDelegate.h \
            gui/SoundManager.h \
            gui/HelpDialog.h \
            gui/LogoBar.h \
            gui/common/AvatarDialog.h \
            gui/FileTransfer/xprogressbar.h \
            gui/FileTransfer/DetailsDialog.h \
            gui/FileTransfer/FileTransferInfoWidget.h \
            gui/FileTransfer/DLListDelegate.h \
            gui/FileTransfer/ULListDelegate.h \
            gui/FileTransfer/TransfersDialog.h \
            gui/statistics/TurtleRouterDialog.h \
            gui/statistics/TurtleRouterStatistics.h \
            gui/statistics/dhtgraph.h \
            gui/statistics/BandwidthGraphWindow.h \
            gui/statistics/turtlegraph.h \
            gui/statistics/BandwidthStatsWidget.h \
            gui/statistics/DhtWindow.h \
            gui/statistics/GlobalRouterStatistics.h \
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
            util/win32.h \
            util/RetroStyleLabel.h \
            util/dllexport.h \
            util/NonCopyable.h \
            util/rsutildll.h \
            util/dllexport.h \
            util/global.h \
            util/rsqtutildll.h \
            util/Interface.h \
            util/PixmapMerging.h \
            util/MouseEventFilter.h \
            util/EventFilter.h \
            util/EventReceiver.h \
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
            gui/msgs/MessageInterface.h \
            gui/msgs/MessageComposer.h \
            gui/msgs/MessageWindow.h \
            gui/msgs/MessageWidget.h \
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
            gui/settings/DirectoriesPage.h \
            gui/settings/ServerPage.h \
            gui/settings/NetworkPage.h \
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
            gui/settings/RelayPage.h \
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
            gui/common/RsCollectionFile.h \
            gui/common/RsCollectionDialog.h \
            gui/common/rwindow.h \
            gui/common/html.h \
            gui/common/AvatarDefs.h \
            gui/common/GroupFlagsWidget.h \
            gui/common/GroupSelectionBox.h \
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
            gui/common/RSItemDelegate.h \
            gui/common/PeerDefs.h \
            gui/common/FilesDefs.h \
            gui/common/PopularityDefs.h \
            gui/common/RsBanListDefs.h \
            gui/common/GroupTreeWidget.h \
            gui/common/RSTreeView.h \
            gui/common/AvatarWidget.h \
            gui/common/FriendList.h \
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
            gui/MessagesDialog.h \
            gui/help/browser/helpbrowser.h \
            gui/help/browser/helptextbrowser.h \
            gui/statusbar/peerstatus.h \
            gui/statusbar/natstatus.h \
            gui/statusbar/dhtstatus.h \
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
            gui/elastic/node.h \
            gui/NewsFeed.h \
            gui/feeds/FeedItem.h \
            gui/feeds/FeedHolder.h \
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
    gui/settings/WebuiPage.h \
    gui/statistics/BWGraph.h

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
            gui/GenCertDialog.ui \
            gui/AboutDialog.ui \
            gui/QuickStartWizard.ui \
            gui/NetworkDialog.ui \
            gui/common/AvatarDialog.ui \
            gui/FileTransfer/TransfersDialog.ui \
            gui/FileTransfer/DetailsDialog.ui \
            gui/MainWindow.ui \
            gui/NetworkView.ui \
            gui/MessengerWindow.ui \
            gui/FriendsDialog.ui \
            gui/SearchDialog.ui \
            gui/SharedFilesDialog.ui \
            gui/ShareManager.ui \
            gui/ShareDialog.ui \
            gui/MessagesDialog.ui \
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
            gui/msgs/MessageComposer.ui \
            gui/msgs/MessageWindow.ui\
            gui/msgs/MessageWidget.ui\
            gui/settings/settings.ui \
            gui/settings/GeneralPage.ui \
            gui/settings/DirectoriesPage.ui \
            gui/settings/ServerPage.ui \
            gui/settings/NetworkPage.ui \
            gui/settings/NotifyPage.ui \
            gui/settings/CryptoPage.ui \
            gui/settings/MessagePage.ui \
            gui/settings/NewTag.ui \
            gui/settings/ForumPage.ui \
            gui/settings/PluginsPage.ui \
            gui/settings/AppearancePage.ui \
            gui/settings/TransferPage.ui \
            gui/settings/SoundPage.ui \
            gui/settings/ChatPage.ui \
            gui/settings/ChannelPage.ui \
            gui/settings/PostedPage.ui \
            gui/settings/RelayPage.ui \
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
            gui/common/FriendList.ui \
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
            gui/statistics/StatisticsWindow.ui \
            gui/statistics/BwCtrlWindow.ui \
            gui/statistics/RttStatistics.ui \
            gui/GetStartedDialog.ui \
    gui/settings/WebuiPage.ui

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
            gui/QuickStartWizard.cpp \
            gui/StartDialog.cpp \
            gui/GenCertDialog.cpp \
            gui/NetworkDialog.cpp \
            gui/mainpagestack.cpp \
            gui/MainWindow.cpp \
            gui/NetworkView.cpp \
            gui/MessengerWindow.cpp \
            gui/FriendsDialog.cpp \
            gui/ServicePermissionDialog.cpp \
            gui/RemoteDirModel.cpp \
            gui/RsAutoUpdatePage.cpp \
            gui/RetroShareLink.cpp \
            gui/SearchTreeWidget.cpp \
            gui/SearchDialog.cpp \
            gui/SharedFilesDialog.cpp \
            gui/ShareManager.cpp \
            gui/ShareDialog.cpp \
#            gui/SFListDelegate.cpp \
            gui/SoundManager.cpp \
            gui/MessagesDialog.cpp \
            gui/im_history/ImHistoryBrowser.cpp \
            gui/im_history/IMHistoryItemDelegate.cpp \
            gui/im_history/IMHistoryItemPainter.cpp \
            gui/help/browser/helpbrowser.cpp \
            gui/help/browser/helptextbrowser.cpp \
            gui/FileTransfer/TransfersDialog.cpp \
            gui/FileTransfer/FileTransferInfoWidget.cpp \
            gui/FileTransfer/DLListDelegate.cpp \
            gui/FileTransfer/ULListDelegate.cpp \
            gui/FileTransfer/xprogressbar.cpp \
            gui/FileTransfer/DetailsDialog.cpp \
            gui/FileTransfer/TransferUserNotify.cpp \
            gui/MainPage.cpp \
            gui/HelpDialog.cpp \
            gui/LogoBar.cpp \
            lang/languagesupport.cpp \
            util/RsProtectedTimer.cpp \
            util/stringutil.cpp \
            util/RsNetUtil.cpp \
            util/DateTime.cpp \
            util/win32.cpp \
            util/RetroStyleLabel.cpp \
            util/WidgetBackgroundImage.cpp \
            util/NonCopyable.cpp \
            util/PixmapMerging.cpp \
            util/MouseEventFilter.cpp \
            util/EventFilter.cpp \
            util/EventReceiver.cpp \
            util/Widget.cpp \
            util/RsAction.cpp \
            util/printpreview.cpp \
            util/log.cpp \
            util/misc.cpp \
            util/HandleRichText.cpp \
            util/ObjectPainter.cpp \
            util/RsFile.cpp \
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
            gui/msgs/MessageComposer.cpp \
            gui/msgs/MessageWidget.cpp \
            gui/msgs/MessageWindow.cpp \
            gui/msgs/TagsMenu.cpp \
            gui/msgs/MessageUserNotify.cpp \
            gui/common/RsButtonOnText.cpp \
            gui/common/RSGraphWidget.cpp \
            gui/common/ElidedLabel.cpp \
            gui/common/vmessagebox.cpp \
            gui/common/RsCollectionFile.cpp \
            gui/common/RsCollectionDialog.cpp \
            gui/common/RsUrlHandler.cpp \
            gui/common/rwindow.cpp \
            gui/common/html.cpp \
            gui/common/AvatarDefs.cpp \
            gui/common/AvatarDialog.cpp \
            gui/common/GroupFlagsWidget.cpp \
            gui/common/GroupSelectionBox.cpp \
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
            gui/common/RSItemDelegate.cpp \
            gui/common/PeerDefs.cpp \
            gui/common/FilesDefs.cpp \
            gui/common/PopularityDefs.cpp \
            gui/common/RsBanListDefs.cpp \
            gui/common/GroupTreeWidget.cpp \
            gui/common/RSTreeView.cpp \
            gui/common/AvatarWidget.cpp \
            gui/common/FriendList.cpp \
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
            gui/settings/DirectoriesPage.cpp \
            gui/settings/ServerPage.cpp \
            gui/settings/NetworkPage.cpp \
            gui/settings/NotifyPage.cpp \
            gui/settings/CryptoPage.cpp \
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
            gui/settings/RelayPage.cpp \
            gui/settings/ServicePermissionsPage.cpp \
            gui/settings/AddFileAssociationDialog.cpp \
            gui/settings/GroupFrameSettingsWidget.cpp \
                gui/settings/WebuiPage.cpp \
            gui/statusbar/peerstatus.cpp \
            gui/statusbar/natstatus.cpp \
            gui/statusbar/dhtstatus.cpp \
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
            gui/elastic/node.cpp \
            gui/NewsFeed.cpp \
            gui/feeds/FeedItem.cpp \
            gui/feeds/FeedHolder.cpp \
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
            gui/groups/CreateGroup.cpp \
            gui/GetStartedDialog.cpp \
            gui/statistics/BandwidthGraphWindow.cpp \
            gui/statistics/BandwidthStatsWidget.cpp \
            gui/statistics/DhtWindow.cpp \
            gui/statistics/TurtleRouterDialog.cpp \
            gui/statistics/TurtleRouterStatistics.cpp \
            gui/statistics/GlobalRouterStatistics.cpp \
            gui/statistics/StatisticsWindow.cpp \
            gui/statistics/BwCtrlWindow.cpp \
            gui/statistics/RttStatistics.cpp \
            gui/statistics/BWGraph.cpp

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

RESOURCES += gui/images.qrc gui/icons.qrc lang/lang.qrc gui/help/content/content.qrc

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

#blogs {
#
#DEPENDPATH += gui/unfinished \
#
#HEADERS += gui/unfinished/blogs/BlogsDialog.h \
#           gui/unfinished/blogs/CreateBlog.h \
#           gui/unfinished/blogs/CreateBlogMsg.h \
#           gui/unfinished/blogs/BlogsMsgItem.h \
#           gui/unfinished/blogs/BlogDetails.h \
#           gui/feeds/BlogNewItem.h \
#           gui/feeds/BlogMsgItem.h \
#
#FORMS += gui/unfinished/blogs/BlogsDialog.ui \
#         gui/unfinished/blogs/CreateBlog.ui \
#         gui/unfinished/blogs/CreateBlogMsg.ui \
#         gui/unfinished/blogs/BlogsMsgItem.ui \
#         gui/unfinished/blogs/BlogDetails.ui \
#         gui/feeds/BlogNewItem.ui \
#         gui/feeds/BlogMsgItem.ui \
#
#SOURCES += gui/unfinished/blogs/BlogsDialog.cpp \
#           gui/unfinished/blogs/CreateBlog.cpp \
#           gui/unfinished/blogs/CreateBlogMsg.cpp \
#           gui/unfinished/blogs/BlogsMsgItem.cpp \
#           gui/unfinished/blogs/BlogDetails.cpp \
#           gui/feeds/BlogNewItem.cpp \
#           gui/feeds/BlogMsgItem.cpp \
#
#DEFINES += BLOGS
#}

# use_links {
# HEADERS += gui/AddLinksDialog.h \
#            gui/LinksDialog.h
# 
# FORMS += gui/AddLinksDialog.ui \
#          gui/LinksDialog.ui
# 
# SOURCES += gui/AddLinksDialog.cpp \
#            gui/LinksDialog.cpp
# 
# DEFINES += RS_USE_LINKS
# }


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
	#DEFINES += RS_USE_PHOTOSHARE

	HEADERS += \
		gui/PhotoShare/PhotoDrop.h \
		gui/PhotoShare/AlbumItem.h \
		gui/PhotoShare/AlbumDialog.h \
		gui/PhotoShare/AlbumCreateDialog.h \
		gui/PhotoShare/PhotoItem.h \
		gui/PhotoShare/PhotoShareItemHolder.h \
		gui/PhotoShare/PhotoShare.h \
		gui/PhotoShare/PhotoSlideShow.h \
		gui/PhotoShare/PhotoDialog.h \
		gui/PhotoShare/PhotoCommentItem.h \
		gui/PhotoShare/AddCommentDialog.h
	
	FORMS += \
		gui/PhotoShare/PhotoItem.ui \
		gui/PhotoShare/PhotoDialog.ui \
		gui/PhotoShare/AlbumItem.ui \
		gui/PhotoShare/AlbumDialog.ui \
		gui/PhotoShare/AlbumCreateDialog.ui \
		gui/PhotoShare/PhotoShare.ui \
		gui/PhotoShare/PhotoSlideShow.ui \
		gui/PhotoShare/PhotoCommentItem.ui \
		gui/PhotoShare/AddCommentDialog.ui
	
	SOURCES += \
		gui/PhotoShare/PhotoItem.cpp \
		gui/PhotoShare/PhotoDialog.cpp \
		gui/PhotoShare/PhotoDrop.cpp \
		gui/PhotoShare/AlbumItem.cpp \
		gui/PhotoShare/AlbumDialog.cpp \
		gui/PhotoShare/AlbumCreateDialog.cpp \
		gui/PhotoShare/PhotoShareItemHolder.cpp \
		gui/PhotoShare/PhotoShare.cpp \
		gui/PhotoShare/PhotoSlideShow.cpp \
		gui/PhotoShare/PhotoCommentItem.cpp \
		gui/PhotoShare/AddCommentDialog.cpp
	
	RESOURCES += gui/PhotoShare/Photo_images.qrc

}
	
	
wikipoos {
	
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
	
	HEADERS += gui/TheWire/PulseItem.h \
		gui/TheWire/WireDialog.h \
		gui/TheWire/PulseAddDialog.h \
	
	FORMS += gui/TheWire/PulseItem.ui \
		gui/TheWire/WireDialog.ui \
		gui/TheWire/PulseAddDialog.ui \
	
	SOURCES += gui/TheWire/PulseItem.cpp \
		gui/TheWire/WireDialog.cpp \
		gui/TheWire/PulseAddDialog.cpp \
	
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
		gui/gxsforums/GxsForumsFillThread.h \
		gui/gxsforums/GxsForumUserNotify.h \
		gui/feeds/GxsForumGroupItem.h \
		gui/feeds/GxsForumMsgItem.h

	FORMS += gui/gxsforums/CreateGxsForumMsg.ui \
		gui/gxsforums/GxsForumThreadWidget.ui \
		gui/feeds/GxsForumGroupItem.ui \
		gui/feeds/GxsForumMsgItem.ui

	SOURCES += gui/gxsforums/GxsForumsDialog.cpp \
		gui/gxsforums/GxsForumGroupDialog.cpp \
		gui/gxsforums/CreateGxsForumMsg.cpp \
		gui/gxsforums/GxsForumThreadWidget.cpp \
		gui/gxsforums/GxsForumsFillThread.cpp \
		gui/gxsforums/GxsForumUserNotify.cpp \
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
		gui/Posted/PostedGroupDialog.h \
		gui/feeds/PostedGroupItem.h \
		gui/Posted/PostedCreatePostDialog.h \
		gui/Posted/PostedUserNotify.h

		#gui/Posted/PostedCreateCommentDialog.h \
		#gui/Posted/PostedComments.h \
	
	FORMS += gui/Posted/PostedListWidget.ui \
		gui/feeds/PostedGroupItem.ui \
		gui/Posted/PostedItem.ui \
		gui/Posted/PostedCreatePostDialog.ui \

		#gui/Posted/PostedDialog.ui \
		#gui/Posted/PostedComments.ui \
		#gui/Posted/PostedCreateCommentDialog.ui
	
	SOURCES += gui/Posted/PostedDialog.cpp \
		gui/Posted/PostedListWidget.cpp \
		gui/feeds/PostedGroupItem.cpp \
		gui/Posted/PostedItem.cpp \
		gui/Posted/PostedGroupDialog.cpp \
		gui/Posted/PostedCreatePostDialog.cpp \
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
		gui/gxs/RsGxsUpdateBroadcastBase.h \
		gui/gxs/RsGxsUpdateBroadcastWidget.h \
		gui/gxs/RsGxsUpdateBroadcastPage.h \
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
		gui/gxs/RsGxsUpdateBroadcastBase.cpp \
		gui/gxs/RsGxsUpdateBroadcastWidget.cpp \
		gui/gxs/RsGxsUpdateBroadcastPage.cpp \
		gui/gxs/GxsUserNotify.cpp \
		gui/gxs/GxsFeedWidget.cpp \
		util/TokenQueue.cpp \
		util/RsGxsUpdateBroadcast.cpp \
	
#		gui/gxs/GxsMsgDialog.cpp \
	
	
}
