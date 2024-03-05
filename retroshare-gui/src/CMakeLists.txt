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

list(
	APPEND RS_GUI_SOURCES
	src/TorControl/TorControlWindow.cpp

	src/main.cpp 
	src/rshare.cpp 

	src/gui/notifyqt.cpp 
	src/gui/AboutDialog.cpp 
	src/gui/AboutWidget.cpp 
	src/gui/QuickStartWizard.cpp 
	src/gui/StartDialog.cpp 
	src/gui/HomePage.cpp
	src/gui/ChatLobbyWidget.cpp 
	src/gui/GetStartedDialog.cpp 
	src/gui/GenCertDialog.cpp 
	src/gui/NetworkDialog.cpp 
	src/gui/mainpagestack.cpp 
	src/gui/MainWindow.cpp 
	src/gui/NetworkView.cpp 
	src/gui/FriendsDialog.cpp 
	src/gui/ServicePermissionDialog.cpp 
	src/gui/RemoteDirModel.cpp 
	src/gui/RsAutoUpdatePage.cpp 
	src/gui/RetroShareLink.cpp 
	src/gui/SearchTreeWidget.cpp 
	src/gui/ShareManager.cpp 
	#	src/gui/ShareDialog.cpp 
	src/gui/NewsFeed.cpp 
	src/gui/MainPage.cpp 
	src/gui/HelpDialog.cpp 
	src/gui/LogoBar.cpp 
	src/gui/SoundManager.cpp 

	src/gui/im_history/ImHistoryBrowser.cpp 
	src/gui/im_history/IMHistoryItemDelegate.cpp 
	src/gui/im_history/IMHistoryItemPainter.cpp 

	src/gui/help/browser/helpbrowser.cpp 
	src/gui/help/browser/helptextbrowser.cpp 

	src/gui/FileTransfer/SearchDialog.cpp 
	src/gui/FileTransfer/SharedFilesDialog.cpp 
	src/gui/FileTransfer/TransfersDialog.cpp 
	src/gui/FileTransfer/FileTransferInfoWidget.cpp 
	src/gui/FileTransfer/DLListDelegate.cpp 
	src/gui/FileTransfer/ULListDelegate.cpp 
	src/gui/FileTransfer/xprogressbar.cpp 
	src/gui/FileTransfer/DetailsDialog.cpp 
	src/gui/FileTransfer/TransferUserNotify.cpp 
	src/gui/FileTransfer/BannedFilesDialog.cpp 

	src/lang/languagesupport.cpp 

	src/util/RsProtectedTimer.cpp 
	src/util/stringutil.cpp 
	src/util/RsNetUtil.cpp 
	src/util/DateTime.cpp 
	src/util/RetroStyleLabel.cpp 
	src/util/WidgetBackgroundImage.cpp 
	src/util/PixmapMerging.cpp 
	src/util/MouseEventFilter.cpp 
	src/util/EventFilter.cpp 
	src/util/Widget.cpp 
	src/util/RsAction.cpp 
	src/util/printpreview.cpp 
	src/util/log.cpp 
	src/util/misc.cpp 
	src/util/HandleRichText.cpp 
	src/util/ObjectPainter.cpp 
	src/util/RsFile.cpp 
	src/util/RichTextEdit.cpp 
	src/util/ClickableLabel.cpp 
	src/util/AspectRatioPixmapLabel.cpp 

	src/gui/profile/ProfileWidget.cpp 
	src/gui/profile/StatusMessage.cpp 
	src/gui/profile/ProfileManager.cpp 

	src/gui/chat/PopupChatWindow.cpp 
	src/gui/chat/PopupChatDialog.cpp 
	src/gui/chat/PopupDistantChatDialog.cpp 
	src/gui/chat/ChatTabWidget.cpp 
	src/gui/chat/ChatWidget.cpp 
	src/gui/chat/ChatDialog.cpp 
	src/gui/chat/ChatLobbyDialog.cpp 
	src/gui/chat/CreateLobbyDialog.cpp 
	src/gui/chat/ChatStyle.cpp 
	src/gui/chat/ChatUserNotify.cpp 
	src/gui/chat/ChatLobbyUserNotify.cpp 

	src/gui/connect/ConfCertDialog.cpp 
	src/gui/connect/PGPKeyDialog.cpp 
	src/gui/connect/ConnectFriendWizard.cpp 
	src/gui/connect/ConnectProgressDialog.cpp 
	src/gui/connect/FriendRecommendDialog.cpp 

	src/gui/msgs/MessagesDialog.cpp 
	src/gui/msgs/MessageComposer.cpp 
	src/gui/msgs/MessageWidget.cpp 
	src/gui/msgs/MessageWindow.cpp 
	src/gui/msgs/MessageModel.cpp 
	src/gui/msgs/TagsMenu.cpp 
	src/gui/msgs/MessageUserNotify.cpp 

	src/gui/common/RsButtonOnText.cpp 
	src/gui/common/RSGraphWidget.cpp 
	src/gui/common/ElidedLabel.cpp 
	src/gui/common/vmessagebox.cpp 
	src/gui/common/RsCollectionDialog.cpp 
	src/gui/common/RsCollection.cpp 
	src/gui/common/RsUrlHandler.cpp 
	src/gui/common/rwindow.cpp 
	src/gui/common/rshtml.cpp 
	src/gui/common/AvatarDefs.cpp 
	src/gui/common/AvatarDialog.cpp 
	src/gui/common/GroupFlagsWidget.cpp 
	src/gui/common/GroupSelectionBox.cpp 
	src/gui/common/GroupChooser.cpp 
	src/gui/common/StatusDefs.cpp 
	src/gui/common/TagDefs.cpp 
	src/gui/common/GroupDefs.cpp 
	src/gui/common/Emoticons.cpp 
	src/gui/common/RSComboBox.cpp 
	src/gui/common/RSListWidgetItem.cpp 
	src/gui/common/RSTextEdit.cpp 
	src/gui/common/RSPlainTextEdit.cpp 
	src/gui/common/RSTreeWidget.cpp 
	src/gui/common/RSTreeWidgetItem.cpp 
	src/gui/common/RSFeedWidget.cpp 
	src/gui/common/RSTabWidget.cpp 
	src/gui/common/RSElidedItemDelegate.cpp 
	src/gui/common/RSItemDelegate.cpp 
	src/gui/common/PeerDefs.cpp 
	src/gui/common/FilesDefs.cpp 
	src/gui/common/PopularityDefs.cpp 
	src/gui/common/RsBanListDefs.cpp 
	src/gui/common/GroupTreeWidget.cpp 
	src/gui/common/RSTreeView.cpp 
	src/gui/common/AvatarWidget.cpp 
	src/gui/common/FriendListModel.cpp 
	src/gui/common/NewFriendList.cpp 
	src/gui/common/FriendSelectionWidget.cpp 
	src/gui/common/FriendSelectionDialog.cpp 
	src/gui/common/HashBox.cpp 
	src/gui/common/LineEditClear.cpp 
	src/gui/common/DropLineEdit.cpp 
	src/gui/common/RSTextBrowser.cpp 
	src/gui/common/RSImageBlockWidget.cpp 
	src/gui/common/FeedNotify.cpp 
	src/gui/common/UserNotify.cpp 
	src/gui/common/HeaderFrame.cpp 
	src/gui/common/MimeTextEdit.cpp 
	src/gui/common/UIStateHelper.cpp 
	src/gui/common/FloatingHelpBrowser.cpp 
	src/gui/common/SubscribeToolButton.cpp 
	src/gui/common/RsBanListToolButton.cpp 
	src/gui/common/FlowLayout.cpp 
	src/gui/common/PictureFlow.cpp 
	src/gui/common/ToasterNotify.cpp 
	src/gui/common/NotifyWidget.cpp 

	src/gui/style/RSStyle.cpp 
	src/gui/style/StyleDialog.cpp 

	src/gui/settings/RSPermissionMatrixWidget.cpp 
	src/gui/settings/rsharesettings.cpp 
	src/gui/settings/RsharePeerSettings.cpp 
	src/gui/settings/rsettings.cpp 
	src/gui/settings/rsettingswin.cpp 
	src/gui/settings/GeneralPage.cpp 
	src/gui/settings/AboutPage.cpp 
	src/gui/settings/ServerPage.cpp 
	src/gui/settings/NotifyPage.cpp 
	src/gui/settings/CryptoPage.cpp 
	src/gui/settings/PeoplePage.cpp 
	src/gui/settings/MessagePage.cpp 
	src/gui/settings/NewTag.cpp 
	src/gui/settings/ForumPage.cpp 
	src/gui/settings/PluginsPage.cpp 
	src/gui/settings/PluginItem.cpp 
	src/gui/settings/AppearancePage.cpp 
	src/gui/settings/FileAssociationsPage.cpp 
	src/gui/settings/SoundPage.cpp 
	src/gui/settings/TransferPage.cpp 
	src/gui/settings/ChatPage.cpp 
	src/gui/settings/ChannelPage.cpp 
	src/gui/settings/PostedPage.cpp 
	src/gui/settings/ServicePermissionsPage.cpp 
	src/gui/settings/AddFileAssociationDialog.cpp 
	src/gui/settings/GroupFrameSettingsWidget.cpp 

	src/gui/statusbar/peerstatus.cpp 
	src/gui/statusbar/natstatus.cpp 
	src/gui/statusbar/dhtstatus.cpp 
	src/gui/statusbar/torstatus.cpp 
	src/gui/statusbar/ratesstatus.cpp 
	src/gui/statusbar/hashingstatus.cpp 
	src/gui/statusbar/discstatus.cpp 
	src/gui/statusbar/SoundStatus.cpp 
	src/gui/statusbar/OpModeStatus.cpp 
	src/gui/statusbar/ToasterDisable.cpp 
	src/gui/statusbar/SysTrayStatus.cpp 

	src/gui/toaster/ToasterItem.cpp 
	src/gui/toaster/MessageToaster.cpp 
	src/gui/toaster/DownloadToaster.cpp 
	src/gui/toaster/OnlineToaster.cpp 
	src/gui/toaster/ChatToaster.cpp 
	src/gui/toaster/GroupChatToaster.cpp 
	src/gui/toaster/ChatLobbyToaster.cpp 
	src/gui/toaster/FriendRequestToaster.cpp 

	src/gui/advsearch/advancedsearchdialog.cpp 
	src/gui/advsearch/expressionwidget.cpp 
	src/gui/advsearch/guiexprelement.cpp 

	src/gui/elastic/graphwidget.cpp 
	src/gui/elastic/edge.cpp 
	src/gui/elastic/arrow.cpp 
	src/gui/elastic/elnode.cpp 

	src/gui/feeds/BoardsCommentsItem.cpp 
	src/gui/feeds/FeedItem.cpp 
	src/gui/feeds/FeedHolder.cpp 
	src/gui/feeds/GxsCircleItem.cpp 
	src/gui/feeds/ChannelsCommentsItem.cpp 
	src/gui/feeds/PeerItem.cpp 
	src/gui/feeds/MsgItem.cpp 
	src/gui/feeds/ChatMsgItem.cpp 
	src/gui/feeds/SubFileItem.cpp 
	src/gui/feeds/AttachFileItem.cpp 
	src/gui/feeds/SecurityItem.cpp 
	src/gui/feeds/SecurityIpItem.cpp 
	src/gui/feeds/NewsFeedUserNotify.cpp 

	src/gui/groups/CreateGroup.cpp 

	src/gui/statistics/BandwidthGraphWindow.cpp 
	src/gui/statistics/BandwidthStatsWidget.cpp 
	src/gui/statistics/DhtWindow.cpp 
	src/gui/statistics/Histogram.cpp 
	src/gui/statistics/TurtleRouterDialog.cpp 
	src/gui/statistics/TurtleRouterStatistics.cpp 
	src/gui/statistics/GxsIdStatistics.cpp 
	src/gui/statistics/GlobalRouterStatistics.cpp 
	src/gui/statistics/GxsTransportStatistics.cpp 
	src/gui/statistics/StatisticsWindow.cpp 
	src/gui/statistics/BwCtrlWindow.cpp 
	src/gui/statistics/RttStatistics.cpp 
	src/gui/statistics/BWGraph.cpp 

	src/util/RsSyntaxHighlighter.cpp 
	src/util/imageutil.cpp 
	src/util/retroshareWin32.cpp

	src/gui/NetworkDialog/pgpid_item_model.cpp 
	src/gui/NetworkDialog/pgpid_item_proxy.cpp 
	)

