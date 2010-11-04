/****************************************************************
 *
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,  crypton
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

#include <QMessageBox>
#include <QTimer>
#include <QScrollBar>
#include <QCloseEvent>
#include <QColorDialog>
#include <QDateTime>
#include <QFontDialog>
#include <QDir>
#include <QFileDialog>
#include <QBuffer>
#include <QTextCodec>
#include <QSound>
#include <sys/stat.h>

#include "PopupChatDialog.h"
#include "gui/RetroShareLink.h"
#include "rshare.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsmsgs.h>
#include <retroshare/rsfiles.h>
#include "retroshare/rsinit.h"
#include <retroshare/rsnotify.h>
#include <retroshare/rsstatus.h>
#include <retroshare/rsiface.h>
#include "gui/settings/rsharesettings.h"
#include "gui/settings/RsharePeerSettings.h"
#include "gui/notifyqt.h"
#include "../RsAutoUpdatePage.h"
#include "gui/common/StatusDefs.h"
#include "gui/common/Emoticons.h"
#include "gui/im_history/ImHistoryBrowser.h"

#include "gui/feeds/AttachFileItem.h"
#include "gui/msgs/MessageComposer.h"
#include "gui/common/PeerDefs.h"

#include <time.h>

#define appDir QApplication::applicationDirPath()

#define IMAGE_WINDOW         ":/images/rstray3.png"
#define IMAGE_WINDOW_TYPING  ":/images/typing.png"


/*****
 * #define CHAT_DEBUG 1
 *****/

static std::map<std::string, PopupChatDialog *> chatDialogs;

// play sound when recv a message
void playsound()
{
    Settings->beginGroup("Sound");
        Settings->beginGroup("SoundFilePath");
            QString OnlineSound = Settings->value("NewChatMessage","").toString();
        Settings->endGroup();
        Settings->beginGroup("Enable");
            bool flag = Settings->value("NewChatMessage",false).toBool();
        Settings->endGroup();
    Settings->endGroup();

    if (!OnlineSound.isEmpty() && flag) {
        if (QSound::isAvailable()) {
            QSound::play(OnlineSound);
        }
    }
}

