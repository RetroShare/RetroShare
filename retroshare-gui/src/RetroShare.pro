CONFIG += qt gui uic qrc resources uitools pluginmgr newsettings
QT     += network xml script opengl
TEMPLATE = app
TARGET = RetroShare

DEFINES *= RS_RELEASE_VERSION
DEFINES += RS_USE_PGPSSL
RCC_DIR = temp/qrc
UI_DIR  = temp/ui
MOC_DIR = temp/moc

################################# Linux ##########################################
# Put lib dir in QMAKE_LFLAGS so it appears before -L/usr/lib
linux-* {
	CONFIG += version_detail_bash_script
	LIBS += ../../libretroshare/src/lib/libretroshare.a
        LIBS += -lssl -lgpgme -lupnp
}

linux-g++ {
	OBJECTS_DIR = temp/linux-g++/obj
}

linux-g++-64 {
	OBJECTS_DIR = temp/linux-g++-64/obj
}

version_detail_bash_script {
	DEFINES += ADD_LIBRETROSHARE_VERSION_INFO
	QMAKE_EXTRA_TARGETS += write_version_detail
	PRE_TARGETDEPS = write_version_detail
	write_version_detail.commands = ./version_detail.sh
}

#################### Cross compilation for windows under Linux ###################

win32-x-g++ {
		OBJECTS_DIR = temp/win32-x-g++/obj

		LIBS += ../../libretroshare/src/lib.win32xgcc/libretroshare.a
		LIBS += ../../../../lib/win32-x-g++-v0.5/libssl.a 
		LIBS += ../../../../lib/win32-x-g++-v0.5/libcrypto.a 
		LIBS += ../../../../lib/win32-x-g++-v0.5/libgpgme.dll.a 
		LIBS += ../../../../lib/win32-x-g++-v0.5/libminiupnpc.a 
		LIBS += ../../../../lib/win32-x-g++-v0.5/libz.a 
		LIBS += -L${HOME}/.wine/drive_c/pthreads/lib -lpthreadGCE2
		LIBS += -lQtUiTools
		LIBS += -lws2_32 -luuid -lole32 -liphlpapi -lcrypt32 -gdi32
		LIBS += -lole32 -lwinmm

		DEFINES *= WIN32 WIN32_CROSS_UBUNTU

		INCLUDEPATH += ../../../../gpgme-1.1.8/src/
		INCLUDEPATH += ../../../../libgpg-error-1.7/src/

		RC_FILE = gui/images/retroshare_win.rc
}

#################################### Windows #####################################

win32 {

    OBJECTS_DIR = temp/obj
    #LIBS += -L"D/Qt/2009.03/qt/plugins/imageformats"
    #QTPLUGIN += qjpeg

    LIBS += ../../libretroshare/src/lib/libretroshare.a
    LIBS += -L"../../../../lib" 
    LIBS += -lssl -lcrypto -lgpgme -lpthreadGC2d -lminiupnpc -lz
    LIBS += -lws2_32 -luuid -lole32 -liphlpapi -lcrypt32-cygwin -lgdi32
    LIBS += -lole32 -lwinmm
    RC_FILE = gui/images/retroshare_win.rc

    GPG_ERROR_DIR = ../../../../libgpg-error-1.7
    GPGME_DIR  = ../../../../gpgme-1.1.8
    INCLUDEPATH += . $${GPGME_DIR}/src $${GPG_ERROR_DIR}/src
}

##################################### MacOS ######################################

macx {
    # ENABLE THIS OPTION FOR Univeral Binary BUILD.
    # CONFIG += ppc x86 

    LIBS += -Wl,-search_paths_first
    LIBS += -L"../../../../lib" -lretroshare -lssl -lcrypto -lminiupnpc -lz
    LIBS += -lgpgme
    LIBS += -lQtUiTools
}

############################## Common stuff ######################################

# On Linux systems that alredy have libssl and libcrypto it is advisable
# to rename the patched version of SSL to something like libsslxpgp.a and libcryptoxpg.a

# ###########################################


DEPENDPATH += . \
            rsiface \
            control \
            gui \
            lang \
            util \
            gui\bwgraph \
            gui\chat \
            gui\connect \
            gui\images \              
            gui\common \
            gui\toaster \
            gui\help\browser \
            gui\elastic
            
