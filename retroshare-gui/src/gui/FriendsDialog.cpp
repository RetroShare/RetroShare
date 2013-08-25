/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2011 RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include <time.h>

#include <QColorDialog>
#include <QDropEvent>
#include <QFontDialog>
#include <QMenu>
#include <QScrollBar>
#include <QTextStream>
#include <QTextCodec>
#include <QTimer>
#include <QMessageBox>

#include "retroshare/rspeers.h"
#include <retroshare/rshistory.h>

#include "common/Emoticons.h"
#include "common/PeerDefs.h"
#include "chat/ChatUserNotify.h"
#include "connect/ConnectFriendWizard.h"
#include "groups/CreateGroup.h"
#include "im_history/ImHistoryBrowser.h"
#include "MainWindow.h"
#include "NewsFeed.h"
#include "notifyqt.h"
#include "profile/ProfileWidget.h"
#include "profile/StatusMessage.h"
#include "RetroShareLink.h"
#include "settings/rsharesettings.h"
#include "util/misc.h"
#include "util/HandleRichText.h"
#include "util/DateTime.h"
//#include "FriendRecommendDialog.h"
#include "FriendsDialog.h"
//#include "ServicePermissionDialog.h"
#include "NetworkView.h"
#include "NetworkDialog.h"

/* Images for Newsfeed icons */
#define IMAGE_NEWSFEED           ""
#define IMAGE_NEWSFEED_NEW       ":/images/message-state-new.png"
#define IMAGE_NETWORK2          ":/images/rs1.png"
#define IMAGE_PEERS         	":/images/groupchat.png"

/******
 * #define FRIENDS_DEBUG 1
 *****/

static FriendsDialog *instance = NULL;