/** Default constructor */
PopupChatDialog::PopupChatDialog(std::string id, const QString name, QWidget *parent, Qt::WFlags flags)
  : QMainWindow(parent, flags), dialogId(id), dialogName(name),
    lastChatTime(0), lastChatName("")
    
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);
  
  Settings->loadWidgetInformation(this);
  this->move(qrand()%100, qrand()%100); //avoid to stack multiple popup chat windows on the same position

  m_bInsertOnVisible = true;
  m_manualDelete = false;

  last_status_send_time = 0 ;
  style.setStyleFromSettings(ChatStyle::TYPE_PRIVATE);

  /* Hide or show the frames */
  showAvatarFrame(true);  
  ui.infoframe->setVisible(false);
  ui.statusmessagelabel->hide();

  connect(ui.avatarFrameButton, SIGNAL(toggled(bool)), this, SLOT(showAvatarFrame(bool)));

  connect(ui.actionAvatar, SIGNAL(triggered()),this, SLOT(getAvatar()));

  connect(ui.sendButton, SIGNAL(clicked( ) ), this, SLOT(sendChat( ) ));
  connect(ui.addFileButton, SIGNAL(clicked() ), this , SLOT(addExtraFile()));

  connect(ui.textboldButton, SIGNAL(clicked()), this, SLOT(setFont()));  
  connect(ui.textunderlineButton, SIGNAL(clicked()), this, SLOT(setFont()));  
  connect(ui.textitalicButton, SIGNAL(clicked()), this, SLOT(setFont()));
  connect(ui.attachPictureButton, SIGNAL(clicked()), this, SLOT(addExtraPicture()));
  connect(ui.fontButton, SIGNAL(clicked()), this, SLOT(getFont())); 
  connect(ui.colorButton, SIGNAL(clicked()), this, SLOT(setColor()));
  connect(ui.emoteiconButton, SIGNAL(clicked()), this, SLOT(smileyWidget()));
  connect(ui.actionSave_Chat_History, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
  connect(ui.actionClearOfflineMessages, SIGNAL(triggered()), this, SLOT(clearOfflineMessages()));

  connect(ui.textBrowser, SIGNAL(anchorClicked(const QUrl &)), SLOT(anchorClicked(const QUrl &)));

  connect(NotifyQt::getInstance(), SIGNAL(peerStatusChanged(const QString&, int)), this, SLOT(updateStatus(const QString&, int)));
  connect(NotifyQt::getInstance(), SIGNAL(peerHasNewCustomStateString(const QString&, const QString&)), this, SLOT(updatePeersCustomStateString(const QString&, const QString&)));

  std::cerr << "Connecting custom context menu" << std::endl;
  ui.chattextEdit->setContextMenuPolicy(Qt::CustomContextMenu) ;
  connect(ui.chattextEdit,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(contextMenu(QPoint)));

  // Create the status bar
  resetStatusBar() ;

  //ui.textBrowser->setOpenExternalLinks ( false );
  //ui.textBrowser->setOpenLinks ( false );

  QString title = tr("RetroShare") + " - " + name;
  setWindowTitle(title);

  setWindowIcon(QIcon(IMAGE_WINDOW));
  
  ui.textboldButton->setIcon(QIcon(QString(":/images/edit-bold.png")));
  ui.textunderlineButton->setIcon(QIcon(QString(":/images/edit-underline.png")));
  ui.textitalicButton->setIcon(QIcon(QString(":/images/edit-italic.png")));
  ui.fontButton->setIcon(QIcon(QString(":/images/fonts.png")));
  ui.emoteiconButton->setIcon(QIcon(QString(":/images/emoticons/kopete/kopete020.png")));
  
  ui.textboldButton->setCheckable(true);
  ui.textunderlineButton->setCheckable(true);
  ui.textitalicButton->setCheckable(true);

  setAcceptDrops(true);
  ui.chattextEdit->setAcceptDrops(false);

  QMenu * toolmenu = new QMenu();
  toolmenu->addAction(ui.actionClear_Chat);
  toolmenu->addAction(ui.actionSave_Chat_History);
  toolmenu->addAction(ui.actionClearOfflineMessages);
  toolmenu->addAction(ui.actionMessageHistory);
  //toolmenu->addAction(ui.action_Disable_Emoticons);
  ui.pushtoolsButton->setMenu(toolmenu);

  mCurrentColor.setNamedColor(PeerSettings->getPrivateChatColor(dialogId));
  mCurrentFont.fromString(PeerSettings->getPrivateChatFont(dialogId));

  colorChanged(mCurrentColor);
  fontChanged(mCurrentFont);

  pasteLinkAct = new QAction(QIcon(":/images/pasterslink.png"), tr( "Paste retroshare Link" ), this );
  connect( pasteLinkAct , SIGNAL( triggered() ), this, SLOT( pasteLink() ) );

  updateAvatar() ;
  updatePeerAvatar(id) ;

  // load settings
  processSettings(true);

  // initialize first status
  StatusInfo peerStatusInfo;
  // No check of return value. Non existing status info is handled as offline.
  rsStatus->getStatus(dialogId, peerStatusInfo);
  updateStatus(QString::fromStdString(dialogId), peerStatusInfo.status);

  StatusInfo ownStatusInfo;
  if (rsStatus->getOwnStatus(ownStatusInfo)) {
    updateStatus(QString::fromStdString(ownStatusInfo.id), ownStatusInfo.status);
  }

  // initialize first custom state string
  QString customStateString = QString::fromStdString(rsMsgs->getCustomStateString(dialogId));
  updatePeersCustomStateString(QString::fromStdString(dialogId), customStateString);

  if (Settings->valueFromGroup("Chat", QString::fromUtf8("PrivateChat_History"), true).toBool()) {
      historyKeeper.init(QString::fromStdString(RsInit::RsProfileConfigDirectory()) + "/chat_" + QString::fromStdString(dialogId) +  ".xml");

      // get offline chat messages
      std::list<ChatInfo> offlineChat;
      std::list<ChatInfo>::iterator offineChatIt;
      rsMsgs->getPrivateChatQueueCount(false) && rsMsgs->getPrivateChatQueue(false, dialogId, offlineChat);

      QList<IMHistoryItem> historyItems;
      historyKeeper.getMessages(historyItems, Settings->getPrivateChatHistoryCount());
      foreach(IMHistoryItem item, historyItems) {
          for(offineChatIt = offlineChat.begin(); offineChatIt != offlineChat.end(); offineChatIt++) {
              /* are they public? */
              if ((offineChatIt->chatflags & RS_CHAT_PRIVATE) == 0) {
                  /* this should not happen */
                  continue;
              }

              QDateTime sendTime = QDateTime::fromTime_t(offineChatIt->sendTime);
              QString message = QString::fromStdWString(offineChatIt->msg);

              if (IMHistoryKeeper::compareItem(item, false, offineChatIt->rsid, sendTime, message)) {
                  // don't show offline message out of the history
                  break;
              }
          }

          if (offineChatIt == offlineChat.end()) {
              addChatMsg(item.incoming, item.id, item.name, item.sendTime, item.recvTime, item.messageText, TYPE_HISTORY, false);
          }
      }
  }

  ui.chattextEdit->installEventFilter(this);

  // call once
  onPrivateChatChanged(NOTIFY_LIST_PRIVATE_OUTGOING_CHAT, NOTIFY_TYPE_ADD, true);
}

/** Destructor. */
PopupChatDialog::~PopupChatDialog()
{
    // save settings
    processSettings(false);
}

void PopupChatDialog::processSettings(bool bLoad)
{
    Settings->beginGroup(QString("ChatDialog"));

    if (bLoad) {
        // load settings

        // state of splitter
        ui.chatsplitter->restoreState(Settings->value("ChatSplitter").toByteArray());
    } else {
        // save settings

        // state of splitter
        Settings->setValue("ChatSplitter", ui.chatsplitter->saveState());
    }

    Settings->endGroup();
}

/*static*/ PopupChatDialog *PopupChatDialog::getPrivateChat(std::string id, uint chatflags)
{
    /* see if it exists already */
    PopupChatDialog *popupchatdialog = NULL;
    bool show = false;

    if (chatflags & RS_CHAT_REOPEN)
    {
        show = true;
#ifdef PEERS_DEBUG
        std::cerr << "reopen flag so: enable SHOW popupchatdialog()" << std::endl;
#endif
    }

    std::map<std::string, PopupChatDialog *>::iterator it;
    if (chatDialogs.end() != (it = chatDialogs.find(id)))
    {
        /* exists already */
        popupchatdialog = it->second;
    }
    else
    {

        if (chatflags & RS_CHAT_OPEN_NEW)
        {
            RsPeerDetails sslDetails;
            if (rsPeers->getPeerDetails(id, sslDetails)) {
                popupchatdialog = new PopupChatDialog(id, PeerDefs::nameWithLocation(sslDetails));
                chatDialogs[id] = popupchatdialog;
#ifdef PEERS_DEBUG
                std::cerr << "new chat so: enable SHOW popupchatdialog()" << std::endl;
#endif

                show = true;
            }
        }
    }

    if (show && popupchatdialog)
    {
#ifdef PEERS_DEBUG
        std::cerr << "SHOWING popupchatdialog()" << std::endl;
#endif

        if (popupchatdialog->isVisible() == false) {
            if (chatflags & RS_CHAT_FOCUS) {
                popupchatdialog->show();
            } else {
                popupchatdialog->showMinimized();
            }
        }
    }

    /* now only do these if the window is visible */
    if (popupchatdialog && popupchatdialog->isVisible())
    {
        if (chatflags & RS_CHAT_FOCUS)
        {
#ifdef PEERS_DEBUG
            std::cerr << "focus chat flag so: GETFOCUS popupchatdialog()" << std::endl;
#endif

            popupchatdialog->getfocus();
        }
        else
        {
#ifdef PEERS_DEBUG
            std::cerr << "no focus chat flag so: FLASH popupchatdialog()" << std::endl;
#endif

            popupchatdialog->flash();
        }
    }
    else
    {
#ifdef PEERS_DEBUG
        std::cerr << "not visible ... so leave popupchatdialog()" << std::endl;
#endif
    }

    return popupchatdialog;
}

