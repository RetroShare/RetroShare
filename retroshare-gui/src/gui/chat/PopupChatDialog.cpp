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
#include "PopupChatWindow.h"
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

#define WINDOW(This) dynamic_cast<PopupChatWindow*>(This->window())

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
PopupChatDialog::PopupChatDialog(const std::string &id, const QString &name, QWidget *parent, Qt::WFlags flags)
  : QWidget(parent, flags), dialogId(id), dialogName(name),
    lastChatTime(0), lastChatName("")
    
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);

  newMessages = false;
  typing = false;
  m_manualDelete = false;
  peerStatus = 0;

  last_status_send_time = 0 ;
  style.setStyleFromSettings(ChatStyle::TYPE_PRIVATE);

  /* Hide or show the frames */
  showAvatarFrame(true);  
  ui.infoframe->setVisible(false);
  ui.statusmessagelabel->hide();

  connect(ui.avatarFrameButton, SIGNAL(toggled(bool)), this, SLOT(showAvatarFrame(bool)));

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


  connect(NotifyQt::getInstance(), SIGNAL(peerStatusChanged(const QString&, int)), this, SLOT(updateStatus(const QString&, int)));
  connect(NotifyQt::getInstance(), SIGNAL(peerHasNewCustomStateString(const QString&, const QString&)), this, SLOT(updatePeersCustomStateString(const QString&, const QString&)));

  connect(ui.chattextEdit,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(contextMenu(QPoint)));

  // Create the status bar
  resetStatusBar();

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
  QString customStateString = QString::fromUtf8(rsMsgs->getCustomStateString(dialogId).c_str());
  updatePeersCustomStateString(QString::fromStdString(dialogId), customStateString);

  if (Settings->valueFromGroup("Chat", QString::fromUtf8("PrivateChat_History"), true).toBool()) {
      historyKeeper.init(QString::fromStdString(RsInit::RsProfileConfigDirectory()) + "/chat_" + QString::fromStdString(dialogId) +  ".xml");

      // get offline chat messages
      std::list<ChatInfo> offlineChat;
      std::list<ChatInfo>::iterator offineChatIt;
      rsMsgs->getPrivateChatQueueCount(false) && rsMsgs->getPrivateChatQueue(false, dialogId, offlineChat);

      QList<IMHistoryItem> historyItems;
      int messageCount = Settings->getPrivateChatHistoryCount();
      if (messageCount > 0) {
          historyKeeper.getMessages(historyItems, messageCount);
      }
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

#ifdef RS_RELEASE_VERSION
  ui.attachPictureButton->setVisible(false);
#endif
}

/** Destructor. */
PopupChatDialog::~PopupChatDialog()
{
    // save settings
    processSettings(false);

    PopupChatWindow *window = WINDOW(this);
    if (window) {
        window->removeDialog(this);
        window->calculateTitle(NULL);
    }

    std::map<std::string, PopupChatDialog *>::iterator it;
    if (chatDialogs.end() != (it = chatDialogs.find(dialogId))) {
        chatDialogs.erase(it);
    }
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

/*static*/ PopupChatDialog *PopupChatDialog::getExistingInstance(const std::string &id)
{
    std::map<std::string, PopupChatDialog *>::iterator it;
    if (chatDialogs.end() != (it = chatDialogs.find(id))) {
        /* exists already */
        return it->second;
    }

    return NULL;
}

/*static*/ PopupChatDialog *PopupChatDialog::getPrivateChat(const std::string &id, uint chatflags)
{
    /* see if it exists already */
    PopupChatDialog *popupchatdialog = getExistingInstance(id);
    if (popupchatdialog == NULL) {
        if (chatflags & RS_CHAT_OPEN) {
            RsPeerDetails sslDetails;
            if (rsPeers->getPeerDetails(id, sslDetails)) {
                popupchatdialog = new PopupChatDialog(id, PeerDefs::nameWithLocation(sslDetails));
                chatDialogs[id] = popupchatdialog;

                PopupChatWindow *window = PopupChatWindow::getWindow(false);
                window->addDialog(popupchatdialog);
            }
        }
    }

    if (popupchatdialog == NULL) {
        return NULL;
    }

    popupchatdialog->insertChatMsgs();

    PopupChatWindow *window = WINDOW(popupchatdialog);
    if (window) {
        window->showDialog(popupchatdialog, chatflags);
    }

    return popupchatdialog;
}

/*static*/ void PopupChatDialog::cleanupChat()
{
    PopupChatWindow::cleanup();

    /* PopupChatDialog destuctor removes the entry from the map */
    std::list<PopupChatDialog*> list;

    std::map<std::string, PopupChatDialog*>::iterator it;
    for (it = chatDialogs.begin(); it != chatDialogs.end(); it++) {
        if (it->second) {
            list.push_back(it->second);
        }
    }

    chatDialogs.clear();

    std::list<PopupChatDialog*>::iterator it1;
    for (it1 = list.begin(); it1 != list.end(); it1++) {
        delete (*it1);
    }
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

void PopupChatDialog::chatFriend(const std::string &id)
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
                    getPrivateChat(*it, RS_CHAT_OPEN | RS_CHAT_FOCUS);
                    return;
                }
            }
        }
    } else {
        if (detail.state & RS_PEER_STATE_CONNECTED) {
            getPrivateChat(id, RS_CHAT_OPEN | RS_CHAT_FOCUS);
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
            getPrivateChat(firstId, RS_CHAT_OPEN | RS_CHAT_FOCUS);
        }
    }
}