list(
	APPEND RS_GUI_FORMS
	src/TorControl/TorControlWindow.ui

	src/gui/StartDialog.ui 
	src/gui/HomePage.ui
	src/gui/GetStartedDialog.ui 
	src/gui/GenCertDialog.ui 
	src/gui/AboutDialog.ui 
	src/gui/AboutWidget.ui 
	src/gui/QuickStartWizard.ui 
	src/gui/NetworkDialog.ui 
	src/gui/common/AvatarDialog.ui 
	src/gui/MainWindow.ui 
	src/gui/NetworkView.ui 
	src/gui/FriendsDialog.ui 
	src/gui/NewsFeed.ui 
	src/gui/ShareManager.ui 
	src/gui/help/browser/helpbrowser.ui 
	src/gui/HelpDialog.ui 
	src/gui/ServicePermissionDialog.ui 
	src/gui/ChatLobbyWidget.ui 

	src/gui/FileTransfer/TransfersDialog.ui 
	src/gui/FileTransfer/DetailsDialog.ui 
	src/gui/FileTransfer/SearchDialog.ui 
	src/gui/FileTransfer/SharedFilesDialog.ui 
	src/gui/FileTransfer/BannedFilesDialog.ui 

	src/gui/profile/ProfileWidget.ui 
	src/gui/profile/StatusMessage.ui 
	src/gui/profile/ProfileManager.ui 

	src/gui/chat/PopupChatWindow.ui 
	src/gui/chat/PopupChatDialog.ui 
	src/gui/chat/ChatTabWidget.ui 
	src/gui/chat/ChatWidget.ui 
	src/gui/chat/ChatLobbyDialog.ui 
	src/gui/chat/CreateLobbyDialog.ui 

	src/gui/connect/ConfCertDialog.ui 
	src/gui/connect/PGPKeyDialog.ui 
	src/gui/connect/ConnectFriendWizard.ui 
	src/gui/connect/ConnectProgressDialog.ui 
	src/gui/connect/FriendRecommendDialog.ui 

	src/gui/msgs/MessagesDialog.ui 
	src/gui/msgs/MessageComposer.ui 
	src/gui/msgs/MessageWindow.ui
	src/gui/msgs/MessageWidget.ui

	src/gui/settings/settingsw.ui 
	src/gui/settings/GeneralPage.ui 
	src/gui/settings/ServerPage.ui 
	src/gui/settings/NotifyPage.ui 
	src/gui/settings/PeoplePage.ui 
	src/gui/settings/CryptoPage.ui 
	src/gui/settings/MessagePage.ui 
	src/gui/settings/NewTag.ui 
	src/gui/settings/ForumPage.ui 
	src/gui/settings/AboutPage.ui 
	src/gui/settings/PluginsPage.ui 
	src/gui/settings/AppearancePage.ui 
	src/gui/settings/TransferPage.ui 
	src/gui/settings/SoundPage.ui 
	src/gui/settings/ChatPage.ui 
	src/gui/settings/ChannelPage.ui 
	src/gui/settings/PostedPage.ui 
	src/gui/settings/ServicePermissionsPage.ui 
	src/gui/settings/PluginItem.ui 
	src/gui/settings/GroupFrameSettingsWidget.ui 

	src/gui/toaster/MessageToaster.ui 
	src/gui/toaster/OnlineToaster.ui 
	src/gui/toaster/DownloadToaster.ui 
	src/gui/toaster/ChatToaster.ui 
	src/gui/toaster/GroupChatToaster.ui 
	src/gui/toaster/ChatLobbyToaster.ui 
	src/gui/toaster/FriendRequestToaster.ui 

	src/gui/advsearch/AdvancedSearchDialog.ui 
	src/gui/advsearch/expressionwidget.ui 

	src/gui/feeds/BoardsCommentsItem.ui 
	src/gui/feeds/GxsCircleItem.ui 
	src/gui/feeds/ChannelsCommentsItem.ui 
	src/gui/feeds/PeerItem.ui 
	src/gui/feeds/MsgItem.ui 
	src/gui/feeds/ChatMsgItem.ui 
	src/gui/feeds/SubFileItem.ui 
	src/gui/feeds/AttachFileItem.ui 
	src/gui/feeds/SecurityItem.ui 
	src/gui/feeds/SecurityIpItem.ui 

	src/gui/im_history/ImHistoryBrowser.ui 

	src/gui/groups/CreateGroup.ui 

	src/gui/common/GroupTreeWidget.ui 
	src/gui/common/AvatarWidget.ui 
	src/gui/common/NewFriendList.ui 
	src/gui/common/FriendSelectionWidget.ui 
	src/gui/common/HashBox.ui 
	src/gui/common/RSImageBlockWidget.ui 
	src/gui/common/RsCollectionDialog.ui 
	src/gui/common/HeaderFrame.ui 
	src/gui/common/RSFeedWidget.ui 
	src/gui/common/NotifyWidget.ui 

	src/gui/style/StyleDialog.ui 

	src/gui/statistics/BandwidthGraphWindow.ui 
	src/gui/statistics/BandwidthStatsWidget.ui 
	src/gui/statistics/DhtWindow.ui 
	src/gui/statistics/TurtleRouterDialog.ui 
	src/gui/statistics/TurtleRouterStatistics.ui 
	src/gui/statistics/GxsIdStatistics.ui 
	src/gui/statistics/GlobalRouterStatistics.ui 
	src/gui/statistics/GxsTransportStatistics.ui 
	src/gui/statistics/StatisticsWindow.ui 
	src/gui/statistics/BwCtrlWindow.ui 
	src/gui/statistics/RttStatistics.ui 

	src/util/RichTextEdit.ui
	)