INCLUDEPATH += ../../libretroshare/src/

# Input
HEADERS +=  rshare.h \
            gui/notifyqt.h \
            control/bandwidthevent.h \
            control/eventtype.h \
            gui/QuickStartWizard.h \
            gui/DetailsDialog.h \
            gui/DLListDelegate.h \
            gui/ULListDelegate.h \
            gui/StartDialog.h \
            gui/NetworkDialog.h \
            gui/GenCertDialog.h \
            gui/TransfersDialog.h \
            gui/graphframe.h \
            gui/linetypes.h \
            gui/mainpage.h \
            gui/mainpagestack.h \
            gui/MainWindow.h \
            gui/TurtleRouterDialog.h \
            gui/AboutDialog.h \
            gui/AddLinksDialog.h \
            gui/LinksDialog.h \
            gui/ForumsDialog.h \
            gui/forums/ForumDetails.h \
            gui/forums/CreateForum.h \
            gui/forums/CreateForumMsg.h \
            gui/NetworkView.h \
            gui/TrustView.h \
            gui/MessengerWindow.h \
            gui/PeersDialog.h \
            gui/RemoteDirModel.h \
            gui/RetroShareLinkAnalyzer.h \
            gui/SearchTreeWidget.h \
            gui/SearchDialog.h \
            gui/SharedFilesDialog.h \
            gui/ShareManager.h \
            gui/SFListDelegate.h \
            gui/SoundManager.h \
            gui/FileTransferInfoWidget.h \
            gui/RsAutoUpdatePage.h \
            gui/HelpDialog.h \
            gui/InfoDialog.h \
            gui/LogoBar.h \
            gui/xprogressbar.h \
            gui/plugins/PluginInterface.h \
            gui/im_history/IMHistoryKeeper.h           \
            gui/im_history/IMHistoryReader.h           \
            gui/im_history/IMHistoryItem.h             \
            gui/im_history/IMHistoryWriter.h           \
            lang/languagesupport.h \
            util/stringutil.h \
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
            util/Widget.h \
            util/rsversion.h \
            util/RsAction.h \
            util/printpreview.h \
            util/log.h \
            util/misc.h \
            gui/bwgraph/bwgraph.h \
            gui/profile/ProfileView.h \
            gui/profile/ProfileEdit.h \
            gui/profile/ProfileWidget.h \
            gui/profile/StatusMessage.h \
            gui/chat/PopupChatDialog.h \
            gui/channels/CreateChannelMsg.h \
            gui/connect/ConnectDialog.h \
            gui/connect/ConfCertDialog.h \
            gui/msgs/ChanMsgDialog.h \
            gui/msgs/ChanCreateDialog.h \
            gui/images/retroshare_win.rc.h \
            gui/settings/rsharesettings.h \
            gui/settings/rsettings.h \         
            gui/toaster/MessageToaster.h \
            gui/toaster/OnlineToaster.h \
            gui/toaster/ChatToaster.h \
            gui/toaster/CallToaster.h \
            gui/toaster/QtToaster.h \
            gui/toaster/IQtToaster.h \
            gui/toaster/RetroStyleLabelProxy.h \
            gui/common/vmessagebox.h \
            gui/common/rwindow.h \
            gui/common/html.h \
            gui/MessagesDialog.h \
            gui/MessagesPopupDialog.h \
            gui/help/browser/helpbrowser.h \
            gui/help/browser/helptextbrowser.h \
            gui/statusbar/peerstatus.h \
            gui/statusbar/natstatus.h \
            gui/statusbar/ratesstatus.h \  
            gui/advsearch/advancedsearchdialog.h \
            gui/advsearch/expressionwidget.h \
            gui/advsearch/guiexprelement.h \
            gui/elastic/graphwidget.h \
            gui/elastic/edge.h \
            gui/elastic/arrow.h \
            gui/elastic/node.h \
            gui/NewsFeed.h \
            gui/ChannelFeed.h \
            gui/GeneralMsgDialog.h \
            gui/ChanGroupDelegate.h \
            gui/feeds/FeedHolder.h \
            gui/feeds/ForumNewItem.h \
            gui/feeds/ForumMsgItem.h \
            gui/feeds/PeerItem.h \
            gui/feeds/MsgItem.h \
            gui/feeds/ChanGroupItem.h \
            gui/feeds/ChanMenuItem.h \
            gui/feeds/ChanNewItem.h \
            gui/feeds/ChanMsgItem.h \
            gui/feeds/BlogMsgItem.h \
            gui/feeds/SubFileItem.h \
            gui/feeds/SubDestItem.h \
            gui/connect/ConnectFriendWizard.h


