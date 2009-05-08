CONFIG += qt gui uic qrc resources uitools release # pluginmgr debug
QT     += network xml script 
TEMPLATE = app
TARGET = RetroShare

DEFINES *= RS_RELEASE_VERSION
RCC_DIR = temp/qrc
UI_DIR  = temp/ui
MOC_DIR = temp/moc

################################# Linux ##########################################

linux-g++ {
	OBJECTS_DIR = temp/linux-g++/obj

	LIBS += ../../../../lib/linux-g++/libretroshare.a 
	LIBS += ../../../../lib/linux-g++/libminiupnpc.a 
	LIBS += ../../../../lib/linux-g++/libssl.a 
	LIBS += ../../../../lib/linux-g++/libcrypto.a
	LIBS += -lQtUiTools
    LIBS += -lz 
}
linux-g++-64 {
	OBJECTS_DIR = temp/linux-g++-64/obj

	LIBS += ../../../../lib/linux-g++-64/libretroshare.a 
	LIBS += ../../../../lib/linux-g++-64/libminiupnpc.a 
	LIBS += ../../../../lib/linux-g++-64/libssl.a 
	LIBS += ../../../../lib/linux-g++-64/libcrypto.a
    LIBS += -lz
}

#################### Cross compilation for windows under Linux ###################

win32-x-g++ {
	OBJECTS_DIR = temp/win32-x-g++/obj

	LIBS += ../../../../lib/win32-x-g++/libretroshare.a 
	LIBS += ../../../../lib/win32-x-g++/libssl.a 
	LIBS += ../../../../lib/win32-x-g++/libcrypto.a 
	LIBS += ../../../../lib/win32-x-g++/libminiupnpc.a 
	LIBS += ../../../../lib/win32-x-g++/libz.a 
	LIBS += -L${HOME}/.wine/drive_c/pthreads/lib -lpthreadGCE2
	LIBS += -lQtUiTools
	LIBS += -lws2_32 -luuid -lole32 -liphlpapi -lcrypt32 -gdi32
	LIBS += -lole32 -lwinmm

	DEFINES *= WIN32

	RC_FILE = gui/images/retroshare_win.rc
}

#################################### Windows #####################################

win32 {

	OBJECTS_DIR = temp/obj

    LIBS += -L"../../../../lib" -lretroshare -lssl -lcrypto -lpthreadGC2d -lminiupnpc -lz
    LIBS += -lws2_32 -luuid -lole32 -liphlpapi -lcrypt32-cygwin -lgdi32
    LIBS += -lole32 -lwinmm

    RC_FILE = gui/images/retroshare_win.rc
    
    ## ENABLE THIS OPTION for release Version.
    #DEFINES += RS_RELEASE_VERSION


}

##################################### MacOS ######################################

macx {
    # ENABLE THIS OPTION FOR Univeral Binary BUILD.
    # CONFIG += ppc x86 

    LIBS += -Wl,-search_paths_first
    LIBS += -L"../../../../lib" -lretroshare -lssl -lcrypto -lminiupnpc -lz
}

############################## Common stuff ######################################

DEPENDPATH += . \
            rsiface \
            control \
            gui \
            lang \
            util \
            games\qbackgammon \
            gui\bwgraph \
            gui\chat \
            gui\connect \
            gui\images \              
            gui\Preferences \
            gui\common \
            gui\library \
            gui\Settings \
            gui\toaster \
            gui\help\browser \
            gui\elastic
            
INCLUDEPATH += . \

# Input
HEADERS +=  rshare.h \
            rsiface/rsiface.h \
            rsiface/rstypes.h \
            rsiface/notifyqt.h \
            rsiface/RemoteDirModel.h \
            control/bandwidthevent.h \
            control/eventtype.h \
            gui/DLListDelegate.h \
            gui/ULListDelegate.h \
            gui/StartDialog.h \