/** Constructor */
FriendsDialog::FriendsDialog(QWidget *parent)
            : RsAutoUpdatePage(1500,parent)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    if (instance == NULL) {
        instance = this;
    }

    last_status_send_time = 0 ;
    inChatCharFormatChanged = false;

    connect( ui.mypersonalstatusLabel, SIGNAL(clicked()), SLOT(statusmessage()));
    connect( ui.actionSet_your_Avatar, SIGNAL(triggered()), this, SLOT(getAvatar()));
    connect( ui.actionSet_your_Personal_Message, SIGNAL(triggered()), this, SLOT(statusmessage()));
    connect( ui.addfileButton, SIGNAL(clicked() ), this , SLOT(addExtraFile()));
    connect( ui.actionAdd_Friend, SIGNAL(triggered()), this, SLOT(addFriend()));
	 connect( ui.actionFriendRecommendations, SIGNAL(triggered()), this, SLOT(recommendFriends()));
    connect( ui.actionServicePermission, SIGNAL(triggered()), this, SLOT(servicePermission()));
    connect( ui.filter_lineEdit, SIGNAL(textChanged(QString)), ui.friendList, SLOT(filterItems(QString)));

    ui.filter_lineEdit->setPlaceholderText(tr("Search")) ;
    ui.filter_lineEdit->showFilterIcon();

    ui.avatar->setFrameType(AvatarWidget::STATUS_FRAME);
    ui.avatar->setOwnId();

    ui.tabWidget->setTabPosition(QTabWidget::North);
    ui.tabWidget->addTab(networkView = new NetworkView(),QIcon(IMAGE_NETWORK2), tr("Local network"));
    ui.tabWidget->addTab(networkDialog = new NetworkDialog(),QIcon(IMAGE_PEERS), tr("Keyring"));

    //ui.tabWidget->addTab(new ProfileWidget(), tr("Profile"));
    //newsFeed = new NewsFeed();
    //int newsFeedTabIndex = ui.tabWidget->insertTab(0, newsFeed, tr("News Feed"));
    //ui.tabWidget->setCurrentIndex(newsFeedTabIndex);

    ui.tabWidget->hideCloseButton(0);
    ui.tabWidget->hideCloseButton(1);
    ui.tabWidget->hideCloseButton(2);
    ui.tabWidget->hideCloseButton(3);

    /* get the current text and text color of the tab bar */
    //newsFeedTabColor = ui.tabWidget->tabBar()->tabTextColor(newsFeedTabIndex);
    //newsFeedText = ui.tabWidget->tabBar()->tabText(newsFeedTabIndex);

    //connect(newsFeed, SIGNAL(newsFeedChanged(int)), this, SLOT(newsFeedChanged(int)));

    connect(ui.Sendbtn, SIGNAL(clicked()), this, SLOT(sendMsg()));
    connect(ui.emoticonBtn, SIGNAL(clicked()), this, SLOT(smileyWidgetgroupchat()));

    connect(ui.msgText,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(contextMenuMsgText(QPoint)));

    connect(ui.lineEdit,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(contextMenu(QPoint)));
    // reset text and color after removing all characters from the QTextEdit and after calling QTextEdit::clear
    connect(ui.lineEdit, SIGNAL(currentCharFormatChanged(QTextCharFormat)), this, SLOT(chatCharFormatChanged()));

    connect(ui.textboldChatButton, SIGNAL(clicked()), this, SLOT(setFont()));
    connect(ui.textunderlineChatButton, SIGNAL(clicked()), this, SLOT(setFont()));
    connect(ui.textitalicChatButton, SIGNAL(clicked()), this, SLOT(setFont()));
    connect(ui.fontsButton, SIGNAL(clicked()), this, SLOT(chooseFont()));
    connect(ui.colorChatButton, SIGNAL(clicked()), this, SLOT(chooseColor()));
    connect(ui.actionSave_History, SIGNAL(triggered()), this, SLOT(fileSaveAs()));

    connect(ui.hashBox, SIGNAL(fileHashingFinished(QList<HashedFile>)), this, SLOT(fileHashingFinished(QList<HashedFile>)));

    ui.fontsButton->setIcon(QIcon(QString(":/images/fonts.png")));

    mCurrentColor = Qt::black;
    mCurrentFont.fromString(Settings->getChatScreenFont());

    colorChanged();
    fontChanged();
    setColorAndFont();

    style.setStyleFromSettings(ChatStyle::TYPE_PUBLIC);

    setChatInfo(tr("Retroshare broadcast chat: messages are sent to all connected friends."), QString::fromUtf8("blue"));

    if (rsHistory->getEnable(true)) {
        int messageCount = Settings->getPublicChatHistoryCount();
        if (messageCount > 0) {
            std::list<HistoryMsg> historyMsgs;
            rsHistory->getMessages("", historyMsgs, messageCount);

            std::list<HistoryMsg>::iterator it;
            for (it = historyMsgs.begin(); it != historyMsgs.end(); it++) {
                addChatMsg(it->incoming, true, QString::fromUtf8(it->peerName.c_str()), QDateTime::fromTime_t(it->sendTime), QDateTime::fromTime_t(it->recvTime), QString::fromUtf8(it->message.c_str()));
            }
        }
    }

    QMenu *menu = new QMenu();
    menu->addAction(ui.actionClear_Chat_History);
    menu->addAction(ui.actionDelete_Chat_History);
    menu->addAction(ui.actionSave_History);
    menu->addAction(ui.actionMessageHistory);
    ui.menuButton->setMenu(menu);

//    menu = new QMenu();
//    menu->addAction(ui.actionAdd_Friend);
//    menu->addAction(ui.actionAdd_Group);
//    menu->addAction(ui.actionCreate_new_Chat_lobby);
//    menu->addAction(ui.actionFriendRecommendations);
//    menu->addAction(ui.actionServicePermission);
//
//    menu->addSeparator();
//    menu->addAction(ui.actionSet_your_Avatar);
//    menu->addAction(ui.actionSet_your_Personal_Message);
//
//    ui.menutoolButton->setMenu(menu);

    setAcceptDrops(true);
    ui.lineEdit->setAcceptDrops(false);
    ui.hashBox->setDropWidget(this);
    ui.hashBox->setAutoHide(true);

    /* Set initial size the splitter */
    QList<int> sizes;
    sizes << height() << 100; // Qt calculates the right sizes
    ui.splitter_2->setSizes(sizes);

    loadmypersonalstatus();
    ui.displayButton->setMenu(ui.friendList->createDisplayMenu());

    // load settings
    RsAutoUpdatePage::lockAllEvents();
    ui.friendList->setShowStatusColumn(true);
    ui.friendList->setShowLastContactColumn(false);
    ui.friendList->setShowAvatarColumn(false);
    ui.friendList->setRootIsDecorated(true);
    ui.friendList->setShowGroups(true);
    processSettings(true);
    RsAutoUpdatePage::unlockAllEvents();

    ui.lineEdit->installEventFilter(this);

    // add self nick and Avatar to Friends.
    RsPeerDetails pd ;
    if (rsPeers->getPeerDetails(rsPeers->getOwnId(),pd)) {
        ui.nicknameLabel->setText(PeerDefs::nameWithLocation(pd));
    }

	QString hlp_str = tr(
		" <h1><img width=\"32\" src=\":/images/64px_help.png\">&nbsp;&nbsp;Friends</h1>                                   \
		  <p>The Friends tab shows...your friends: the list of persons you have accepted to connect to.                   \
		  </p>                                                   \
		  <p>You can group friends together to allow a finer level of information access, for instance to only allow      \
		  some friends to see some files.</p> \
		  <p>On the right, you will find 3 useful tabs:                                                                   \
		  <ul>	                                                                                                         \
		  		<li>Broadcast sends messages to all connected friends at once</li>                             \
		  		<li>Local Network shows the network around you, including friends of your friends</li>                 \
		  		<li>Keyring contains keys you collected, mostly forwarded to you by your friends</li>                              \
		  </ul> </p>                                                                                                      \
		") ;

	 registerHelpButton(ui.helpButton, hlp_str) ;

    /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