list(
	APPEND RS_IMPLEMENTATION_HEADERS

	src/TorControl/TorControlWindow.h

	src/chat/distantchat.h
	src/chat/distributedchat.h
	src/chat/p3chatservice.h
	src/chat/rschatitems.h 

	src/rshare.h 
	src/retroshare-gui/configpage.h 
	src/retroshare-gui/RsAutoUpdatePage.h 
	src/retroshare-gui/mainpage.h 

	src/control/bandwidthevent.h 
	src/control/eventtype.h 

	src/gui/QuickStartWizard.h 
	src/gui/notifyqt.h 
	src/gui/GetStartedDialog.h 
	src/gui/StartDialog.h 
	src/gui/HomePage.h
	src/gui/NetworkDialog.h 
	src/gui/GenCertDialog.h 
	src/gui/linetypes.h 
	src/gui/mainpagestack.h 
	src/gui/MainWindow.h 
	src/gui/RSHumanReadableDelegate.h 
	src/gui/AboutDialog.h 
	src/gui/AboutWidget.h 
	src/gui/NetworkView.h 
	src/gui/FriendsDialog.h 
	src/gui/ServicePermissionDialog.h 
	src/gui/RemoteDirModel.h 
	src/gui/RetroShareLink.h 
	src/gui/SearchTreeWidget.h 
	src/gui/ShareManager.h 
	src/gui/NewsFeed.h 
	src/gui/ShareDialog.h 
	src/gui/SFListDelegate.h 
	src/gui/SoundManager.h 
	src/gui/HelpDialog.h 
	src/gui/LogoBar.h 

	src/gui/common/AvatarDialog.h 

	src/gui/NetworkDialog/pgpid_item_model.h 
	src/gui/NetworkDialog/pgpid_item_proxy.h 

	src/gui/FileTransfer/SearchDialog.h 
	src/gui/FileTransfer/SharedFilesDialog.h 
	src/gui/FileTransfer/xprogressbar.h 
	src/gui/FileTransfer/DetailsDialog.h 
	src/gui/FileTransfer/FileTransferInfoWidget.h 
	src/gui/FileTransfer/DLListDelegate.h 
	src/gui/FileTransfer/ULListDelegate.h 
	src/gui/FileTransfer/TransfersDialog.h 
	src/gui/FileTransfer/BannedFilesDialog.h 
	src/gui/FileTransfer/TransferUserNotify.h 

	src/gui/statistics/TurtleRouterDialog.h 
	src/gui/statistics/TurtleRouterStatistics.h 
	src/gui/statistics/GxsIdStatistics.h 
	src/gui/statistics/dhtgraph.h 
	src/gui/statistics/Histogram.h 
	src/gui/statistics/BandwidthGraphWindow.h 
	src/gui/statistics/turtlegraph.h 
	src/gui/statistics/BandwidthStatsWidget.h 
	src/gui/statistics/DhtWindow.h 
	src/gui/statistics/GlobalRouterStatistics.h 
	src/gui/statistics/GxsTransportStatistics.h 
	src/gui/statistics/StatisticsWindow.h 
	src/gui/statistics/BwCtrlWindow.h 
	src/gui/statistics/BWGraph.h 
	src/gui/statistics/RttStatistics.h 

	src/gui/plugins/PluginInterface.h 

	src/gui/im_history/ImHistoryBrowser.h 
	src/gui/im_history/IMHistoryItemDelegate.h 
	src/gui/im_history/IMHistoryItemPainter.h 

	src/lang/languagesupport.h 

	src/util/RsSyntaxHighlighter.h 
	src/util/imageutil.h 
	src/util/RichTextEdit.h 
	src/util/retroshareWin32.h
	src/util/RsProtectedTimer.h 
	src/util/stringutil.h 
	src/util/RsNetUtil.h 
	src/util/DateTime.h 
	src/util/RetroStyleLabel.h 
	src/util/dllexport.h 
	src/util/NonCopyable.h 
	src/util/rsutildll.h 
	src/util/dllexport.h 
	src/util/global.h 
	src/util/rsqtutildll.h 
	src/util/Interface.h 
	src/util/PixmapMerging.h 
	src/util/MouseEventFilter.h 
	src/util/EventFilter.h 
	src/util/Widget.h 
	src/util/RsAction.h 
	src/util/RsUserdata.h 
	src/util/printpreview.h 
	src/util/log.h 
	src/util/misc.h 
	src/util/HandleRichText.h 
	src/util/ObjectPainter.h 
	src/util/QtVersion.h 
	src/util/RsFile.h 
	src/util/qtthreadsutils.h 
	src/util/ClickableLabel.h 
	src/util/AspectRatioPixmapLabel.h 

	src/gui/profile/ProfileWidget.h 
	src/gui/profile/ProfileManager.h 
	src/gui/profile/StatusMessage.h 

	src/gui/chat/PopupChatWindow.h 
	src/gui/chat/PopupChatDialog.h 
	src/gui/chat/PopupDistantChatDialog.h 
	src/gui/chat/ChatTabWidget.h 
	src/gui/chat/ChatWidget.h 
	src/gui/chat/ChatDialog.h 
	src/gui/ChatLobbyWidget.h 
	src/gui/chat/ChatLobbyDialog.h 
	src/gui/chat/CreateLobbyDialog.h 
	src/gui/chat/ChatStyle.h 
	src/gui/chat/ChatUserNotify.h 
	src/gui/chat/ChatLobbyUserNotify.h 

	src/gui/connect/ConfCertDialog.h 
	src/gui/connect/PGPKeyDialog.h 
	src/gui/connect/FriendRecommendDialog.h 

	src/gui/msgs/MessagesDialog.h 
	src/gui/msgs/MessageInterface.h 
	src/gui/msgs/MessageComposer.h 
	src/gui/msgs/MessageWindow.h 
	src/gui/msgs/MessageWidget.h 
	src/gui/msgs/MessageModel.h 
	src/gui/msgs/TagsMenu.h 
	src/gui/msgs/textformat.h 
	src/gui/msgs/MessageUserNotify.h 

	src/gui/images/retroshare_win.rc.h 

	src/gui/settings/RSPermissionMatrixWidget.h 
	src/gui/settings/rsharesettings.h 
	src/gui/settings/RsharePeerSettings.h 
	src/gui/settings/rsettings.h 
	src/gui/settings/rsettingswin.h 
	src/gui/settings/GeneralPage.h 
	src/gui/settings/PeoplePage.h 
	src/gui/settings/AboutPage.h 
	src/gui/settings/ServerPage.h 
	src/gui/settings/NotifyPage.h 
	src/gui/settings/CryptoPage.h 
	src/gui/settings/MessagePage.h 
	src/gui/settings/NewTag.h 
	src/gui/settings/ForumPage.h 
	src/gui/settings/PluginsPage.h 
	src/gui/settings/PluginItem.h 
	src/gui/settings/AppearancePage.h 
	src/gui/settings/FileAssociationsPage.h 
	src/gui/settings/SoundPage.h 
	src/gui/settings/TransferPage.h 
	src/gui/settings/ChatPage.h 
	src/gui/settings/ChannelPage.h 
	src/gui/settings/PostedPage.h 
	src/gui/settings/ServicePermissionsPage.h 
	src/gui/settings/AddFileAssociationDialog.h 
	src/gui/settings/GroupFrameSettingsWidget.h 

	src/gui/toaster/ToasterItem.h 
	src/gui/toaster/MessageToaster.h 
	src/gui/toaster/OnlineToaster.h 
	src/gui/toaster/DownloadToaster.h 
	src/gui/toaster/ChatToaster.h 
	src/gui/toaster/GroupChatToaster.h 
	src/gui/toaster/ChatLobbyToaster.h 
	src/gui/toaster/FriendRequestToaster.h 

	src/gui/common/RsButtonOnText.h 
	src/gui/common/RsCollection.h 
	src/gui/common/RSGraphWidget.h 
	src/gui/common/ElidedLabel.h 
	src/gui/common/vmessagebox.h 
	src/gui/common/RsUrlHandler.h 
	src/gui/common/RsCollectionDialog.h 
	src/gui/common/rwindow.h 
	src/gui/common/rshtml.h 
	src/gui/common/AvatarDefs.h 
	src/gui/common/GroupFlagsWidget.h 
	src/gui/common/GroupSelectionBox.h 
	src/gui/common/GroupChooser.h 
	src/gui/common/StatusDefs.h 
	src/gui/common/TagDefs.h 
	src/gui/common/GroupDefs.h 
	src/gui/common/Emoticons.h 
	src/gui/common/RSComboBox.h 
	src/gui/common/RSListWidgetItem.h 
	src/gui/common/RSTextEdit.h 
	src/gui/common/RSPlainTextEdit.h 
	src/gui/common/RSTreeWidget.h 
	src/gui/common/RSTreeWidgetItem.h 
	src/gui/common/RSFeedWidget.h 
	src/gui/common/RSTabWidget.h 
	src/gui/common/RSElidedItemDelegate.h 
	src/gui/common/RSItemDelegate.h 
	src/gui/common/PeerDefs.h 
	src/gui/common/FilesDefs.h 
	src/gui/common/PopularityDefs.h 
	src/gui/common/RsBanListDefs.h 
	src/gui/common/GroupTreeWidget.h 
	src/gui/common/RSTreeView.h 
	src/gui/common/AvatarWidget.h 
	src/gui/common/FriendListModel.h 
	src/gui/common/NewFriendList.h 
	src/gui/common/FriendSelectionWidget.h 
	src/gui/common/FriendSelectionDialog.h 
	src/gui/common/HashBox.h 
	src/gui/common/LineEditClear.h 
	src/gui/common/DropLineEdit.h 
	src/gui/common/RSTextBrowser.h 
	src/gui/common/RSImageBlockWidget.h 
	src/gui/common/FeedNotify.h 
	src/gui/common/UserNotify.h 
	src/gui/common/HeaderFrame.h 
	src/gui/common/MimeTextEdit.h 
	src/gui/common/UIStateHelper.h 
	src/gui/common/FloatingHelpBrowser.h 
	src/gui/common/SubscribeToolButton.h 
	src/gui/common/RsBanListToolButton.h 
	src/gui/common/FlowLayout.h 
	src/gui/common/PictureFlow.h 
	src/gui/common/ToasterNotify.h 
	src/gui/common/NotifyWidget.h 

	src/gui/style/RSStyle.h 
	src/gui/style/StyleDialog.h 

	src/gui/help/browser/helpbrowser.h 
	src/gui/help/browser/helptextbrowser.h 

	src/gui/statusbar/peerstatus.h 
	src/gui/statusbar/natstatus.h 
	src/gui/statusbar/dhtstatus.h 
	src/gui/statusbar/torstatus.h 
	src/gui/statusbar/ratesstatus.h 
	src/gui/statusbar/hashingstatus.h 
	src/gui/statusbar/discstatus.h 
	src/gui/statusbar/SoundStatus.h 
	src/gui/statusbar/OpModeStatus.h 
	src/gui/statusbar/ToasterDisable.h 
	src/gui/statusbar/SysTrayStatus.h 

	src/gui/advsearch/advancedsearchdialog.h 
	src/gui/advsearch/expressionwidget.h 
	src/gui/advsearch/guiexprelement.h 

	src/gui/elastic/graphwidget.h 
	src/gui/elastic/edge.h 
	src/gui/elastic/arrow.h 
	src/gui/elastic/elnode.h 

	src/gui/feeds/BoardsCommentsItem.h 
	src/gui/feeds/FeedItem.h 
	src/gui/feeds/FeedHolder.h 
	src/gui/feeds/GxsCircleItem.h 
	src/gui/feeds/ChannelsCommentsItem.h 
	src/gui/feeds/PeerItem.h 
	src/gui/feeds/MsgItem.h 
	src/gui/feeds/ChatMsgItem.h 
	src/gui/feeds/SubFileItem.h 
	src/gui/feeds/AttachFileItem.h 
	src/gui/feeds/SecurityItem.h 
	src/gui/feeds/SecurityIpItem.h 
	src/gui/feeds/NewsFeedUserNotify.h 

	src/gui/connect/ConnectFriendWizard.h 
	src/gui/connect/ConnectProgressDialog.h 

	src/gui/groups/CreateGroup.h 
	)