#           gui/ChatDialog.h \
            gui/BlogDialog.h \
            gui/CalDialog.h \
            gui/NetworkDialog.h \
            gui/GenCertDialog.h \
            gui/TransfersDialog.h \
            gui/graphframe.h \
            gui/linetypes.h \
            gui/mainpage.h \
            gui/mainpagestack.h \
            gui/MainWindow.h \
            gui/ApplicationWindow.h \
            gui/ExampleDialog.h \
            gui/GamesDialog.h \
            gui/PhotoDialog.h \
            gui/PhotoShow.h \
            gui/LinksDialog.h \
            gui/LibraryDialog.h \
            gui/ForumsDialog.h \
            gui/forums/CreateForum.h \
            gui/forums/CreateForumMsg.h \
            gui/NetworkView.h \
            gui/TrustView.h \
            gui/MessengerWindow.h \
            gui/PeersDialog.h \
            gui/SearchTreeWidget.h \
            gui/SearchDialog.h \
            gui/SharedFilesDialog.h \
            gui/ShareManager.h \
            gui/StatisticDialog.h \
            gui/HelpDialog.h \
            gui/LogoBar.h \
            gui/xprogressbar.h \
            # gui/PluginsPage.h  \
            # gui/PluginManagerWidget.h \
            # gui/PluginManager.h      \
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
            gui/chat/PopupChatDialog.h \
            gui/connect/ConnectDialog.h \
            gui/connect/ConfCertDialog.h \
#            gui/connect/InviteDialog.h \
#            gui/connect/AddFriendDialog.h \
#            gui/connect/AddFriendWizard.h \
            gui/library/FindWindow.h \ 
            gui/msgs/ChanMsgDialog.h \
            gui/msgs/ChanCreateDialog.h \
            gui/images/retroshare_win.rc.h \
            gui/Preferences/configpage.h \
            gui/Preferences/configpagestack.h \
            gui/Preferences/CryptographyDialog.h \
            gui/Preferences/DirectoriesDialog.h \
            gui/Preferences/AppearanceDialog.h \
            gui/Preferences/GeneralDialog.h \
            gui/Preferences/PreferencesWindow.h \
            gui/Preferences/ServerDialog.h \          
            gui/Preferences/NotifyDialog.h \          
            gui/Preferences/ConfirmQuitDialog.h \
            gui/Preferences/rsharesettings.h \
            gui/Preferences/rsettings.h \
            gui/Preferences/FileAssotiationsDialog.h \
            gui/Preferences/AddFileAssotiationDialog.h \
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
            gui/Settings/gsettingswin.h \
            gui/Settings/GeneralPage.h \
            gui/Settings/DirectoriesPage.h \
            gui/Settings/ServerPage.h \
            gui/Settings/NetworkPage.h \
            gui/statusbar/peerstatus.h \
            gui/statusbar/dhtstatus.h \
            gui/statusbar/natstatus.h \
            gui/statusbar/ratesstatus.h \  
            gui/advsearch/advancedsearchdialog.h \
            gui/advsearch/expressionwidget.h \
            gui/advsearch/guiexprelement.h \
            games/qbackgammon/bgwindow.h \
            games/qbackgammon/bgwidget.h \
            games/qbackgammon/bgboard.h \
            games/qbackgammon/optionsdlg.h \
            gui/channels/channelsDialog.h \
            gui/channels/channelBrowserDialog.h \
            gui/channels/channelStatsDialog.h \
            gui/channels/myChannelsDialog.h \
            gui/channels/mySubscriptionsDialog.h \
            gui/elastic/graphwidget.h \
            gui/elastic/edge.h \
            gui/elastic/node.h \
            gui/NewsFeed.h \
            gui/PeersFeed.h \
            gui/MsgFeed.h \
            gui/TransferFeed.h \
            gui/ChannelFeed.h \
            gui/GeneralMsgDialog.h \
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


FORMS +=   \ #ui/ChatDialog.ui \
            gui/BlogDialog.ui \
            gui/CalDialog.ui \
            gui/StartDialog.ui \
            gui/GenCertDialog.ui \
            gui/NetworkDialog.ui \
            gui/TransfersDialog.ui \
            gui/MainWindow.ui \
            gui/ApplicationWindow.ui \
            gui/ExampleDialog.ui \
            gui/GamesDialog.ui \
            gui/PhotoDialog.ui \
            gui/PhotoShow.ui \
            gui/LinksDialog.ui \
            gui/LibraryDialog.ui \
            gui/ForumsDialog.ui \
            gui/forums/CreateForum.ui \
            gui/forums/CreateForumMsg.ui \
            gui/NetworkView.ui \
            gui/TrustView.ui \
            gui/MessengerWindow.ui \
            gui/PeersDialog.ui \
            gui/SearchDialog.ui \
            gui/SharedFilesDialog.ui \
            gui/ShareManager.ui \
            gui/StatisticDialog.ui \
            gui/MessagesDialog.ui \
            gui/MessagesPopupDialog.ui \
            gui/help/browser/helpbrowser.ui \
            gui/HelpDialog.ui \
            gui/bwgraph/bwgraph.ui \
            gui/profile/ProfileView.ui \
            gui/profile/ProfileEdit.ui \
            gui/chat/PopupChatDialog.ui \
            gui/connect/ConnectDialog.ui \
            gui/connect/ConfCertDialog.ui \