/*static*/ void PopupChatDialog::updateAllAvatars()
{
    for(std::map<std::string, PopupChatDialog *>::const_iterator it(chatDialogs.begin());it!=chatDialogs.end();++it)
        it->second->updateAvatar() ;
}

void PopupChatDialog::focusDialog()
{
    ui.chattextEdit->setFocus();
}

void PopupChatDialog::pasteLink()
{
	std::cerr << "In paste link" << std::endl ;
	ui.chattextEdit->insertHtml(RSLinkClipboard::toHtml()) ;
}

void PopupChatDialog::contextMenu( QPoint point )
{
    std::cerr << "In context menu" << std::endl ;

    QMenu *contextMnu = ui.chattextEdit->createStandardContextMenu();

    contextMnu->addSeparator();
    QAction *action = contextMnu->addAction(QIcon(":/images/pasterslink.png"), tr("Paste RetroShare Link"), this, SLOT(pasteLink()));
    action->setDisabled(RSLinkClipboard::empty());

    contextMnu->exec(QCursor::pos());
    delete(contextMnu);
}

void PopupChatDialog::resetStatusBar() 
{
    ui.statusLabel->clear();
    ui.typingpixmapLabel->clear();

    typing = false;

    PopupChatWindow *window = WINDOW(this);
    if (window) {
        window->calculateTitle(this);
    }
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
    ui.statusLabel->setText(status); // displays info for 5 secs.
    ui.typingpixmapLabel->setPixmap(QPixmap(":images/typing.png") );

    if (status_string == "is typing...") {
        typing = true;

        PopupChatWindow *window = WINDOW(this);
        if (window) {
            window->calculateTitle(this);
        }
    }

    QTimer::singleShot(5000,this,SLOT(resetStatusBar())) ;
}

void PopupChatDialog::resizeEvent(QResizeEvent *event)
{
    // Workaround: now the scroll position is correct calculated
    QScrollBar *scrollbar = ui.textBrowser->verticalScrollBar();
    scrollbar->setValue(scrollbar->maximum());
}

void PopupChatDialog::activate()
{
    PopupChatWindow *window = WINDOW(this);
    if (window) {
        if (window->isActiveWindow()) {
            newMessages = false;
            window->calculateTitle(this);
            focusDialog();
        }
    } else {
        newMessages = false;
    }
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

    PopupChatWindow *window = WINDOW(this);
    if (window) {
        window->alertDialog(this);
    }

    if (isVisible() == false || (window && window->isActiveWindow() == false)) {
        newMessages = true;

        if (window) {
            window->calculateTitle(this);
        }
    }
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

    /* Scroll to the end */
    QScrollBar *scrollbar = ui.textBrowser->verticalScrollBar();
    scrollbar->setValue(scrollbar->maximum());

    resetStatusBar();
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
    return QWidget::eventFilter(obj, event);
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

void PopupChatDialog::addExtraFile()
{
	// select a file
	QString qfile = QFileDialog::getOpenFileName(this, tr("Add Extra File"), "", "", 0,
				QFileDialog::DontResolveSymlinks);
	std::string filePath = qfile.toStdString();
	if (filePath != "")
	{
		addAttachment(filePath,0);
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
	else if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "bmp" || ext == "ico" 
	|| ext == "svg" || ext == "tif" || ext == "tiff" || ext == "JPG")
	{
        message+="<img src=\":/images/application-draw.png";
        message+="\" width=\"48\" height=\"48\">";
        message+="<br>";
	}
    


    message += RetroShareLink(QString::fromUtf8(file->FileName().c_str()),file->FileSize(),QString::fromStdString(file->FileHash())).toHtmlSize();

#ifdef CHAT_DEBUG
    std::cerr << "PopupChatDialog::fileHashingFinished message : " << message.toStdString() << std::endl;
#endif

    /* convert to real html document */
    QTextBrowser textBrowser;
    textBrowser.setHtml(message);
    std::wstring msg = textBrowser.toHtml().toStdWString();

    if (rsMsgs->sendPrivateChat(dialogId, msg)) {
        QDateTime currentTime = QDateTime::currentDateTime();
        addChatMsg(false, ownId, QString::fromStdString(rsPeers->getPeerName(ownId)), currentTime, currentTime, QString::fromStdWString(msg), TYPE_NORMAL, true);
    }
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
                    QMessageBox mb(tr("Drop file error."), tr("Directory can't be dropped, only files are accepted."),QMessageBox::Information,QMessageBox::Ok,0,0,this);
                    mb.exec();
                } else if (QFile::exists(localpath)) {
                    PopupChatDialog::addAttachment(localpath.toUtf8().constData(), false);
                } else {
                    std::cerr << "PopupChatDialog::dropEvent() file does not exists."<< std::endl;
                    QMessageBox mb(tr("Drop file error."), tr("File not found or file name not accepted."),QMessageBox::Information,QMessageBox::Ok,0,0,this);
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

        peerStatus = status;

        PopupChatWindow *window = WINDOW(this);
        if (window) {
            window->calculateTitle(this);
        }

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