if(RS_JSON_API)
	list(
		APPEND RS_GUI_SOURCES
		src/gui/settings/JsonApiPage.cc
	)

	list(
		APPEND RS_IMPLEMENTATION_HEADERS
		src/gui/settings/JsonApiPage.h
	)

	list(
		APPEND RS_GUI_FORMS
		src/gui/settings/JsonApiPage.ui
	)
endif(RS_JSON_API)

if(RS_WEBUI)
	list(
		APPEND RS_GUI_SOURCES
		src/gui/settings/WebuiPage.cpp
	)

	list(
		APPEND RS_IMPLEMENTATION_HEADERS
		src/jsonapi/p3webui.h 
		src/gui/settings/WebuiPage.h 
	)

	list(
		APPEND RS_GUI_FORMS
		src/gui/settings/WebuiPage.ui
	)
endif(RS_WEBUI)

list(
	APPEND RS_GUI_QTRESOURCES
	src/gui/images.qrc 
	src/gui/icons.qrc 
	src/lang/lang.qrc 
	src/gui/help/content/content.qrc 
	src/gui/emojione.qrc 
	src/gui/qss/stylesheet/qdarkstyle/dark/Standard_Dark.qrc 
	src/gui/qss/stylesheet/qdarkstyle/light/Standard_Light.qrc
	)