FORMS +=    gui/StartDialog.ui \
            gui/GenCertDialog.ui \
            gui/AboutDialog.ui \
            gui/AddLinksDialog.ui \
            gui/QuickStartWizard.ui \
            gui/NetworkDialog.ui \
            gui/TransfersDialog.ui \
            gui/ForumsDialog.ui \
            gui/MainWindow.ui \
            gui/TurtleRouterDialog.ui \
            gui/LinksDialog.ui \
            gui/forums/CreateForum.ui \
            gui/forums/CreateForumMsg.ui \
            gui/forums/ForumDetails.ui \
            gui/NetworkView.ui \
            gui/TrustView.ui \
            gui/MessengerWindow.ui \
            gui/PeersDialog.ui \
            gui/SearchDialog.ui \
            gui/SharedFilesDialog.ui \
            gui/ShareManager.ui \
            gui/MessagesDialog.ui \
            gui/MessagesPopupDialog.ui \
            gui/help/browser/helpbrowser.ui \
            gui/HelpDialog.ui \
            gui/InfoDialog.ui \
            gui/DetailsDialog.ui \
            gui/bwgraph/bwgraph.ui \
            gui/profile/ProfileView.ui \
            gui/profile/ProfileEdit.ui \
            gui/profile/ProfileWidget.ui \
            gui/profile/StatusMessage.ui \
            gui/channels/CreateChannelMsg.ui \
            gui/chat/PopupChatDialog.ui \
            gui/connect/ConnectDialog.ui \
            gui/connect/ConfCertDialog.ui \
            gui/msgs/ChanMsgDialog.ui \
            gui/msgs/ChanCreateDialog.ui \
            gui/toaster/CallToaster.ui \
            gui/toaster/ChatToaster.ui \
            gui/toaster/MessageToaster.ui \
            gui/toaster/OnlineToaster.ui \
            gui/advsearch/AdvancedSearchDialog.ui \
            gui/advsearch/expressionwidget.ui \
            gui/NewsFeed.ui \
            gui/ChannelFeed.ui \
            gui/GeneralMsgDialog.ui \
            gui/feeds/ForumNewItem.ui \
            gui/feeds/ForumMsgItem.ui \
            gui/feeds/PeerItem.ui \
            gui/feeds/MsgItem.ui \
            gui/feeds/ChanGroupItem.ui \
            gui/feeds/ChanMenuItem.ui \
            gui/feeds/ChanNewItem.ui \
            gui/feeds/ChanMsgItem.ui \
            gui/feeds/BlogMsgItem.ui \
            gui/feeds/SubFileItem.ui \
            gui/feeds/SubDestItem.ui \