FriendsDialog::~FriendsDialog ()
{
    // save settings
    processSettings(false);

    if (this == instance) {
        instance = NULL;
    }
}

void FriendsDialog::activatePage(FriendsDialog::Page page)
{
	switch(page)
	{
		case FriendsDialog::NetworkTab: ui.tabWidget->setCurrentWidget(networkDialog) ;
											  break ;
		case FriendsDialog::BroadcastTab: ui.tabWidget->setCurrentWidget(networkDialog) ;
											  break ;
		case FriendsDialog::NetworkViewTab: ui.tabWidget->setCurrentWidget(networkView) ;
											  break ;
	}
}

UserNotify *FriendsDialog::getUserNotify(QObject *parent)
{
    return new ChatUserNotify(parent);
}

void FriendsDialog::processSettings(bool bLoad)
{
    Settings->beginGroup(QString("FriendsDialog"));

    if (bLoad) {
        // load settings

        // state of splitter
        ui.splitter->restoreState(Settings->value("Splitter").toByteArray());
        ui.splitter_2->restoreState(Settings->value("GroupChatSplitter").toByteArray());
    } else {
        // save settings

        // state of splitter
        Settings->setValue("Splitter", ui.splitter->saveState());
        Settings->setValue("GroupChatSplitter", ui.splitter_2->saveState());
    }

    ui.friendList->processSettings(bLoad);

    Settings->endGroup();
}

void FriendsDialog::showEvent(QShowEvent *event)
{
    static bool first = true;
    if (first) {
        // Workaround: now the scroll position is correct calculated
        first = false;
        QScrollBar *scrollbar = ui.msgText->verticalScrollBar();
        scrollbar->setValue(scrollbar->maximum());
    }

    RsAutoUpdatePage::showEvent(event);
}

void FriendsDialog::pasteLink()
{
    ui.lineEdit->insertHtml(RSLinkClipboard::toHtml()) ;
}

void FriendsDialog::contextMenuMsgText(QPoint point)
{
    QMatrix matrix;
    matrix.translate(ui.msgText->horizontalScrollBar()->value(), ui.msgText->verticalScrollBar()->value());

    QMenu *contextMnu = ui.msgText->createStandardContextMenu(matrix.map(point));

    contextMnu->addSeparator();
    contextMnu->addAction(ui.actionClear_Chat_History);

    contextMnu->exec(ui.msgText->viewport()->mapToGlobal(point));
    delete(contextMnu);
}

void FriendsDialog::contextMenu(QPoint point)
{
    QMenu *contextMnu = ui.lineEdit->createStandardContextMenu(point);

    contextMnu->addSeparator();
    QAction *action = contextMnu->addAction(QIcon(":/images/pasterslink.png"), tr("Paste RetroShare Link"), this, SLOT(pasteLink()));
    action->setDisabled(RSLinkClipboard::empty());

    contextMnu->exec(QCursor::pos());
    delete(contextMnu);
}