/*static*/ void PopupChatDialog::cleanupChat()
{
    std::map<std::string, PopupChatDialog *>::iterator it;
    for (it = chatDialogs.begin(); it != chatDialogs.end(); it++) {
        if (it->second) {
            delete (it->second);
        }
    }

    chatDialogs.clear();
}

/*static*/ void PopupChatDialog::privateChatChanged(int list, int type)
{
    if (list == NOTIFY_LIST_PRIVATE_INCOMING_CHAT && type == NOTIFY_TYPE_ADD) {
        std::list<std::string> ids;
        if (rsMsgs->getPrivateChatQueueIds(true, ids)) {
            uint chatflags = Settings->getChatFlags();

            std::list<std::string>::iterator id;
            for (id = ids.begin(); id != ids.end(); id++) {
                PopupChatDialog *pcd = getPrivateChat(*id, chatflags);

                if (pcd) {
                    pcd->insertChatMsgs();
                }
            }
        }
    }

    /* now notify all open priavate chat windows */
    std::map<std::string, PopupChatDialog *>::iterator it;
    for (it = chatDialogs.begin (); it != chatDialogs.end(); it++) {
        if (it->second) {
            it->second->onPrivateChatChanged(list, type);
        }
    }
}

void PopupChatDialog::chatFriend(std::string id)
{
    if (id.empty()){
        return;
    }
    std::cerr<<" popup dialog chat friend 1"<<std::endl;

    RsPeerDetails detail;
    if (!rsPeers->getPeerDetails(id, detail)) {
        return;
    }

    std::string firstId;

    if (detail.isOnlyGPGdetail) {
        //let's get the ssl child details, and open all the chat boxes
        std::list<std::string> sslIds;
        rsPeers->getSSLChildListOfGPGId(detail.gpg_id, sslIds);
        for (std::list<std::string>::iterator it = sslIds.begin(); it != sslIds.end(); it++) {
            if (firstId.empty()) {
                firstId = *it;
            }

            RsPeerDetails sslDetails;
            if (rsPeers->getPeerDetails(*it, sslDetails)) {
                if (sslDetails.state & RS_PEER_STATE_CONNECTED) {
                    getPrivateChat(*it, RS_CHAT_OPEN_NEW | RS_CHAT_REOPEN | RS_CHAT_FOCUS);
                    return;
                }
            }
        }
    } else {
        if (detail.state & RS_PEER_STATE_CONNECTED) {
            getPrivateChat(id, RS_CHAT_OPEN_NEW | RS_CHAT_REOPEN | RS_CHAT_FOCUS);
            return;
        }
        firstId = id;
    }

    /* info dialog */
    QMessageBox mb(QMessageBox::Question, tr("Friend not Online"), tr("Your Friend is offline \nDo you want to send them a Message instead"), QMessageBox::Yes | QMessageBox::No);
    mb.setWindowIcon(QIcon(":/images/rstray3.png"));
    if (mb.exec() == QMessageBox::Yes) {
        MessageComposer::msgFriend(id, false);
    } else {
        if (firstId.empty() == false) {
            getPrivateChat(firstId, RS_CHAT_OPEN_NEW | RS_CHAT_REOPEN | RS_CHAT_FOCUS);
        }
    }
}

/*static*/ void PopupChatDialog::updateAllAvatars()
{
    for(std::map<std::string, PopupChatDialog *>::const_iterator it(chatDialogs.begin());it!=chatDialogs.end();++it)
        it->second->updateAvatar() ;
}

void PopupChatDialog::pasteLink()
{
	std::cerr << "In paste link" << std::endl ;
	ui.chattextEdit->insertHtml(RSLinkClipboard::toHtml()) ;
}

void PopupChatDialog::contextMenu( QPoint point )
{
	std::cerr << "In context menu" << std::endl ;
	if(RSLinkClipboard::empty())
		return ;

	QMenu contextMnu(this);
	contextMnu.addAction( pasteLinkAct);

        contextMnu.exec(QCursor::pos());
}

void PopupChatDialog::resetStatusBar() 
{
        ui.statusLabel->clear();
        ui.typingpixmapLabel->clear();

        setWindowIcon(QIcon(IMAGE_WINDOW));
}

void PopupChatDialog::updateStatusTyping()
{
    if (time(NULL) - last_status_send_time > 5)	// limit 'peer is typing' packets to at most every 10 sec
    {
#ifdef ONLY_FOR_LINGUIST
        tr("is typing...");
#endif

        rsMsgs->sendStatusString(dialogId, "is typing...");
        last_status_send_time = time(NULL) ;
    }
}