list(
	APPEND RS_TRANSLATIONS
	src/lang/retroshare_ca_ES.ts 
	src/lang/retroshare_cs.ts 
	src/lang/retroshare_da.ts
	src/lang/retroshare_de.ts
	src/lang/retroshare_el.ts
	src/lang/retroshare_en.ts
	src/lang/retroshare_es.ts
	src/lang/retroshare_fi.ts
	src/lang/retroshare_fr.ts
	src/lang/retroshare_hu.ts
	src/lang/retroshare_it.ts
	src/lang/retroshare_ja_JP.ts 
	src/lang/retroshare_nl.ts 
	src/lang/retroshare_ko.ts 
	src/lang/retroshare_pl.ts 
	src/lang/retroshare_ru.ts 
	src/lang/retroshare_sv.ts 
	src/lang/retroshare_tr.ts 
	src/lang/retroshare_zh_CN.ts
	)

if(RS_UNFINISHED_TRANSLATIONS)
	list(
		APPEND RS_TRANSLATIONS
		src/lang/retroshare_bg.ts 
		src/lang/retroshare_af.ts 
		src/lang/retroshare_pt.ts 
		src/lang/retroshare_sl.ts 
		src/lang/retroshare_sr.ts 
		src/lang/retroshare_zh_TW.ts
		)
endif(RS_UNFINISHED_TRANSLATIONS)

if(RS_MESSENGER)

	target_compile_definitions( ${PROJECT_NAME} PUBLIC MESSENGER_WINDOW )

	list(
		APPEND RS_GUI_SOURCES
		src/gui/MessengerWindow.cpp 
		src/gui/common/FriendList.cpp
		)
	list(
		APPEND RS_IMPLEMENTATION_HEADERS
		src/gui/MessengerWindow.h 
		src/gui/common/FriendList.h 
		)
	list(
		APPEND RS_GUI_FORMS
		src/gui/MessengerWindow.ui 
		src/gui/common/FriendList.ui
		)
endif(RS_MESSENGER)

if(RS_IDLE)
	list(
		APPEND RS_IMPLEMENTATION_HEADERS
		src/idle/idle.h
		)
	list(
		APPEND RS_GUI_SOURCES
		src/idle/idle.cpp 
		src/idle/idle_platform.cpp
		)
endif(RS_IDLE)

if(RS_FRAMECATCHER)
	list(
		APPEND RS_IMPLEMENTATION_HEADERS
		src/util/framecatcher.h
		)
	list(
		APPEND RS_GUI_SOURCES
		src/util/framecatcher.cpp
		)

	# TODO: LIBS += -lxine

	target_compile_definitions(
		${PROJECT_NAME} PUBLIC CHANNELS_FRAME_CATCHER
		)