void FriendsDialog::chatCharFormatChanged()
{
    if (inChatCharFormatChanged) {
        return;
    }

    inChatCharFormatChanged = true;

    // Reset font and color before inserting a character if edit box is empty
    // (color info disappears when the user deletes all text)
    if (ui.lineEdit->toPlainText().isEmpty()) {
        setColorAndFont();
    }

    inChatCharFormatChanged = false;
}

void FriendsDialog::updateDisplay()
{
}

void FriendsDialog::addFriend()
{
    std::string groupId = ui.friendList->getSelectedGroupId();

    ConnectFriendWizard connwiz (this);

    if (groupId.empty() == false) {
        connwiz.setGroup(groupId);
    }

    connwiz.exec ();
}

void FriendsDialog::resetStatusBar()
{
#ifdef FRIENDS_DEBUG
	std::cerr << "FriendsDialog: reseting status bar." << std::endl ;
#endif

	ui.statusStringLabel->setText(QString("")) ;
}

void FriendsDialog::updateStatusTyping()
{
    if(time(NULL) - last_status_send_time > 5)	// limit 'peer is typing' packets to at most every 10 sec
    {
#ifdef FRIENDS_DEBUG
        std::cerr << "FriendsDialog: sending group chat typing info." << std::endl ;
#endif

#ifdef ONLY_FOR_LINGUIST
        tr("is typing...");
#endif

        rsMsgs->sendGroupChatStatusString("is typing...");
        last_status_send_time = time(NULL) ;
    }
}

// Called by libretroshare through notifyQt to display the peer's status
//
void FriendsDialog::updateStatusString(const QString& peer_id, const QString& status_string)
{
#ifdef FRIENDS_DEBUG
    std::cerr << "FriendsDialog: received group chat typing info. updating gui." << std::endl ;
#endif

    QString status = QString::fromUtf8(rsPeers->getPeerName(peer_id.toStdString()).c_str()) + " " + tr(status_string.toAscii());
    ui.statusStringLabel->setText(status) ; // displays info for 5 secs.

    QTimer::singleShot(5000,this,SLOT(resetStatusBar())) ;
}

void FriendsDialog::updatePeerStatusString(const QString& peer_id,const QString& status_string,bool is_private_chat)
{
    if (!is_private_chat) {
#ifdef FRIENDS_DEBUG
        std::cerr << "Updating public chat msg from peer " << rsPeers->getPeerName(peer_id.toStdString()) << ": " << status_string.toStdString() << std::endl ;
#endif

        updateStatusString(peer_id, status_string);
    }
}

void FriendsDialog::publicChatChanged(int type)
{
    if (type == NOTIFY_TYPE_ADD) {
        insertChat();
    }
}

void FriendsDialog::addChatMsg(bool incoming, bool history, const QString &name, const QDateTime &sendTime, const QDateTime &recvTime, const QString &message)
{
    unsigned int formatTextFlag = RSHTML_FORMATTEXT_EMBED_LINKS | RSHTML_FORMATTEXT_OPTIMIZE;

    // embed smileys ?
    if (Settings->valueFromGroup("Chat", "Emoteicons_GroupChat", true).toBool()) {
        formatTextFlag |= RSHTML_FORMATTEXT_EMBED_SMILEYS;
    }

	// Always fix colors
	formatTextFlag |= RSHTML_FORMATTEXT_FIX_COLORS;
	qreal desiredContrast = Settings->valueFromGroup("Chat", "MinimumContrast", 4.5).toDouble();
	QColor backgroundColor = ui.groupChatTab->palette().base().color();

	// Remove font name, size, bold, italics?
	if (!Settings->valueFromGroup("Chat", "EnableCustomFonts", true).toBool()) {
		formatTextFlag |= RSHTML_FORMATTEXT_REMOVE_FONT_FAMILY;
	}
	if (!Settings->valueFromGroup("Chat", "EnableCustomFontSize", true).toBool()) {
		formatTextFlag |= RSHTML_FORMATTEXT_REMOVE_FONT_SIZE;
	}
	if (!Settings->valueFromGroup("Chat", "EnableBold", true).toBool()) {
		formatTextFlag |= RSHTML_FORMATTEXT_REMOVE_FONT_WEIGHT;
	}
	if (!Settings->valueFromGroup("Chat", "EnableItalics", true).toBool()) {
		formatTextFlag |= RSHTML_FORMATTEXT_REMOVE_FONT_STYLE;
	}

    ChatStyle::enumFormatMessage type;
    if (incoming) {
        if (history) {
            type = ChatStyle::FORMATMSG_HINCOMING;
        } else {
            type = ChatStyle::FORMATMSG_INCOMING;
        }
    } else {
        if (history) {
            type = ChatStyle::FORMATMSG_HOUTGOING;
        } else {
            type = ChatStyle::FORMATMSG_OUTGOING;
        }
    }

    QString formattedMessage = RsHtml().formatText(ui.msgText->document(), message, formatTextFlag, backgroundColor, desiredContrast);
    QString formatMsg = style.formatMessage(type, name, incoming ? recvTime : sendTime, formattedMessage);

    ui.msgText->append(formatMsg);
}