// Called by libretroshare through notifyQt to display the peer's status
//
void PopupChatDialog::updateStatusString(const QString& peer_id, const QString& status_string)
{
    QString status = QString::fromStdString(rsPeers->getPeerName(peer_id.toStdString())) + " " + tr(status_string.toAscii());
    ui.statusLabel->setText(status) ; // displays info for 5 secs.
    ui.typingpixmapLabel->setPixmap(QPixmap(":images/typing.png") );

    if (status_string == "is typing...") {
        setWindowIcon(QIcon(IMAGE_WINDOW_TYPING));
    }

    QTimer::singleShot(5000,this,SLOT(resetStatusBar())) ;
}

/** 
 Overloads the default show() slot so we can set opacity*/

void PopupChatDialog::show()
{

  if(!this->isVisible()) {
    QMainWindow::show();
  } else {
    //QMainWindow::activateWindow();
    //setWindowState(windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
    //QMainWindow::raise();
  }
  
}

void PopupChatDialog::getfocus()
{

  QMainWindow::activateWindow();
  setWindowState((windowState() & (~Qt::WindowMinimized)) | Qt::WindowActive);
  QMainWindow::raise();
}

void PopupChatDialog::flash()
{

  if(!this->isVisible()) {
    //QMainWindow::show();
  } else {
    // Want to reduce the interference on other applications.
    //QMainWindow::activateWindow();
    //setWindowState(windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
    //QMainWindow::raise();
  }
  
}

void PopupChatDialog::showEvent(QShowEvent *event)
{
    if (m_bInsertOnVisible) {
        insertChatMsgs();
    }
}

void PopupChatDialog::closeEvent (QCloseEvent * event)
{
    Settings->saveWidgetInformation(this);

    hide();
    event->ignore();
}

void PopupChatDialog::onPrivateChatChanged(int list, int type, bool initial /*= false*/)
{
    if (list == NOTIFY_LIST_PRIVATE_OUTGOING_CHAT) {
        switch (type) {
        case NOTIFY_TYPE_ADD:
            {
                m_savedOfflineChat.clear();

                QString name = QString::fromUtf8(rsPeers->getPeerName(rsPeers->getOwnId()).c_str());

                std::list<ChatInfo> offlineChat;
                if (rsMsgs->getPrivateChatQueueCount(false) && rsMsgs->getPrivateChatQueue(false, dialogId, offlineChat)) {
                    ui.actionClearOfflineMessages->setEnabled(true);

                    std::list<ChatInfo>::iterator it;
                    for(it = offlineChat.begin(); it != offlineChat.end(); it++) {
                        /* are they public? */
                        if ((it->chatflags & RS_CHAT_PRIVATE) == 0) {
                            /* this should not happen */
                            continue;
                        }

                        m_savedOfflineChat.push_back(*it);

                        QDateTime sendTime = QDateTime::fromTime_t(it->sendTime);
                        QDateTime recvTime = QDateTime::fromTime_t(it->recvTime);
                        QString message = QString::fromStdWString(it->msg);

                        bool existingMessage;
                        bool showMessage;
                        if (initial) {
                            // show all messages on startup
                            existingMessage = true;
                            showMessage = true;
                        } else {
                            int hiid;
                            existingMessage = historyKeeper.findMessage(false, it->rsid, sendTime, message, hiid);
                            showMessage = !existingMessage;
                        }

                        if (showMessage) {
                            addChatMsg(false, it->rsid, name, sendTime, recvTime, message, TYPE_OFFLINE, !existingMessage);
                        }
                    }
                }
            }
            break;
        case NOTIFY_TYPE_DEL:
            {
                if (m_manualDelete == false) {
                    QString name = QString::fromUtf8(rsPeers->getPeerName(rsPeers->getOwnId()).c_str());

                    // now show saved offline chat messages as sent
                    std::list<ChatInfo>::iterator it;
                    for(it = m_savedOfflineChat.begin(); it != m_savedOfflineChat.end(); it++) {
                        QDateTime sendTime = QDateTime::fromTime_t(it->sendTime);
                        QDateTime recvTime = QDateTime::fromTime_t(it->recvTime);
                        QString message = QString::fromStdWString(it->msg);

                        addChatMsg(false, it->rsid, name, sendTime, recvTime, message, TYPE_NORMAL, false);
                    }
                }

                m_savedOfflineChat.clear();
            }
            break;
        }

        ui.actionClearOfflineMessages->setEnabled(!m_savedOfflineChat.empty());
    }
}

void PopupChatDialog::insertChatMsgs()
{
    if (isVisible() == false) {
        m_bInsertOnVisible = true;
        return;
    }

    m_bInsertOnVisible = false;

    std::list<ChatInfo> newchat;
    if (!rsMsgs->getPrivateChatQueue(true, dialogId, newchat))
    {
#ifdef PEERS_DEBUG
        std::cerr << "no chat for " << dialogId << " available." << std::endl ;
#endif
        return;
    }

    std::list<ChatInfo>::iterator it;
    for(it = newchat.begin(); it != newchat.end(); it++) {
        /* are they public? */
        if ((it->chatflags & RS_CHAT_PRIVATE) == 0) {
            /* this should not happen */
            continue;
        }

        addChatMsg(true, it->rsid, QString::fromStdString(rsPeers->getPeerName(it->rsid)), QDateTime::fromTime_t(it->sendTime), QDateTime::fromTime_t(it->recvTime), QString::fromStdWString(it->msg), TYPE_NORMAL, true);
    }

    rsMsgs->clearPrivateChatQueue(true, dialogId);

    playsound();
    QApplication::alert(this);
}