endif(RS_FRAMECATCHER)

# Embedded Friend Server

if(RS_EFS)
	target_compile_definitions(
		${PROJECT_NAME} PUBLIC RS_EMBEDED_FRIEND_SERVER
		)
	list(
		APPEND RS_GUI_SOURCES
		src/gui/FriendServerControl.cpp
		)
	list(
		APPEND RS_IMPLEMENTATION_HEADERS
		src/gui/FriendServerControl.h
		)
	list(
		APPEND RS_GUI_FORMS
		src/gui/FriendServerControl.ui
		)
endif(RS_EFS)

# BELOW IS GXS Unfinished Services.
	
if(RS_UNFINISHED)

	list(	
		APPEND RS_IMPLEMENTATION_HEADERS
		src/gui/unfinished/ApplicationWindow.h 

		#		gui/unfinished/CalDialog.h 
		#		gui/unfinished/ExampleDialog.h 
		#		gui/unfinished/GamesDialog.h 
		#		gui/unfinished/profile/ProfileView.h 
		#		gui/unfinished/profile/ProfileEdit.h
		#		gui/unfinished/StatisticDialog.h 
		#		gui/unfinished/PhotoDialog.h 
		#		gui/unfinished/PhotoShow.h 
		)

	list(
		APPEND RS_GUI_FORMS
		src/gui/unfinished/ApplicationWindow.ui 

		#		gui/unfinished/CalDialog.ui 
		#		gui/unfinished/ExampleDialog.ui 
		#		gui/unfinished/GamesDialog.ui 
		#		gui/unfinished/profile/ProfileView.ui 
		#		gui/unfinished/profile/ProfileEdit.ui
		#		gui/unfinished/StatisticDialog.ui 
		#		gui/unfinished/PhotoDialog.ui 
		#		gui/unfinished/PhotoShow.ui 
		)

	list(
		APPEND RS_GUI_SOURCES 
		src/gui/unfinished/ApplicationWindow.cpp 

		#		gui/unfinished/CalDialog.cpp 
		#		gui/unfinished/ExampleDialog.cpp 
		#		gui/unfinished/GamesDialog.cpp 
		#		gui/unfinished/profile/ProfileView.cpp 
		#		gui/unfinished/profile/ProfileEdit.cpp
		#		gui/unfinished/StatisticDialog.cpp 
		#		gui/unfinished/PhotoDialog.cpp 
		#		gui/unfinished/PhotoShow.cpp 
		)

	target_compile_definitions(
		${PROJECT_NAME} PUBLIC UNFINISHED 
		)
endif(RS_UNFINISHED)

if(RS_GXSPHOTOSHARE)
	target_compile_definitions(
		${PROJECT_NAME} PUBLIC RS_USE_PHOTO 
		)

	list(
		APPEND RS_IMPLEMENTATION_HEADERS
		src/gui/PhotoShare/AlbumGroupDialog.h 
		src/gui/PhotoShare/AlbumExtra.h 
		src/gui/PhotoShare/PhotoDrop.h 
		src/gui/PhotoShare/AlbumItem.h 
		src/gui/PhotoShare/AlbumDialog.h 
		src/gui/PhotoShare/PhotoItem.h 
		src/gui/PhotoShare/PhotoShareItemHolder.h 
		src/gui/PhotoShare/PhotoShare.h 
		src/gui/PhotoShare/PhotoSlideShow.h 
		src/gui/PhotoShare/PhotoDialog.h
		)
	list(
		APPEND RS_GUI_FORMS
		src/gui/PhotoShare/AlbumExtra.ui 
		src/gui/PhotoShare/PhotoItem.ui 
		src/gui/PhotoShare/PhotoDialog.ui 
		src/gui/PhotoShare/AlbumItem.ui 
		src/gui/PhotoShare/AlbumDialog.ui 
		src/gui/PhotoShare/PhotoShare.ui 
		src/gui/PhotoShare/PhotoSlideShow.ui
		)

	list(
		APPEND RS_GUI_SOURCES
		src/gui/PhotoShare/AlbumGroupDialog.cpp 
		src/gui/PhotoShare/AlbumExtra.cpp 
		src/gui/PhotoShare/PhotoItem.cpp 
		src/gui/PhotoShare/PhotoDialog.cpp 
		src/gui/PhotoShare/PhotoDrop.cpp 
		src/gui/PhotoShare/AlbumItem.cpp 
		src/gui/PhotoShare/AlbumDialog.cpp 
		src/gui/PhotoShare/PhotoShareItemHolder.cpp 
		src/gui/PhotoShare/PhotoShare.cpp 
		src/gui/PhotoShare/PhotoSlideShow.cpp
		)

	list(
		APPEND RS_GUI_QTRESOURCES
		src/gui/PhotoShare/Photo_images.qrc
		)
endif(RS_GXSPHOTOSHARE)

if(RS_GXSWIKIPOS)
	target_compile_definitions(
		${PROJECT_NAME} PUBLIC RS_USE_WIKI 
		)

	target_include_directories(${PROJECT_NAME} PUBLIC ../../supportlibs/pegmarkdown)

	list(
		APPEND RS_IMPLEMENTATION_HEADERS
		src/gui/WikiPoos/WikiDialog.h 
		src/gui/WikiPoos/WikiAddDialog.h 
		src/gui/WikiPoos/WikiEditDialog.h 
		src/gui/gxs/WikiGroupDialog.h 
		src/gui/gxs/RsGxsUpdateBroadcastBase.h 
		src/gui/gxs/RsGxsUpdateBroadcastWidget.h 
		src/gui/gxs/RsGxsUpdateBroadcastPage.h 
		)

	list(
		APPEND RS_GUI_FORMS
		src/gui/WikiPoos/WikiDialog.ui 
		src/gui/WikiPoos/WikiAddDialog.ui 
		src/gui/WikiPoos/WikiEditDialog.ui 
		)

	list(
		APPEND RS_GUI_SOURCES
		src/gui/WikiPoos/WikiDialog.cpp 
		src/gui/WikiPoos/WikiAddDialog.cpp 
		src/gui/WikiPoos/WikiEditDialog.cpp 
		src/gui/gxs/WikiGroupDialog.cpp 
		src/gui/gxs/RsGxsUpdateBroadcastBase.cpp 
		src/gui/gxs/RsGxsUpdateBroadcastWidget.cpp 
		src/gui/gxs/RsGxsUpdateBroadcastPage.cpp 
		)

	list(
		APPEND RS_GUI_QTRESOURCES
		src/gui/WikiPoos/Wiki_images.qrc
		)
endif(RS_GXSWIKIPOS)
	