void FriendsDialog::insertChat()
{
    std::list<ChatInfo> newchat;
    if (!rsMsgs->getPublicChatQueue(newchat))
    {
#ifdef FRIENDS_DEBUG
        std::cerr << "no chat available." << std::endl ;
#endif
        return;
    }
#ifdef FRIENDS_DEBUG
    std::cerr << "got new chat." << std::endl;
#endif
    std::list<ChatInfo>::iterator it;

    /* add in lines at the bottom */
    for(it = newchat.begin(); it != newchat.end(); it++)
    {
        /* are they private? */
        if (it->chatflags & RS_CHAT_PRIVATE)
        {
            /* this should not happen */
            continue;
        }

        QDateTime sendTime = QDateTime::fromTime_t(it->sendTime);
        QDateTime recvTime = QDateTime::fromTime_t(it->recvTime);
        QString name = QString::fromUtf8(rsPeers->getPeerName(it->rsid).c_str());
        QString msg = QString::fromStdWString(it->msg);

#ifdef FRIENDS_DEBUG
        std::cerr << "FriendsDialog::insertChat(): " << msg.toStdString() << std::endl;
#endif

        bool incoming = false;

        // notify with a systray icon msg
        if(it->rsid != rsPeers->getOwnId())
        {
            incoming = true;

            // This is a trick to translate HTML into text.
            QTextEdit editor;
            editor.setHtml(msg);
            QString notifyMsg = name + ": " + editor.toPlainText();

            if(notifyMsg.length() > 30)
                emit notifyGroupChat(tr("New group chat"), notifyMsg.left(30) + QString("..."));
            else
                emit notifyGroupChat(tr("New group chat"), notifyMsg);
        }

        addChatMsg(incoming, false, name, sendTime, recvTime, msg);
    }
}

bool FriendsDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui.lineEdit) {
        if (event->type() == QEvent::KeyPress) {
            updateStatusTyping() ;

            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent && (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)) {
                // Enter pressed
                if (Settings->getChatSendMessageWithCtrlReturn()) {
                    if (keyEvent->modifiers() & Qt::ControlModifier) {
                        // send message with Ctrl+Enter
                        sendMsg();
                        return true; // eat event
                    }
                } else {
                    if (keyEvent->modifiers() & Qt::ControlModifier) {
                        // insert return
                        ui.lineEdit->textCursor().insertText("\n");
                    } else {
                        // send message with Enter
                        sendMsg();
                    }
                    return true; // eat event
                }
            }
        }
    }
    // pass the event on to the parent class
    return RsAutoUpdatePage::eventFilter(obj, event);
}

void FriendsDialog::sendMsg()
{
    QTextEdit *lineWidget = ui.lineEdit;

    if (lineWidget->toPlainText().isEmpty()) {
        // nothing to send
        return;
    }

    QString text;
    RsHtml::optimizeHtml(lineWidget, text);
    std::wstring message = text.toStdWString();

#ifdef FRIENDS_DEBUG
    std::string msg(message.begin(), message.end());
    std::cerr << "FriendsDialog::sendMsg(): " << msg << std::endl;
#endif

    rsMsgs->sendPublicChat(message);
    ui.lineEdit->clear();
    // workaround for Qt bug - http://bugreports.qt.nokia.com/browse/QTBUG-2533
    // QTextEdit::clear() does not reset the CharFormat if document contains hyperlinks that have been accessed.
    ui.lineEdit->setCurrentCharFormat(QTextCharFormat ());

    /* redraw send list */
    insertSendList();
}