void PopupChatDialog::addChatMsg(bool incoming, const std::string &id, const QString &name, const QDateTime &sendTime, const QDateTime &recvTime, const QString &message, enumChatType chatType, bool addToHistory)
{
    std::string ownId = rsPeers->getOwnId();

#ifdef CHAT_DEBUG
    std::cout << "PopupChatDialog:addChatMsg message : " << message.toStdString() << std::endl;
#endif

    unsigned int formatFlag = CHAT_FORMATMSG_EMBED_LINKS;

    // embed smileys ?
    if (Settings->valueFromGroup(QString("Chat"), QString::fromUtf8("Emoteicons_PrivatChat"), true).toBool()) {
        formatFlag |= CHAT_FORMATMSG_EMBED_SMILEYS;
    }

    ChatStyle::enumFormatMessage type;
    if (chatType == TYPE_OFFLINE) {
        type = ChatStyle::FORMATMSG_OOUTGOING;
    } else if (chatType == TYPE_HISTORY) {
        type = incoming ? ChatStyle::FORMATMSG_HINCOMING : ChatStyle::FORMATMSG_HOUTGOING;
    } else {
        type = incoming ? ChatStyle::FORMATMSG_INCOMING : ChatStyle::FORMATMSG_OUTGOING;
    }

    QString formatMsg = style.formatMessage(type, name, recvTime, message, formatFlag);

    if (addToHistory) {
        historyKeeper.addMessage(incoming, id, name, sendTime, recvTime, message);
    }

    ui.textBrowser->append(formatMsg);

    resetStatusBar() ;
}

bool PopupChatDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui.chattextEdit) {
        if (event->type() == QEvent::KeyPress) {
            updateStatusTyping() ;

            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent && (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)) {
                // Enter pressed
                if (Settings->getChatSendMessageWithCtrlReturn()) {
                    if (keyEvent->modifiers() & Qt::ControlModifier) {
                        // send message with Ctrl+Enter
                        sendChat();
                        return true; // eat event
                    }
                } else {
                    if (keyEvent->modifiers() & Qt::ControlModifier) {
                        // insert return
                        ui.chattextEdit->textCursor().insertText("\n");
                    } else {
                        // send message with Enter
                        sendChat();
                    }
                    return true; // eat event
                }
            }
        }
    }
    // pass the event on to the parent class
    return QMainWindow::eventFilter(obj, event);
}

void PopupChatDialog::sendChat()
{
    QTextEdit *chatWidget = ui.chattextEdit;

    if (chatWidget->toPlainText().isEmpty()) {
        // nothing to send
        return;
    }

    std::wstring msg = chatWidget->toHtml().toStdWString();

    if (msg.empty()) {
        // nothing to send
        return;
    }

    std::string ownId;

    {
        rsiface->lockData(); /* Lock Interface */
        const RsConfig &conf = rsiface->getConfig();

        ownId = conf.ownId;

        rsiface->unlockData(); /* Unlock Interface */
    }

#ifdef CHAT_DEBUG 
    std::cout << "PopupChatDialog:sendChat " << std::endl;
#endif

    if (rsMsgs->sendPrivateChat(dialogId, msg)) {
        QDateTime currentTime = QDateTime::currentDateTime();
        addChatMsg(false, ownId, QString::fromStdString(rsPeers->getPeerName(ownId)), currentTime, currentTime, QString::fromStdWString(msg), TYPE_NORMAL, true);
    }

    chatWidget->clear();
    // workaround for Qt bug - http://bugreports.qt.nokia.com/browse/QTBUG-2533
    // QTextEdit::clear() does not reset the CharFormat if document contains hyperlinks that have been accessed.
    chatWidget->setCurrentCharFormat(QTextCharFormat ());

    setFont();

    /* redraw send list */
}

/**
 Toggles the ToolBox on and off, changes toggle button text
 */
void PopupChatDialog::showAvatarFrame(bool show)
{
    if (show) {
        ui.avatarframe->setVisible(true);
        ui.avatarFrameButton->setChecked(true);
        ui.avatarFrameButton->setToolTip(tr("Hide Avatar"));
        ui.avatarFrameButton->setIcon(QIcon(":images/hide_toolbox_frame.png"));
    } else {
        ui.avatarframe->setVisible(false);
        ui.avatarFrameButton->setChecked(false);
        ui.avatarFrameButton->setToolTip(tr("Show Avatar"));
        ui.avatarFrameButton->setIcon(QIcon(":images/show_toolbox_frame.png"));
    }
}

void PopupChatDialog::on_closeInfoFrameButton_clicked()
{
    ui.infoframe->setVisible(false);
}

void PopupChatDialog::setColor()
{	    
    bool ok;
    QRgb color = QColorDialog::getRgba(ui.chattextEdit->textColor().rgba(), &ok, this);
    if (ok) {
        mCurrentColor = QColor(color);
        PeerSettings->setPrivateChatColor(dialogId, mCurrentColor.name());
        colorChanged(mCurrentColor);
    }
    setFont();
}

void PopupChatDialog::colorChanged(const QColor &c)
{
    QPixmap pix(16, 16);
    pix.fill(c);
    ui.colorButton->setIcon(pix);
}

void PopupChatDialog::getFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, mCurrentFont, this);
    if (ok) {
        fontChanged(font);
    }
}

void PopupChatDialog::fontChanged(const QFont &font)
{
    mCurrentFont = font;

    ui.textboldButton->setChecked(mCurrentFont.bold());
    ui.textunderlineButton->setChecked(mCurrentFont.underline());
    ui.textitalicButton->setChecked(mCurrentFont.italic());

    setFont();
}