if(RS_GXSTHEWIRE)
	target_compile_definitions(
		${PROJECT_NAME} PUBLIC RS_USE_WIRE 
		)

	list(
		APPEND RS_IMPLEMENTATION_HEADERS
		src/gui/TheWire/WireDialog.h 
		src/gui/TheWire/WireGroupItem.h 
		src/gui/TheWire/WireGroupDialog.h 
		src/gui/TheWire/WireGroupExtra.h 
		src/gui/TheWire/PulseAddDialog.h 
		src/gui/TheWire/PulseViewItem.h 
		src/gui/TheWire/PulseTopLevel.h 
		src/gui/TheWire/PulseViewGroup.h 
		src/gui/TheWire/PulseReply.h 
		src/gui/TheWire/PulseReplySeperator.h 
		src/gui/TheWire/PulseMessage.h
		src/gui/TheWire/WireUserNotify.h
		src/gui/feeds/WireNotifyGroupItem.h 
		src/gui/feeds/WireNotifyPostItem.h
		)

	list(
		APPEND RS_GUI_FORMS
		src/gui/TheWire/WireDialog.ui 
		src/gui/TheWire/WireGroupItem.ui 
		src/gui/TheWire/WireGroupExtra.ui 
		src/gui/TheWire/PulseAddDialog.ui 
		src/gui/TheWire/PulseTopLevel.ui 
		src/gui/TheWire/PulseViewGroup.ui 
		src/gui/TheWire/PulseReply.ui 
		src/gui/TheWire/PulseReplySeperator.ui 
		src/gui/TheWire/PulseMessage.ui
		src/gui/feeds/WireNotifyGroupItem.ui
		src/gui/feeds/WireNotifyPostItem.ui
		)

	list(
		APPEND RS_GUI_SOURCES
		src/gui/TheWire/WireDialog.cpp 
		src/gui/TheWire/WireGroupItem.cpp 
		src/gui/TheWire/WireGroupDialog.cpp 
		src/gui/TheWire/WireGroupExtra.cpp 
		src/gui/TheWire/PulseAddDialog.cpp 
		src/gui/TheWire/PulseViewItem.cpp 
		src/gui/TheWire/PulseTopLevel.cpp 
		src/gui/TheWire/PulseViewGroup.cpp 
		src/gui/TheWire/PulseReply.cpp 
		src/gui/TheWire/PulseReplySeperator.cpp 
		src/gui/TheWire/PulseMessage.cpp 
		src/gui/TheWire/WireUserNotify.cpp
		src/gui/feeds/WireNotifyGroupItem.cpp
		src/gui/feeds/WireNotifyPostItem.cpp
		)

	list(
		APPEND RS_GUI_QTRESOURCES
		RESOURCES += src/gui/TheWire/TheWire_images.qrc
		)
endif(RS_GXSTHEWIRE)

if(RS_GXSIDENTITIES)
	list(
		APPEND RS_IMPLEMENTATION_HEADERS
		src/gui/Identity/IdDialog.h 
		src/gui/Identity/IdEditDialog.h 
		src/gui/Identity/IdDetailsDialog.h 
		)

	list(
		APPEND RS_GUI_FORMS
		src/gui/Identity/IdDialog.ui 
		src/gui/Identity/IdEditDialog.ui 
		src/gui/Identity/IdDetailsDialog.ui 
		)

	list(
		APPEND RS_GUI_SOURCES
		src/gui/Identity/IdDialog.cpp 
		src/gui/Identity/IdEditDialog.cpp 
		src/gui/Identity/IdDetailsDialog.cpp 
		)
endif(RS_GXSIDENTITIES)
	
if(RS_GXSCIRCLES)
	list(
		APPEND RS_IMPLEMENTATION_HEADERS
		src/gui/Circles/CirclesDialog.h 
		src/gui/Circles/CreateCircleDialog.h 
		src/gui/People/PeopleDialog.h
		src/gui/People/CircleWidget.h
		src/gui/People/IdentityWidget.h
		)

	list(
		APPEND RS_GUI_FORMS
		src/gui/Circles/CirclesDialog.ui 
		src/gui/Circles/CreateCircleDialog.ui 
		src/gui/People/PeopleDialog.ui 
		src/gui/People/CircleWidget.ui
		src/gui/People/IdentityWidget.ui
		)

	list(
		APPEND RS_GUI_SOURCES
		src/gui/Circles/CirclesDialog.cpp 
		src/gui/Circles/CreateCircleDialog.cpp 
		src/gui/People/PeopleDialog.cpp 
		src/gui/People/CircleWidget.cpp
		src/gui/People/IdentityWidget.cpp
		)
endif(RS_GXSCIRCLES)

if(RS_GXSGUI)
	list(
		APPEND RS_IMPLEMENTATION_HEADERS
		src/gui/gxs/GxsGroupDialog.h 
		src/gui/gxs/GxsIdDetails.h 
		src/gui/gxs/GxsIdChooser.h
		src/gui/gxs/GxsIdLabel.h 
		src/gui/gxs/GxsCircleChooser.h 
		src/gui/gxs/GxsCircleLabel.h 
		src/gui/gxs/GxsIdTreeWidgetItem.h 
		src/gui/gxs/GxsCommentTreeWidget.h 
		src/gui/gxs/GxsCommentContainer.h 
		src/gui/gxs/GxsCommentDialog.h 
		src/gui/gxs/GxsCreateCommentDialog.h 
		src/gui/gxs/GxsGroupFrameDialog.h 
		src/gui/gxs/GxsMessageFrameWidget.h 
		src/gui/gxs/GxsMessageFramePostWidget.h 
		src/gui/gxs/GxsGroupFeedItem.h 
		src/gui/gxs/GxsFeedItem.h 
		src/gui/gxs/GxsGroupShareKey.h 
		src/gui/gxs/GxsUserNotify.h 
		src/gui/gxs/GxsFeedWidget.h 
		src/util/TokenQueue.h 
		src/util/RsGxsUpdateBroadcast.h 
		src/gui/gxs/GxsStatisticsProvider.h
		)

	list(
		APPEND RS_GUI_FORMS
		src/gui/gxs/GxsGroupDialog.ui 
		src/gui/gxs/GxsCommentContainer.ui 
		src/gui/gxs/GxsCommentDialog.ui 
		src/gui/gxs/GxsCreateCommentDialog.ui 
		src/gui/gxs/GxsGroupFrameDialog.ui
		src/gui/gxs/GxsGroupShareKey.ui 
		)
	
	list(
		APPEND RS_GUI_SOURCES
		src/gui/gxs/GxsGroupDialog.cpp 
		src/gui/gxs/GxsIdDetails.cpp 
		src/gui/gxs/GxsIdChooser.cpp 
		src/gui/gxs/GxsIdLabel.cpp 
		src/gui/gxs/GxsCircleChooser.cpp 
		src/gui/gxs/GxsGroupShareKey.cpp 
		src/gui/gxs/GxsCircleLabel.cpp 
		src/gui/gxs/GxsIdTreeWidgetItem.cpp 
		src/gui/gxs/GxsCommentTreeWidget.cpp 
		src/gui/gxs/GxsCommentContainer.cpp 
		src/gui/gxs/GxsCommentDialog.cpp 
		src/gui/gxs/GxsCreateCommentDialog.cpp 
		src/gui/gxs/GxsGroupFrameDialog.cpp 
		src/gui/gxs/GxsMessageFrameWidget.cpp 
		src/gui/gxs/GxsMessageFramePostWidget.cpp 
		src/gui/gxs/GxsGroupFeedItem.cpp 
		src/gui/gxs/GxsFeedItem.cpp 
		src/gui/gxs/GxsUserNotify.cpp 
		src/gui/gxs/GxsFeedWidget.cpp 
		src/util/TokenQueue.cpp 
		src/util/RsGxsUpdateBroadcast.cpp 
		src/gui/gxs/GxsStatisticsProvider.cpp 
		)