#            gui/connect/InviteDialog.ui \
#            gui/connect/AddFriendDialog.ui \
#            gui/connect/AddFriendWizard.ui \
            gui/msgs/ChanMsgDialog.ui \
            gui/msgs/ChanCreateDialog.ui \
            gui/Preferences/CryptographyDialog.ui \
            gui/Preferences/DirectoriesDialog.ui \
            gui/Preferences/AppearanceDialog.ui \
            gui/Preferences/GeneralDialog.ui \
            gui/Preferences/PreferencesWindow.ui \
            gui/Preferences/ServerDialog.ui \
            gui/Preferences/NotifyDialog.ui \          
            gui/Preferences/ConfirmQuitDialog.ui \
            gui/toaster/CallToaster.ui \
            gui/toaster/ChatToaster.ui \
            gui/toaster/MessageToaster.ui \
            gui/toaster/OnlineToaster.ui \
            gui/Settings/settings.ui \
            gui/Settings/GeneralPage.ui \
            gui/Settings/DirectoriesPage.ui \
            gui/Settings/ServerPage.ui \
            gui/Settings/NetworkPage.ui \
            gui/advsearch/AdvancedSearchDialog.ui \
            gui/advsearch/expressionwidget.ui \
            gui/channels/ChannelsDialog.ui \
            gui/channels/ChannelBrowserDialog.ui \
            gui/channels/ChannelStatsDialog.ui \
            gui/channels/MyChannelsDialog.ui \
            gui/channels/MySubscriptionsDialog.ui \
            gui/NewsFeed.ui \
            gui/PeersFeed.ui \
            gui/MsgFeed.ui \
            gui/TransferFeed.ui \
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

SOURCES += main.cpp \
            rshare.cpp \
            rsiface/notifyqt.cpp \
            rsiface/RemoteDirModel.cpp \
            gui/DLListDelegate.cpp \
            gui/ULListDelegate.cpp \
            gui/StartDialog.cpp \
            gui/GenCertDialog.cpp \
#           gui/ChatDialog.cpp \
            gui/BlogDialog.cpp \
            gui/CalDialog.cpp \
            gui/NetworkDialog.cpp \
            gui/TransfersDialog.cpp \
            gui/graphframe.cpp \
            gui/mainpagestack.cpp \
            gui/MainWindow.cpp \
            gui/ApplicationWindow.cpp \
            gui/ExampleDialog.cpp \
            gui/GamesDialog.cpp \
            gui/PhotoDialog.cpp \
            gui/PhotoShow.cpp \
            gui/LinksDialog.cpp \
            gui/LibraryDialog.cpp \
            gui/ForumsDialog.cpp \
            gui/forums/CreateForum.cpp \
            gui/forums/CreateForumMsg.cpp \
            gui/NetworkView.cpp \
            gui/TrustView.cpp \
            gui/MessengerWindow.cpp \
            gui/PeersDialog.cpp \
            gui/SearchTreeWidget.cpp \
            gui/SearchDialog.cpp \
            gui/SharedFilesDialog.cpp \
            gui/ShareManager.cpp \
            gui/StatisticDialog.cpp \
            gui/MessagesDialog.cpp \
            gui/MessagesPopupDialog.cpp \
            gui/im_history/IMHistoryKeeper.cpp        \
            gui/im_history/IMHistoryReader.cpp        \
            gui/im_history/IMHistoryItem.cpp          \
            gui/im_history/IMHistoryWriter.cpp        \
            gui/help/browser/helpbrowser.cpp \
            gui/help/browser/helptextbrowser.cpp \
            gui/HelpDialog.cpp \
            gui/LogoBar.cpp \
            gui/xprogressbar.cpp \
            # gui/PluginsPage.cpp \
            # gui/PluginManagerWidget.cpp \
            # gui/PluginManager.cpp      \
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
            util/rsversion.cpp \
            util/RsAction.cpp \
            util/printpreview.cpp \
            util/log.cpp \
            gui/bwgraph/bwgraph.cpp \
            gui/profile/ProfileView.cpp \
            gui/profile/ProfileEdit.cpp \
            gui/chat/PopupChatDialog.cpp \
            gui/connect/ConnectDialog.cpp \
            gui/connect/ConfCertDialog.cpp \