SOURCES +=  main.cpp \
            rshare.cpp \
            gui/notifyqt.cpp \
            gui/AboutDialog.cpp \
            gui/QuickStartWizard.cpp \
            gui/DetailsDialog.cpp \
            gui/DLListDelegate.cpp \
            gui/ULListDelegate.cpp \
            gui/StartDialog.cpp \
            gui/GenCertDialog.cpp \
            gui/NetworkDialog.cpp \
            gui/TransfersDialog.cpp \
            gui/graphframe.cpp \
            gui/mainpagestack.cpp \
            gui/TurtleRouterDialog.cpp \
            gui/MainWindow.cpp \
            gui/LinksDialog.cpp \
            gui/ForumsDialog.cpp \
            gui/forums/ForumDetails.cpp \
            gui/forums/CreateForum.cpp \
            gui/forums/CreateForumMsg.cpp \
            gui/NetworkView.cpp \
            gui/TrustView.cpp \
            gui/MessengerWindow.cpp \
            gui/PeersDialog.cpp \
            gui/RemoteDirModel.cpp \
            gui/RsAutoUpdatePage.cpp \
            gui/RetroShareLinkAnalyzer.cpp \
            gui/SearchTreeWidget.cpp \
            gui/SearchDialog.cpp \
            gui/AddLinksDialog.cpp \
            gui/SharedFilesDialog.cpp \
            gui/ShareManager.cpp \
            gui/SFListDelegate.cpp \
            gui/SoundManager.cpp \
            gui/MessagesDialog.cpp \
            gui/FileTransferInfoWidget.cpp \
            gui/MessagesPopupDialog.cpp \
            gui/im_history/IMHistoryKeeper.cpp        \
            gui/im_history/IMHistoryReader.cpp        \
            gui/im_history/IMHistoryItem.cpp          \
            gui/im_history/IMHistoryWriter.cpp        \
            gui/help/browser/helpbrowser.cpp \
            gui/help/browser/helptextbrowser.cpp \
            gui/HelpDialog.cpp \
            gui/InfoDialog.cpp \
            gui/LogoBar.cpp \
            gui/xprogressbar.cpp \
            lang/languagesupport.cpp \
            util/stringutil.cpp \
            util/win32.cpp \
            util/RetroStyleLabel.cpp \
            util/WidgetBackgroundImage.cpp \
            util/NonCopyable.cpp \
            util/PixmapMerging.cpp \
            util/MouseEventFilter.cpp \
            util/EventFilter.cpp \
            util/Widget.cpp \
            util/RsAction.cpp \
            util/rsversion.cpp \
            util/printpreview.cpp \
            util/log.cpp \
            gui/bwgraph/bwgraph.cpp \
            gui/profile/ProfileView.cpp \
            gui/profile/ProfileEdit.cpp \
            gui/profile/ProfileWidget.cpp \
            gui/profile/StatusMessage.cpp \
            gui/channels/CreateChannelMsg.cpp \
            gui/chat/PopupChatDialog.cpp \
            gui/connect/ConnectDialog.cpp \
            gui/connect/ConfCertDialog.cpp \
            gui/msgs/ChanMsgDialog.cpp \
            gui/msgs/ChanCreateDialog.cpp \          
            gui/common/vmessagebox.cpp \
            gui/common/rwindow.cpp \ 
            gui/common/html.cpp \
            gui/settings/rsharesettings.cpp \
            gui/settings/rsettings.cpp \ 
            gui/statusbar/peerstatus.cpp \  
            gui/statusbar/natstatus.cpp \
            gui/statusbar/ratesstatus.cpp \  
            gui/toaster/ChatToaster.cpp \
            gui/toaster/MessageToaster.cpp \
            gui/toaster/CallToaster.cpp \
            gui/toaster/OnlineToaster.cpp \
            gui/toaster/QtToaster.cpp \
            gui/advsearch/advancedsearchdialog.cpp \
            gui/advsearch/expressionwidget.cpp \
            gui/advsearch/guiexprelement.cpp \
            gui/elastic/graphwidget.cpp \
            gui/elastic/edge.cpp \
            gui/elastic/arrow.cpp \
            gui/elastic/node.cpp \
            gui/NewsFeed.cpp \
            gui/ChannelFeed.cpp \
            gui/GeneralMsgDialog.cpp \
            gui/ChanGroupDelegate.cpp \
            gui/feeds/ForumNewItem.cpp \
            gui/feeds/ForumMsgItem.cpp \
            gui/feeds/PeerItem.cpp \
            gui/feeds/MsgItem.cpp \
            gui/feeds/ChanGroupItem.cpp \
            gui/feeds/ChanMenuItem.cpp \
            gui/feeds/ChanNewItem.cpp \
            gui/feeds/ChanMsgItem.cpp \
            gui/feeds/BlogMsgItem.cpp \
            gui/feeds/SubFileItem.cpp \
            gui/feeds/SubDestItem.cpp \
            gui/connect/ConnectFriendWizard.cpp
            
RESOURCES += gui/images.qrc lang/lang.qrc gui/help/content/content.qrc 