endif(RS_GXSGUI)
	
if(RS_GXSFORUMS) 
	list(
		APPEND RS_IMPLEMENTATION_HEADERS 
		src/gui/gxsforums/GxsForumsDialog.h 
		src/gui/gxsforums/GxsForumGroupDialog.h 
		src/gui/gxsforums/CreateGxsForumMsg.h 
		src/gui/gxsforums/GxsForumThreadWidget.h 
		src/gui/gxsforums/GxsForumModel.h 
		src/gui/gxsforums/GxsForumUserNotify.h 
		src/gui/feeds/GxsForumGroupItem.h 
		src/gui/feeds/GxsForumMsgItem.h
		)

	list(
		APPEND RS_GUI_FORMS
		src/gui/gxsforums/CreateGxsForumMsg.ui 
		src/gui/gxsforums/GxsForumThreadWidget.ui 
		src/gui/feeds/GxsForumGroupItem.ui 
		src/gui/feeds/GxsForumMsgItem.ui
		)

	list(
		APPEND RS_GUI_SOURCES
		src/gui/gxsforums/GxsForumsDialog.cpp 
		src/gui/gxsforums/GxsForumGroupDialog.cpp 
		src/gui/gxsforums/CreateGxsForumMsg.cpp 
		src/gui/gxsforums/GxsForumThreadWidget.cpp 
		src/gui/gxsforums/GxsForumModel.cpp 
		src/gui/gxsforums/GxsForumUserNotify.cpp 
		src/gui/feeds/GxsForumGroupItem.cpp 
		src/gui/feeds/GxsForumMsgItem.cpp
		)
endif(RS_GXSFORUMS)
	
	
if(RS_GXSCHANNELS)
	list(
		APPEND RS_IMPLEMENTATION_HEADERS
		src/gui/gxschannels/GxsChannelDialog.h 
		src/gui/gxschannels/GxsChannelGroupDialog.h 
		src/gui/gxschannels/CreateGxsChannelMsg.h 
		src/gui/gxschannels/GxsChannelPostsWidgetWithModel.h 
		src/gui/gxschannels/GxsChannelPostsModel.h 
		src/gui/gxschannels/GxsChannelPostFilesModel.h 
		src/gui/gxschannels/GxsChannelPostThumbnail.h 
		src/gui/gxschannels/GxsChannelFilesStatusWidget.h 
		src/gui/feeds/GxsChannelGroupItem.h 
		src/gui/feeds/GxsChannelPostItem.h 
		src/gui/gxschannels/GxsChannelUserNotify.h
		)
	
	list(
		APPEND RS_GUI_FORMS
		src/gui/gxschannels/GxsChannelPostsWidgetWithModel.ui 
		src/gui/gxschannels/GxsChannelFilesStatusWidget.ui 
		src/gui/gxschannels/CreateGxsChannelMsg.ui 
		src/gui/feeds/GxsChannelGroupItem.ui 
		src/gui/feeds/GxsChannelPostItem.ui
		)
	
	list(
		APPEND RS_GUI_SOURCES
		src/gui/gxschannels/GxsChannelDialog.cpp 
		src/gui/gxschannels/GxsChannelPostsWidgetWithModel.cpp 
		src/gui/gxschannels/GxsChannelPostsModel.cpp 
		src/gui/gxschannels/GxsChannelPostFilesModel.cpp 
		src/gui/gxschannels/GxsChannelFilesStatusWidget.cpp 
                src/gui/gxschannels/GxsChannelPostThumbnail.cpp 
		src/gui/gxschannels/GxsChannelGroupDialog.cpp 
		src/gui/gxschannels/CreateGxsChannelMsg.cpp 
		src/gui/feeds/GxsChannelGroupItem.cpp 
		src/gui/feeds/GxsChannelPostItem.cpp 
		src/gui/gxschannels/GxsChannelUserNotify.cpp
		)
endif(RS_GXSCHANNELS)
	
if(RS_GXSPOSTED)
	list(
		APPEND RS_IMPLEMENTATION_HEADERS
		src/gui/Posted/PostedDialog.h 
		src/gui/Posted/PostedListWidgetWithModel.h 
		src/gui/Posted/PostedPostsModel.h 
		src/gui/Posted/BoardPostDisplayWidget.h 
		src/gui/Posted/PostedItem.h 
		src/gui/Posted/PostedCardView.h 
		src/gui/Posted/PostedGroupDialog.h 
		src/gui/feeds/PostedGroupItem.h 
		src/gui/Posted/PostedCreatePostDialog.h 
		src/gui/Posted/PhotoView.h 
		src/gui/Posted/PostedUserNotify.h
		)

	list(
		APPEND RS_GUI_FORMS
		src/gui/Posted/PostedListWidgetWithModel.ui 
		src/gui/feeds/PostedGroupItem.ui 
		src/gui/Posted/BoardPostDisplayWidget_compact.ui 
		src/gui/Posted/BoardPostDisplayWidget_card.ui 
		src/gui/Posted/PostedItem.ui 
		src/gui/Posted/PostedCardView.ui 
		src/gui/Posted/PostedCreatePostDialog.ui 
		src/gui/Posted/PhotoView.ui 
		)
	
	list(
		APPEND RS_GUI_SOURCES
		src/gui/Posted/PostedDialog.cpp 
		src/gui/Posted/PostedListWidgetWithModel.cpp 
		src/gui/Posted/BoardPostDisplayWidget.cpp 
		src/gui/Posted/PostedPostsModel.cpp 
		src/gui/feeds/PostedGroupItem.cpp 
		src/gui/Posted/PostedItem.cpp 
		src/gui/Posted/PostedCardView.cpp 
		src/gui/Posted/PostedGroupDialog.cpp 
		src/gui/Posted/PostedCreatePostDialog.cpp 
		src/gui/Posted/PhotoView.cpp 
		src/gui/Posted/PostedUserNotify.cpp
		)

	list(
		APPEND RS_GUI_QTRESOURCES
		src/gui/Posted/Posted_images.qrc
		)
endif(RS_GXSPOSTED)