void  FriendsDialog::insertSendList()
{
#ifdef false
	std::list<std::string> peers;
	std::list<std::string>::iterator it;

	if (!rsPeers)
	{
		/* not ready yet! */
		return;
	}

	rsPeers->getOnlineList(peers);

        /* get a link to the table */
        //QTreeWidget *sendWidget = ui.msgSendList;
    QList<QTreeWidgetItem *> items;

	for(it = peers.begin(); it != peers.end(); it++)
	{

		RsPeerDetails details;
		if (!rsPeers->getPeerDetails(*it, details))
		{
			continue; /* BAD */
		}

        /* make a widget per friend */
            QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

		/* add all the labels */
		/* (0) Person */
		item -> setText(0, QString::fromStdString(details.name));

		item -> setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		//item -> setFlags(Qt::ItemIsUserCheckable);

		item -> setCheckState(0, Qt::Checked);

		if (rsicontrol->IsInChat(*it))
		{
			item -> setCheckState(0, Qt::Checked);
		}
		else
		{
			item -> setCheckState(0, Qt::Unchecked);
		}

		/* disable for the moment */
		item -> setFlags(Qt::ItemIsUserCheckable);
		item -> setCheckState(0, Qt::Checked);

		/* add to the list */
		items.append(item);
	}

		/* remove old items */
	//sendWidget->clear();
	//sendWidget->setColumnCount(1);

	/* add the items in! */
	//sendWidget->insertTopLevelItems(0, items);

	//sendWidget->update(); /* update display */
#endif
}


/* to toggle the state */


//void FriendsDialog::toggleSendItem( QTreeWidgetItem *item, int col )
//{
//#ifdef FRIENDS_DEBUG
//	std::cerr << "ToggleSendItem()" << std::endl;
//#endif
//
//	/* extract id */
//	std::string id = (item -> text(4)).toStdString();
//
//	/* get state */
//	bool inChat = (Qt::Checked == item -> checkState(0)); /* alway column 0 */
//
//	/* call control fns */
//
//	rsicontrol -> SetInChat(id, inChat);
//	return;
//}

//============================================================================

void FriendsDialog::chooseColor()
{
    bool ok;
    QRgb color = QColorDialog::getRgba(ui.lineEdit->textColor().rgba(), &ok, this);
    if (ok) {
        mCurrentColor = QColor(color);
        colorChanged();
        setColorAndFont();
    }
}

void FriendsDialog::colorChanged()
{
    QPixmap pxm(16,16);
    pxm.fill(mCurrentColor);
    ui.colorChatButton->setIcon(pxm);
}

void FriendsDialog::chooseFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, mCurrentFont, this);
    if (ok) {
        mCurrentFont = font;
        fontChanged();
        setFont();
    }
}

void FriendsDialog::fontChanged()
{
    ui.textboldChatButton->setChecked(mCurrentFont.bold());
    ui.textunderlineChatButton->setChecked(mCurrentFont.underline());
    ui.textitalicChatButton->setChecked(mCurrentFont.italic());
}

void FriendsDialog::setColorAndFont()
{
    mCurrentFont.setBold(ui.textboldChatButton->isChecked());
    mCurrentFont.setUnderline(ui.textunderlineChatButton->isChecked());
    mCurrentFont.setItalic(ui.textitalicChatButton->isChecked());

    ui.lineEdit->setFont(mCurrentFont);
    ui.lineEdit->setTextColor(mCurrentColor);

    ui.lineEdit->setFocus();
}

void FriendsDialog::setFont()
{
    setColorAndFont();
    Settings->setChatScreenFont(mCurrentFont.toString());
}

// Update Chat Info information
void FriendsDialog::setChatInfo(QString info, QColor color)
{
  static unsigned int nbLines = 0;
  ++nbLines;
  // Check log size, clear it if too big
  if(nbLines > 200) {
    ui.msgText->clear();
    nbLines = 1;
  }
  ui.msgText->append("<font color='grey'>" + DateTime::formatTime(QTime::currentTime()) + "</font> - <font color='" + color.name() + "'><i>" + info + "</i></font>");
}

void FriendsDialog::on_actionClear_Chat_History_triggered()
{
    ui.msgText->clear();
}