TRANSLATIONS +=  \
            lang/retroshare_en.ts \
            lang/retroshare_de.ts \
            lang/retroshare_da.ts \
            lang/retroshare_bg.ts \
            lang/retroshare_es.ts \
            lang/retroshare_fi.ts \
            lang/retroshare_fr.ts \
            lang/retroshare_af.ts  \
            lang/retroshare_gr.ts  \
            lang/retroshare_it.ts  \
            lang/retroshare_jp.ts  \
            lang/retroshare_kr.ts  \
            lang/retroshare_pl.ts  \
            lang/retroshare_pt.ts  \
            lang/retroshare_ru.ts  \
            lang/retroshare_tr.ts \
            lang/retroshare_sl.ts \
            lang/retroshare_sr.ts \
            lang/retroshare_sv.ts \
            lang/retroshare_zh_CN.ts \
            lang/retroshare_zh_TW.ts

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

newsettings {
	    SOURCES += 	gui/settings/rsettingswin.cpp \
			gui/settings/GeneralPage.cpp \
			gui/settings/DirectoriesPage.cpp \
			gui/settings/ServerPage.cpp \
			gui/settings/NetworkPage.cpp \ 
			gui/settings/NotifyPage.cpp \
			gui/settings/CryptoPage.cpp \
			gui/settings/AppearancePage.cpp \
			gui/settings/FileAssociationsPage.cpp \
			gui/settings/SoundPage.cpp \
			gui/settings/AddFileAssociationDialog.cpp
	    
	    HEADERS +=  gui/settings/rsettingswin.h \
			gui/settings/GeneralPage.h \
			gui/settings/DirectoriesPage.h \
			gui/settings/ServerPage.h \
			gui/settings/NetworkPage.h \
			gui/settings/NotifyPage.h \
			gui/settings/CryptoPage.h \
			gui/settings/AppearancePage.h \
			gui/settings/FileAssociationsPage.h \
			gui/settings/SoundPage.h \
			gui/settings/AddFileAssociationDialog.h\

	    FORMS +=    gui/settings/settings.ui \
			gui/settings/GeneralPage.ui \
			gui/settings/DirectoriesPage.ui \
			gui/settings/ServerPage.ui \
			gui/settings/NetworkPage.ui \
			gui/settings/NotifyPage.ui \
			gui/settings/CryptoPage.ui \
			gui/settings/AppearancePage.ui \
			gui/settings/SoundPage.ui

	   DEFINES *= NEWSETTINGS

}


unfinished {

DEPENDPATH += gui/unfinished \

HEADERS += gui/unfinished/ApplicationWindow.h \
           gui/unfinished/BlogDialog.h \
           gui/unfinished/CalDialog.h \
           gui/unfinished/ExampleDialog.h \
           gui/unfinished/GamesDialog.h \
           gui/unfinished/MsgFeed.h \
           gui/unfinished/PeersFeed.h \
           gui/unfinished/PhotoDialog.h \
           gui/unfinished/PhotoShow.h \
           gui/unfinished/StatisticDialog.h \
           gui/unfinished/TransferFeed.h

FORMS += gui/unfinished/ApplicationWindow.ui \
         gui/unfinished/BlogDialog.ui \
         gui/unfinished/CalDialog.ui \
         gui/unfinished/ExampleDialog.ui \
         gui/unfinished/GamesDialog.ui \
         gui/unfinished/MsgFeed.ui \
         gui/unfinished/PeersFeed.ui \
         gui/unfinished/PhotoDialog.ui \
         gui/unfinished/PhotoShow.ui \
         gui/unfinished/StatisticDialog.ui \
         gui/unfinished/TransferFeed.ui
         
SOURCES += gui/unfinished/ApplicationWindow.cpp \
           gui/unfinished/BlogDialog.cpp \
           gui/unfinished/CalDialog.cpp \
           gui/unfinished/ExampleDialog.cpp \
           gui/unfinished/GamesDialog.cpp \
           gui/unfinished/MsgFeed.cpp \
           gui/unfinished/PeersFeed.cpp \
           gui/unfinished/PhotoDialog.cpp \
           gui/unfinished/PhotoShow.cpp \
           gui/unfinished/StatisticDialog.cpp \
           gui/unfinished/TransferFeed.cpp

          DEFINES *= UNFINISHED
}