#            gui/connect/InviteDialog.cpp \
#            gui/connect/AddFriendDialog.cpp \
#            gui/connect/AddFriendWizard.cpp \
            gui/msgs/ChanMsgDialog.cpp \
            gui/msgs/ChanCreateDialog.cpp \
            gui/Preferences/configpagestack.cpp \
            gui/Preferences/CryptographyDialog.cpp \
            gui/Preferences/DirectoriesDialog.cpp \
            gui/Preferences/AppearanceDialog.cpp \
            gui/Preferences/GeneralDialog.cpp \
            gui/Preferences/PreferencesWindow.cpp \
            gui/Preferences/ServerDialog.cpp \
            gui/Preferences/NotifyDialog.cpp \          
            gui/Preferences/ConfirmQuitDialog.cpp \
            gui/Preferences/rsharesettings.cpp \
            gui/Preferences/rsettings.cpp \
            gui/Preferences/FileAssotiationsDialog.cpp   \
            gui/Preferences/AddFileAssotiationDialog.cpp \
            gui/common/vmessagebox.cpp \
            gui/common/rwindow.cpp \ 
            gui/common/html.cpp \ 
            gui/library/FindWindow.cpp \        
            gui/Settings/gsettingswin.cpp \
            gui/Settings/GeneralPage.cpp \
            gui/Settings/DirectoriesPage.cpp \
            gui/Settings/ServerPage.cpp \
            gui/Settings/NetworkPage.cpp \
            gui/statusbar/peerstatus.cpp \  
            gui/statusbar/dhtstatus.cpp \
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
            games/qbackgammon/bgwindow.cpp \
            games/qbackgammon/bgwidget.cpp \
            games/qbackgammon/bgboard.cpp \
            games/qbackgammon/optionsdlg.cpp \
            gui/channels/channelsDialog.cpp \
            gui/channels/channelBrowserDialog.cpp \
            gui/channels/channelStatsDialog.cpp \
            gui/channels/myChannelsDialog.cpp \
            gui/channels/mySubscriptionsDialog.cpp \
            gui/elastic/graphwidget.cpp \
            gui/elastic/edge.cpp \
            gui/elastic/node.cpp \
            gui/NewsFeed.cpp \
            gui/PeersFeed.cpp \
            gui/MsgFeed.cpp \
            gui/TransferFeed.cpp \
            gui/ChannelFeed.cpp \
            gui/GeneralMsgDialog.cpp \
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
            lang/retroshare_bg.ts \
            lang/retroshare_es.ts \
            lang/retroshare_fi.ts \
            lang/retroshare_fr.ts \
            lang/retroshare_af.ts  \
            lang/retroshare_cn_simp.ts  \
            lang/retroshare_cn_trad.ts  \
            lang/retroshare_gr.ts  \
            lang/retroshare_it.ts  \
            lang/retroshare_jp.ts  \
            lang/retroshare_kr.ts  \
            lang/retroshare_pl.ts  \
            lang/retroshare_pt.ts  \
            lang/retroshare_ru.ts  \
            lang/retroshare_tr.ts \
            lang/retroshare_dk.ts \
            lang/retroshare_sl.ts \
            lang/retroshare_sr.ts \
            lang/retroshare_se.ts \
            lang/retroshare_zh_CN.ts \
            lang/retroshare_zh_TW.ts

# To compile for turtle hopping. I'm using this flag to avoid conflict while developping.
# Just do a 
#    qmake CONFIG=turtle

turtle {                        
        SOURCES += gui/TurtleSearchDialog.cpp
        HEADERS += rsiface/rsturtle.h gui/TurtleSearchDialog.h
        FORMS += gui/TurtleSearchDialog.ui
        DEFINES *= TURTLE_HOPPING
        DEFINES *= RS_RELEASE_VERSION
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