void FriendsDialog::on_actionDelete_Chat_History_triggered()
{
    if ((QMessageBox::question(this, "RetroShare", tr("Do you really want to physically delete the history?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)) == QMessageBox::Yes) {
        on_actionClear_Chat_History_triggered();
        rsHistory->clear("");
    }
}

void FriendsDialog::smileyWidgetgroupchat()
{
    Emoticons::showSmileyWidget(this, ui.emoticonBtn, SLOT(addSmileys()), true);
}

void FriendsDialog::addSmileys()
{
    ui.lineEdit->textCursor().insertText(qobject_cast<QPushButton*>(sender())->toolTip().split("|").first());
}

void FriendsDialog::getAvatar()
{
	QByteArray ba;
	if (misc::getOpenAvatarPicture(this, ba))
	{
#ifdef FRIENDS_DEBUG
		std::cerr << "Avatar image size = " << ba.size() << std::endl ;
#endif

		rsMsgs->setOwnAvatarData((unsigned char *)(ba.data()), ba.size()) ;	// last char 0 included.
	}
}

/** Loads own personal status */
void FriendsDialog::loadmypersonalstatus()
{
    ui.mypersonalstatusLabel->setText(QString::fromUtf8(rsMsgs->getCustomStateString().c_str()));
}

void FriendsDialog::statusmessage()
{
    StatusMessage statusmsgdialog (this);
    statusmsgdialog.exec();
}

void FriendsDialog::addExtraFile()
{
    QStringList files;
    if (misc::getOpenFileNames(this, RshareSettings::LASTDIR_EXTRAFILE, tr("Add Extra File"), "", files)) {
        ui.hashBox->addAttachments(files,TransferRequestFlags(0u));	// no anonymous routing, because it is for friends only!
    }
}

void FriendsDialog::fileHashingFinished(QList<HashedFile> hashedFiles)
{
    std::cerr << "FriendsDialog::fileHashingFinished() started." << std::endl;

    QString mesgString;

    QList<HashedFile>::iterator it;
    for (it = hashedFiles.begin(); it != hashedFiles.end(); ++it) {
        HashedFile& hashedFile = *it;
        RetroShareLink link;

        if (!link.createExtraFile(hashedFile.filename, hashedFile.size, QString::fromStdString(hashedFile.hash),QString::fromStdString(rsPeers->getOwnId())))
            continue;

        mesgString += link.toHtmlSize();
        if (it!= hashedFiles.end()) {
            mesgString += "<BR>";
        }
    }

#ifdef FRIENDS_DEBUG
    std::cerr << "FriendsDialog::fileHashingFinished mesgString : " << mesgString.toStdString() << std::endl;
#endif

    ui.lineEdit->insertHtml(mesgString);
}

bool FriendsDialog::fileSave()
{
    if (fileName.isEmpty())
        return fileSaveAs();

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly))
        return false;
    QTextStream ts(&file);
    ts.setCodec(QTextCodec::codecForName("UTF-8"));
    ts << ui.msgText->document()->toPlainText();
    ui.msgText->document()->setModified(false);
    return true;
}

bool FriendsDialog::fileSaveAs()
{
    QString fn;
    if (misc::getSaveFileName(this, RshareSettings::LASTDIR_HISTORY, tr("Save as..."), tr("Text File (*.txt );;All Files (*)"), fn)) {
        setCurrentFileName(fn);
        return fileSave();
    }

    return false;
}

void FriendsDialog::setCurrentFileName(const QString &fileName)
{
    this->fileName = fileName;
    ui.msgText->document()->setModified(false);

    setWindowModified(false);
}

void FriendsDialog::on_actionMessageHistory_triggered()
{
    ImHistoryBrowser imBrowser("", ui.lineEdit, this);
    imBrowser.exec();
}

/*static*/ bool FriendsDialog::isGroupChatActive()
{
	FriendsDialog *friendsDialog = dynamic_cast<FriendsDialog*>(MainWindow::getPage(MainWindow::Friends));
	if (!friendsDialog) {
		return false;
	}

    if (friendsDialog->ui.tabWidget->currentWidget() == friendsDialog->ui.groupChatTab) {
        return true;
    }

    return false;
}

/*static*/ void FriendsDialog::groupChatActivate()
{
	FriendsDialog *friendsDialog = dynamic_cast<FriendsDialog*>(MainWindow::getPage(MainWindow::Friends));
	if (!friendsDialog) {
		return;
	}

	MainWindow::showWindow(MainWindow::Friends);
	friendsDialog->ui.tabWidget->setCurrentWidget(friendsDialog->ui.groupChatTab);
	friendsDialog->ui.lineEdit->setFocus();
}