void PopupChatDialog::setFont()
{
    mCurrentFont.setBold(ui.textboldButton->isChecked());
    mCurrentFont.setUnderline(ui.textunderlineButton->isChecked());
    mCurrentFont.setItalic(ui.textitalicButton->isChecked());

    ui.chattextEdit->setFont(mCurrentFont);
    ui.chattextEdit->setTextColor(mCurrentColor);

    ui.chattextEdit->setFocus();

    PeerSettings->setPrivateChatFont(dialogId, mCurrentFont.toString());
}

//============================================================================

void PopupChatDialog::smileyWidget()
{ 
    Emoticons::showSmileyWidget(this, ui.emoteiconButton, SLOT(addSmiley()), true);
}

//============================================================================

void PopupChatDialog::addSmiley()
{
    ui.chattextEdit->textCursor().insertText(qobject_cast<QPushButton*>(sender())->toolTip().split("|").first());
}

//============================================================================

void PopupChatDialog::on_actionClear_Chat_triggered()
{
    ui.textBrowser->clear();
}

void PopupChatDialog::updatePeerAvatar(const std::string& peer_id)
{
        #ifdef CHAT_DEBUG
        std::cerr << "popupchatDialog::updatePeerAvatar() updating avatar for peer " << peer_id << std::endl ;
        std::cerr << "Requesting avatar image for peer " << peer_id << std::endl ;
        #endif

	unsigned char *data = NULL;
	int size = 0 ;

	rsMsgs->getAvatarData(peer_id,data,size); 

        #ifdef CHAT_DEBUG
        std::cerr << "Image size = " << size << std::endl;
        #endif

        if(size == 0) {
           #ifdef CHAT_DEBUG
	   std::cerr << "Got no image" << std::endl ;
           #endif
	   ui.avatarlabel->setPixmap(QPixmap(":/images/no_avatar_background.png"));
		return ;
	}

	// set the image
	QPixmap pix ;
	pix.loadFromData(data,size,"PNG") ;
	ui.avatarlabel->setPixmap(pix); // writes image into ba in JPG format

	delete[] data ;
}

void PopupChatDialog::updateAvatar()
{
	unsigned char *data = NULL;
	int size = 0 ;

	rsMsgs->getOwnAvatarData(data,size); 

        #ifdef CHAT_DEBUG
	std::cerr << "Image size = " << size << std::endl ;
        #endif

        if(size == 0) {
            #ifdef CHAT_DEBUG
            std::cerr << "Got no image" << std::endl ;
            #endif
        }

	// set the image
	QPixmap pix ;
	pix.loadFromData(data,size,"PNG") ;
	ui.myavatarlabel->setPixmap(pix); // writes image into ba in PNGformat

	delete[] data ;
}

void PopupChatDialog::getAvatar()
{
	QString fileName = QFileDialog::getOpenFileName(this, "Load File", QDir::homePath(), "Pictures (*.png *.xpm *.jpg)");

	if(!fileName.isEmpty())
	{
		picture = QPixmap(fileName).scaled(96,96, Qt::IgnoreAspectRatio,Qt::SmoothTransformation);

		std::cerr << "Sending avatar image down the pipe" << std::endl ;

		// send avatar down the pipe for other peers to get it.
		QByteArray ba;
		QBuffer buffer(&ba);
		buffer.open(QIODevice::WriteOnly);
		picture.save(&buffer, "PNG"); // writes image into ba in PNG format

		std::cerr << "Image size = " << ba.size() << std::endl ;

		rsMsgs->setOwnAvatarData((unsigned char *)(ba.data()),ba.size()) ;	// last char 0 included.

		updateAvatar() ;
	}
}

void PopupChatDialog::addExtraFile()
{
	// select a file
	QString qfile = QFileDialog::getOpenFileName(this, tr("Add Extra File"), "", "", 0,
				QFileDialog::DontResolveSymlinks);
	std::string filePath = qfile.toStdString();
	if (filePath != "")
	{
	    PopupChatDialog::addAttachment(filePath,0);
	}
}

void PopupChatDialog::addExtraPicture()
{
	// select a picture file
    QString qfile = QFileDialog::getOpenFileName(this, "Load Picture File", QDir::homePath(), "Pictures (*.png *.xpm *.jpg)",0,
			        QFileDialog::DontResolveSymlinks);
	std::string filePath=qfile.toStdString();
	if(filePath!="")
	{
		PopupChatDialog::addAttachment(filePath,1); //picture
	}
}

void PopupChatDialog::addAttachment(std::string filePath,int flag) 
{
	    /* add a AttachFileItem to the attachment section */
	    std::cerr << "PopupChatDialog::addExtraFile() hashing file.";
	    std::cerr << std::endl;

	    /* add widget in for new destination */
        AttachFileItem *file = new AttachFileItem(filePath);
	    //file->
	    
	    if(flag==1)
	    	file->setPicFlag(1);

	    ui.vboxLayout->addWidget(file, 1, 0);

	    //when the file is local or is finished hashing, call the fileHashingFinished method to send a chat message
	    if (file->getState() == AFI_STATE_LOCAL) {
		fileHashingFinished(file);
	    } else {
		QObject::connect(file,SIGNAL(fileFinished(AttachFileItem *)), SLOT(fileHashingFinished(AttachFileItem *))) ;
	    }
}



void PopupChatDialog::fileHashingFinished(AttachFileItem* file) 
{
    std::cerr << "PopupChatDialog::fileHashingFinished() started.";
    std::cerr << std::endl;

    //check that the file is ok tos end
    if (file->getState() == AFI_STATE_ERROR) {
#ifdef CHAT_DEBUG
        std::cerr << "PopupChatDialog::fileHashingFinished error file is not hashed.";
#endif
        return;
    }

    std::string ownId;

    {
        rsiface->lockData(); /* Lock Interface */
        const RsConfig &conf = rsiface->getConfig();

        ownId = conf.ownId;

        rsiface->unlockData(); /* Unlock Interface */
    }

    QString message;
    QString ext = QFileInfo(QString::fromStdString(file->FileName())).suffix();

    if(file->getPicFlag()==1){
        message+="<img src=\"file:///";
        message+=file->FilePath().c_str();
        message+="\" width=\"100\" height=\"100\">";
        message+="<br>";
    }    
	else if (ext == "ogg" || ext == "mp3" || ext == "MP3"  || ext == "mp1" || ext == "mp2" || ext == "wav" || ext == "wma") 
	{
        message+="<img src=\":/images/audio-x-monkey.png";
        message+="\" width=\"48\" height=\"48\">";
        message+="<br>";
	}
    else if (ext == "avi" || ext == "AVI" || ext == "mpg" || ext == "mpeg" || ext == "wmv" || ext == "ogm"
    || ext == "mkv" || ext == "mp4" || ext == "flv" || ext == "mov"
    || ext == "vob" || ext == "qt" || ext == "rm" || ext == "3gp")
    {
        message+="<img src=\":/images/video-x-generic.png";
        message+="\" width=\"48\" height=\"48\">";
        message+="<br>";
	}
    else if (ext == "tar" || ext == "bz2" || ext == "zip" || ext == "gz" || ext == "7z"
    || ext == "rar" || ext == "rpm" || ext == "deb")
    {
        message+="<img src=\":/images/application-x-rar.png";
        message+="\" width=\"48\" height=\"48\">";
        message+="<br>";
	}
    


    message+= RetroShareLink(QString::fromUtf8(file->FileName().c_str()),file->FileSize(),QString::fromStdString(file->FileHash())).toHtmlSize();

#ifdef CHAT_DEBUG
    std::cerr << "PopupChatDialog::anchorClicked message : " << message.toStdString() << std::endl;
#endif

    std::wstring msg = message.toStdWString();

    if (rsMsgs->sendPrivateChat(dialogId, msg)) {
        QDateTime currentTime = QDateTime::currentDateTime();
        addChatMsg(false, ownId, QString::fromStdString(rsPeers->getPeerName(ownId)), currentTime, currentTime, QString::fromStdWString(msg), TYPE_NORMAL, true);
    }
}

void PopupChatDialog::anchorClicked (const QUrl& link ) 
{
#ifdef CHAT_DEBUG
	std::cerr << "PopupChatDialog::anchorClicked link.scheme() : " << link.scheme().toStdString() << std::endl;
#endif

	std::list<std::string> srcIds;
	RetroShareLink::processUrl(link, RSLINK_PROCESS_NOTIFY_ALL);
}

void PopupChatDialog::dropEvent(QDropEvent *event)
{
    if (!(Qt::CopyAction & event->possibleActions()))
    {
        std::cerr << "PopupChatDialog::dropEvent() Rejecting uncopyable DropAction";
        std::cerr << std::endl;

        /* can't do it */
        return;
    }

    std::cerr << "PopupChatDialog::dropEvent() Formats";
    std::cerr << std::endl;
    QStringList formats = event->mimeData()->formats();
    QStringList::iterator it;
    for(it = formats.begin(); it != formats.end(); it++)
    {
        std::cerr << "Format: " << (*it).toStdString();
        std::cerr << std::endl;
    }

    if (event->mimeData()->hasUrls())
    {
        std::cerr << "PopupChatDialog::dropEvent() Urls:";
        std::cerr << std::endl;

        QList<QUrl> urls = event->mimeData()->urls();
        QList<QUrl>::iterator uit;
        for(uit = urls.begin(); uit != urls.end(); uit++)
        {
            QString localpath = uit->toLocalFile();
            std::cerr << "Whole URL: " << uit->toString().toStdString() << std::endl;
            std::cerr << "or As Local File: " << localpath.toStdString() << std::endl;

            if (localpath.isEmpty() == false)
            {
                //Check that the file does exist and is not a directory
                QDir dir(localpath);
                if (dir.exists()) {
                    std::cerr << "PopupChatDialog::dropEvent() directory not accepted."<< std::endl;
                    QMessageBox mb(tr("Drop file error."), tr("Directory can't be dropped, only files are accepted."),QMessageBox::Information,QMessageBox::Ok,0,0);
                    mb.setButtonText( QMessageBox::Ok, "OK" );
                    mb.exec();
                } else if (QFile::exists(localpath)) {
                    PopupChatDialog::addAttachment(localpath.toUtf8().constData(), false);
                } else {
                    std::cerr << "PopupChatDialog::dropEvent() file does not exists."<< std::endl;
                    QMessageBox mb(tr("Drop file error."), tr("File not found or file name not accepted."),QMessageBox::Information,QMessageBox::Ok,0,0);
                    mb.setButtonText( QMessageBox::Ok, "OK" );
                    mb.exec();
                }
            }
        }
    }

    event->setDropAction(Qt::CopyAction);
    event->accept();
}

void PopupChatDialog::dragEnterEvent(QDragEnterEvent *event)
{
	/* print out mimeType */
	std::cerr << "PopupChatDialog::dragEnterEvent() Formats";
	std::cerr << std::endl;
	QStringList formats = event->mimeData()->formats();
	QStringList::iterator it;
	for(it = formats.begin(); it != formats.end(); it++)
	{
		std::cerr << "Format: " << (*it).toStdString();
		std::cerr << std::endl;
	}

	if (event->mimeData()->hasUrls())
	{
		std::cerr << "PopupChatDialog::dragEnterEvent() Accepting Urls";
		std::cerr << std::endl;
		event->acceptProposedAction();
	}
	else
	{
		std::cerr << "PopupChatDialog::dragEnterEvent() No Urls";
		std::cerr << std::endl;
	}
}

bool PopupChatDialog::fileSave()
{
    if (fileName.isEmpty())
        return fileSaveAs();

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly))
        return false;
    QTextStream ts(&file);
    ts.setCodec(QTextCodec::codecForName("UTF-8"));
    ts << ui.textBrowser->document()->toPlainText();
    ui.textBrowser->document()->setModified(false);
    return true;
}

bool PopupChatDialog::fileSaveAs()
{
    QString fn = QFileDialog::getSaveFileName(this, tr("Save as..."),
                                              QString(), tr("Text File (*.txt );;All Files (*)"));
    if (fn.isEmpty())
        return false;
    setCurrentFileName(fn);
    return fileSave();    
}

void PopupChatDialog::setCurrentFileName(const QString &fileName)
{
    this->fileName = fileName;
    ui.textBrowser->document()->setModified(false);

    setWindowModified(false);
}

void PopupChatDialog::clearOfflineMessages()
{
    m_manualDelete = true;
    rsMsgs->clearPrivateChatQueue(false, dialogId);
    m_manualDelete = false;
}

void PopupChatDialog::updateStatus(const QString &peer_id, int status)
{
    std::string stdPeerId = peer_id.toStdString();
    
    /* set font size for status  */
    if (stdPeerId == dialogId) {
        // the peers status has changed
        switch (status) {
        case RS_STATUS_OFFLINE:
            ui.avatarlabel->setStyleSheet("QLabel#avatarlabel{ border-image:url(:/images/avatarstatus_bg_offline.png); }");
            ui.avatarlabel->setEnabled(false);
            ui.infoframe->setVisible(true);
            ui.infolabel->setText(dialogName + " " + tr("apears to be Offline.") +"\n" + tr("Messages you send will be delivered after Friend is again Online"));
            break;

        case RS_STATUS_INACTIVE:
            ui.avatarlabel->setStyleSheet("QLabel#avatarlabel{ border-image:url(:/images/avatarstatus_bg_away.png); }");
            ui.avatarlabel->setEnabled(true);
            ui.infoframe->setVisible(true);
            ui.infolabel->setText(dialogName + " " + tr("is Idle and may not reply"));
            break;

        case RS_STATUS_ONLINE:
            ui.avatarlabel->setStyleSheet("QLabel#avatarlabel{ border-image:url(:/images/avatarstatus_bg_online.png); }");
            ui.avatarlabel->setEnabled(true);
            ui.infoframe->setVisible(false);
            break;

        case RS_STATUS_AWAY:
            ui.avatarlabel->setStyleSheet("QLabel#avatarlabel{ border-image:url(:/images/avatarstatus_bg_away.png); }");
            ui.avatarlabel->setEnabled(true);
            ui.infolabel->setText(dialogName + " " + tr("is Away and may not reply"));
            ui.infoframe->setVisible(true);
            break;

        case RS_STATUS_BUSY:
            ui.avatarlabel->setStyleSheet("QLabel#avatarlabel{ border-image:url(:/images/avatarstatus_bg_busy.png); }");
            ui.avatarlabel->setEnabled(true);
            ui.infolabel->setText(dialogName + " " + tr("is Busy and may not reply"));
            ui.infoframe->setVisible(true);
            break;
        }

        QString statusString("<span style=\"font-size:11pt; font-weight:500;""\">%1</span>");
        ui.friendnamelabel->setText(dialogName + " (" + statusString.arg(StatusDefs::name(status)) + ")") ;

        return;
    }

    if (stdPeerId == rsPeers->getOwnId()) {
        // my status has changed
        
        switch (status) {
        case RS_STATUS_OFFLINE:
            ui.myavatarlabel->setStyleSheet("QLabel#myavatarlabel{border-image:url(:/images/avatarstatus_bg_offline.png); }");
            break;

        case RS_STATUS_INACTIVE:
            ui.myavatarlabel->setStyleSheet("QLabel#myavatarlabel{border-image:url(:/images/avatarstatus_bg_away.png); }");
            break;

        case RS_STATUS_ONLINE:
            ui.myavatarlabel->setStyleSheet("QLabel#myavatarlabel{border-image:url(:/images/avatarstatus_bg_online.png); }");
            break;

        case RS_STATUS_AWAY:
            ui.myavatarlabel->setStyleSheet("QLabel#myavatarlabel{border-image:url(:/images/avatarstatus_bg_away.png); }");
            break;

        case RS_STATUS_BUSY:
            ui.myavatarlabel->setStyleSheet("QLabel#myavatarlabel{border-image:url(:/images/avatarstatus_bg_busy.png); }");
            break;
        }
        
        return;
    }

    // ignore status change
}

void PopupChatDialog::updatePeersCustomStateString(const QString& peer_id, const QString& status_string)
{
    std::string stdPeerId = peer_id.toStdString();

    if (stdPeerId == dialogId) {
        // the peers status string has changed
        if (status_string.isEmpty()) {
            ui.statusmessagelabel->hide();
        } else {
            ui.statusmessagelabel->show();
            ui.statusmessagelabel->setText(status_string);
        }
    }
}


void PopupChatDialog::on_actionMessageHistory_triggered()
{
    ImHistoryBrowser imBrowser(dialogId, historyKeeper, ui.chattextEdit, this);
    imBrowser.exec();
}
