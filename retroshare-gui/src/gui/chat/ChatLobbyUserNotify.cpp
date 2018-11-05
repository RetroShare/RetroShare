/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2014 RetroShare Team
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

#include <QTime>
#include <QMenu>

#include "ChatLobbyUserNotify.h"

#include "gui/ChatLobbyWidget.h"
#include "gui/MainWindow.h"
#include "gui/notifyqt.h"
#include "gui/SoundManager.h"
#include "gui/settings/rsharesettings.h"
#include "util/DateTime.h"
#include <util/HandleRichText.h>

#include <retroshare/rsidentity.h>
#include "retroshare/rspeers.h"
#include "gui/common/AvatarDefs.h"

ChatLobbyUserNotify::ChatLobbyUserNotify(QObject *parent) :
    UserNotify(parent)
{
	_name = tr("Chats");
	_group = "ChatLobby";

	_bCheckForNickName = Settings->valueFromGroup(_group, "CheckForNickName", true).toBool();
	_bCountUnRead = Settings->valueFromGroup(_group, "CountUnRead", true).toBool();
	_bCountSpecificText = Settings->valueFromGroup(_group, "CountSpecificText", false).toBool();
	_textToNotify = Settings->valueFromGroup(_group, "TextToNotify").toStringList();
	_bTextCaseSensitive = Settings->valueFromGroup(_group, "TextCaseSensitive", false).toBool();
}

bool ChatLobbyUserNotify::hasSetting(QString *name, QString *group)
{
	if (name) *name = _name;
	if (group) *group = _group;

	return true;
}

void ChatLobbyUserNotify::setCheckForNickName(bool value)
{
	if (_bCheckForNickName != value) {
		_bCheckForNickName = value;
		Settings->setValueToGroup(_group, "CheckForNickName", value);
	}
}

void ChatLobbyUserNotify::setCountUnRead(bool value)
{
	if (_bCountUnRead != value) {
		_bCountUnRead = value;
		Settings->setValueToGroup(_group, "CountUnRead", value);
	}
}

void ChatLobbyUserNotify::setCountSpecificText(bool value)
{
    if (_bCountSpecificText != value) {
        _bCountSpecificText = value;
        Settings->setValueToGroup(_group, "CountSpecificText", value);
    }
}
void ChatLobbyUserNotify::setTextToNotify(QStringList value)
{
	if (_textToNotify != value) {
		_textToNotify = value;
		Settings->setValueToGroup(_group, "TextToNotify", value);
	}
}

void ChatLobbyUserNotify::setTextToNotify(QString value)
{
	while(value.contains("\n\n")) value.replace("\n\n","\n");
	QStringList list = value.split("\n");
	setTextToNotify(list);
}

void ChatLobbyUserNotify::setTextCaseSensitive(bool value)
{
	if (_bTextCaseSensitive != value) {
		_bTextCaseSensitive = value;
		Settings->setValueToGroup(_group, "TextCaseSensitive", value);
	}
}

QIcon ChatLobbyUserNotify::getIcon()
{
    return QIcon(":/home/img/face_icon/un_chat_icon_128.png");
}

QIcon ChatLobbyUserNotify::getMainIcon(bool hasNew)
{
    return hasNew ? QIcon(":/home/img/face_icon/un_chat_icon_v_128.png") : QIcon(":/home/img/face_icon/un_chat_icon_128.png");
}

unsigned int ChatLobbyUserNotify::getNewCount()
{
	int iNum=0;
    for (lobby_map::iterator itCL=_listMsg.begin(); itCL!=_listMsg.end();)
    {
        iNum+=itCL->second.size();

        if (itCL->second.size()==0)
        {
            lobby_map::iterator ittmp = itCL ;
            ++ittmp ;

            _listMsg.erase(itCL);
            itCL = ittmp ;
        }
        else
            ++itCL ;
	}

    /* meiyousixin - count more for p2p chat*/
    for (p2pchat_map::iterator itCL=_listP2PMsg.begin(); itCL!=_listP2PMsg.end();)
    {
        iNum+=itCL->second.size();

        if (itCL->second.size()==0)
        {
            p2pchat_map::iterator ittmp = itCL ;
            ++ittmp ;

            _listP2PMsg.erase(itCL);
            itCL = ittmp ;
        }
        else
            ++itCL ;
    }

	return iNum;
}

QString ChatLobbyUserNotify::getTrayMessage(bool plural)
{
	return plural ? tr("You have %1 new messages") : tr("You have %1 new message");
}

QString ChatLobbyUserNotify::getNotifyMessage(bool plural)
{
	return plural ? tr("%1 new messages") : tr("%1 new message");
}

void ChatLobbyUserNotify::iconClicked()
{
	/// Tray icon Menu ///
	QMenu* trayMenu = new QMenu(MainWindow::getInstance());
	std::list<ChatLobbyId> lobbies;
	rsMsgs->getChatLobbyList(lobbies);
	bool doUpdate=false;
    bool bFoundForLobby = false;
    for (lobby_map::iterator itCL=_listMsg.begin(); itCL!=_listMsg.end();)
    {
        /// Create a menu per lobby ///
        bool bFound=false;
        QString strLobbyName=tr("Unknown Lobby");
        QIcon icoLobby=QIcon();
        std::list<ChatLobbyId>::const_iterator lobbyIt;
        for (lobbyIt = lobbies.begin(); lobbyIt != lobbies.end(); ++lobbyIt) {
            ChatLobbyId clId = *lobbyIt;
            if (clId==itCL->first) {
                ChatLobbyInfo clInfo;
                if (rsMsgs->getChatLobbyInfo(clId,clInfo))
                    strLobbyName=QString::fromUtf8(clInfo.lobby_name.c_str()) ;
                icoLobby=(clInfo.lobby_flags & RS_CHAT_LOBBY_FLAGS_PUBLIC) ? QIcon(":/images/chat_red24.png") : QIcon(":/images/chat_x24.png");
                bFound=true;
                bFoundForLobby = bFound;
                break;
            }
        }

        if (bFound)
        {
            makeSubMenu(trayMenu, icoLobby, strLobbyName, itCL->first);
            ++itCL ;
        }
        else
        {
            lobby_map::iterator ittmp(itCL);
            ++ittmp ;
            _listMsg.erase(itCL);
            itCL=ittmp ;
            doUpdate=true;
        }
    }


    /* meiyousixin - for p2p chat */
    bool bFoundForContact = false;
    std::list<RsPeerId> ids;
    if (rsPeers->getFriendList(ids))
    {
        for (p2pchat_map::iterator itCL=_listP2PMsg.begin(); itCL!=_listP2PMsg.end();)
        {
            /// Create a menu per lobby ///
            bool bFound=false;
            QString strContactName=tr("Unknown contact");
            QIcon iconContact=QIcon();
            std::list<RsPeerId>::const_iterator peerIt;
            for (peerIt = ids.begin(); peerIt != ids.end(); ++peerIt) {
                RsPeerId clId = *peerIt;
                if (clId==(itCL->first).toPeerId()) {
                    RsPgpId pgpId = rsPeers->getGPGId(clId);
                    std::string nickname = rsPeers->getGPGName(pgpId);
                    strContactName = QString::fromStdString(nickname);

                    QPixmap avatar;
                    AvatarDefs::getAvatarFromSslId(clId, avatar);
                    if (!avatar.isNull())
                        iconContact = QIcon(avatar) ;

                    bFoundForContact = bFound=true;
                    break;
                }
            }

            if (bFound)
            {
                makeSubMenuForP2P(trayMenu, iconContact, strContactName, itCL->first);
                ++itCL ;
            }
            else
            {
                p2pchat_map::iterator ittmp(itCL);
                ++ittmp ;
                _listP2PMsg.erase(itCL);
                itCL=ittmp ;
                doUpdate=true;
            }
        }
    }


#ifdef WINDOWS_SYS
	if (notifyCombined()) {
		QSystemTrayIcon* trayIcon=getTrayIcon();
		if (trayIcon!=NULL) trayIcon->setContextMenu(trayMenu);
	} else {
		QAction* action=getNotifyIcon();
		if (action!=NULL) {
			action->setMenu(trayMenu);
		}
	}

    if (bFoundForLobby)
    {

        QString strName=tr("Remove all lobby unread chat");
        QAction *pAction = new QAction( QIcon(), strName, trayMenu);
        ActionTag actionTag={0x0, "", true};
        pAction->setData(qVariantFromValue(actionTag));
        connect(trayMenu, SIGNAL(triggered(QAction*)), this, SLOT(subMenuClicked(QAction*)));
        connect(trayMenu, SIGNAL(hovered(QAction*)), this, SLOT(subMenuHovered(QAction*)));
        trayMenu->addAction(pAction);

        trayMenu->exec(QCursor::pos());

    }

    if (bFoundForContact)
    {
        QString strName=tr("Remove all contacts unread chat");
        QAction *pAction = new QAction( QIcon(), strName, trayMenu);
        ActionTag2 actionTag={RsPeerId(), "", true};
        pAction->setData(qVariantFromValue(actionTag));
        connect(trayMenu, SIGNAL(triggered(QAction*)), this, SLOT(subMenuClickedP2P(QAction*)));
        //connect(trayMenu, SIGNAL(hovered(QAction*)), this, SLOT(subMenuHovered(QAction*)));
        trayMenu->addAction(pAction);

        trayMenu->exec(QCursor::pos());
    }
#endif
    if (doUpdate) updateIcon();

}

void ChatLobbyUserNotify::makeSubMenu(QMenu* parentMenu, QIcon icoLobby, QString strLobbyName, ChatLobbyId id)
{
	lobby_map::iterator itCL=_listMsg.find(id);
	if (itCL==_listMsg.end()) return;
	msg_map msgMap = itCL->second;

	unsigned int msgCount=msgMap.size();

	if(!parentMenu) parentMenu = new QMenu(MainWindow::getInstance());
	QMenu *lobbyMenu = parentMenu->addMenu(icoLobby, strLobbyName);
#ifdef WINDOWS_SYS
    connect(lobbyMenu, SIGNAL(triggered(QAction*)), this, SLOT(subMenuClicked(QAction*)));
    connect(lobbyMenu, SIGNAL(hovered(QAction*)), this, SLOT(subMenuHovered(QAction*)));
#endif
	lobbyMenu->setToolTip(getNotifyMessage(msgCount>1).arg(msgCount));

	for (msg_map::iterator itMsg=msgMap.begin(); itMsg!=msgMap.end(); ++itMsg) {
		/// initialize menu ///
		QString strName=itMsg->first;
		MsgData msgData=itMsg->second;
		QTextDocument doc;
		doc.setHtml(msgData.text);
		strName.append(":").append(doc.toPlainText().left(30).replace(QString("\n"),QString(" ")));
		QAction *pAction = new QAction( icoLobby, strName, lobbyMenu);
		pAction->setToolTip(doc.toPlainText());
		ActionTag actionTag={itCL->first, itMsg->first, false};
		pAction->setData(qVariantFromValue(actionTag));
		lobbyMenu->addAction(pAction);
	}

//	QString strName=tr("Remove All");
//	QAction *pAction = new QAction( icoLobby, strName, lobbyMenu);
//	ActionTag actionTag={itCL->first, "", true};
//	pAction->setData(qVariantFromValue(actionTag));
//	lobbyMenu->addAction(pAction);

}


void ChatLobbyUserNotify::makeSubMenuForP2P(QMenu* parentMenu, QIcon iconContact, QString strContactName, ChatId id)
{
    p2pchat_map::iterator itCL=_listP2PMsg.find(id);
    if (itCL==_listP2PMsg.end()) return;
    msg_map msgMap = itCL->second;

    unsigned int msgCount=msgMap.size();

    if(!parentMenu) parentMenu = new QMenu(MainWindow::getInstance());
    QMenu *contactMenu = parentMenu->addMenu(iconContact, strContactName);
#ifdef WINDOWS_SYS
    connect(contactMenu, SIGNAL(triggered(QAction*)), this, SLOT(subMenuClickedP2P(QAction*)));
    //connect(contactMenu, SIGNAL(hovered(QAction*)), this, SLOT(subMenuHovered(QAction*)));
#endif
    contactMenu->setToolTip(getNotifyMessage(msgCount>1).arg(msgCount));

    for (msg_map::iterator itMsg=msgMap.begin(); itMsg!=msgMap.end(); ++itMsg) {
        /// initialize menu ///
        QString strName=itMsg->first;
        MsgData msgData=itMsg->second;
        QTextDocument doc;
        doc.setHtml(msgData.text);
        strName.append(":").append(doc.toPlainText().left(30).replace(QString("\n"),QString(" ")));
        QAction *pAction = new QAction( iconContact, strName, contactMenu);
        pAction->setToolTip(doc.toPlainText());
        ActionTag2 actionTag={(itCL->first).toPeerId(), itMsg->first, false};
        pAction->setData(qVariantFromValue(actionTag));
        contactMenu->addAction(pAction);
    }

//    QString strName=tr("Remove All");
//    QAction *pAction = new QAction( iconContact, strName, contactMenu);
//    ActionTag2 actionTag={(itCL->first).toPeerId(), "", true};
//    pAction->setData(qVariantFromValue(actionTag));
//    contactMenu->addAction(pAction);

}


void ChatLobbyUserNotify::iconHovered()
{
	iconClicked();
}


void ChatLobbyUserNotify::chatLobbyNewMessage(ChatLobbyId lobby_id, QDateTime time, QString senderName, QString msg)
{

	bool bGetNickName = false;
	if (_bCheckForNickName) {
		RsGxsId gxs_id;
		rsMsgs->getIdentityForChatLobby(lobby_id,gxs_id);
		RsIdentityDetails details ;
		rsIdentity->getIdDetails(gxs_id,details) ;
		bGetNickName = checkWord(msg, QString::fromUtf8(details.mNickname.c_str()));
	}

	bool bFoundTextToNotify = false;

	if(_bCountSpecificText)
		for (QStringList::Iterator it = _textToNotify.begin(); it != _textToNotify.end(); ++it) {
			bFoundTextToNotify |= checkWord(msg, (*it));
		}

	if ((bGetNickName || bFoundTextToNotify || _bCountUnRead)){
		QString strAnchor = time.toString(Qt::ISODate);
		MsgData msgData;
		msgData.text=RsHtml::plainText(senderName) + ": " + msg;
		msgData.unread=!(bGetNickName || bFoundTextToNotify);

		_listMsg[lobby_id][strAnchor]=msgData;
		emit countChanged(lobby_id, _listMsg[lobby_id].size());
		updateIcon();
		SoundManager::play(SOUND_NEW_LOBBY_MESSAGE);
	}
}

void ChatLobbyUserNotify::chatP2PNewMessage(ChatId chatId, QDateTime time, QString senderName, QString msg)
{

    if ( _bCountUnRead)
    {
        QString strAnchor = time.toString(Qt::ISODate);
        MsgData msgData;
        //msgData.text=RsHtml::plainText(senderName) + ": " + msg;
        msgData.text= ": " + msg;
        //msgData.unread=!(bGetNickName || bFoundTextToNotify);

        _listP2PMsg[chatId][strAnchor]=msgData;
        emit countChangedFromP2P(chatId, _listP2PMsg[chatId].size());
        updateIcon();
        SoundManager::play(SOUND_NEW_CHAT_MESSAGE);
    }

}

bool ChatLobbyUserNotify::checkWord(QString message, QString word)
{
	bool bFound = false;
	int nFound = -1;
	if (((nFound=message.indexOf(word,0,_bTextCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive)) != -1)
	    && (!word.isEmpty())) {
		QString eow=" ~!@#$%^&*()_+{}|:\"<>?,./;'[]\\-="; // end of word
		bool bFirstCharEOW = (nFound==0)?true:(eow.indexOf(message.at(nFound-1)) != -1);
		bool bLastCharEOW = ((nFound+word.length()-1) < message.length())
		    ?(eow.indexOf(message.at(nFound+word.length())) != -1)
		   :true;
		bFound = (bFirstCharEOW && bLastCharEOW);
	}
	return bFound;
}

void ChatLobbyUserNotify::chatLobbyCleared(ChatLobbyId lobby_id, QString anchor, bool onlyUnread /*=false*/)
{
	bool changed = anchor.isEmpty();
	unsigned int count=0;
	if (lobby_id==0) return;
    lobby_map::iterator itCL=_listMsg.find(lobby_id);
	if (itCL!=_listMsg.end()) {
		if (!anchor.isEmpty()) {
			msg_map::iterator itMsg=itCL->second.find(anchor);
			if (itMsg!=itCL->second.end()) {
				MsgData msgData = itMsg->second;
				if(!onlyUnread || msgData.unread) {
					itCL->second.erase(itMsg);
					changed=true;
				}
			}
			count = itCL->second.size();
		}
		if (count==0) _listMsg.erase(itCL);
	}
	if (changed) emit countChanged(lobby_id, count);
	updateIcon();
}

void ChatLobbyUserNotify::chatP2PCleared(ChatId chatId, QString anchor, bool onlyUnread /*=false*/)
{
    bool changed = anchor.isEmpty();
    unsigned int count=0;
    //if (chatId==NULL) return;
    p2pchat_map::iterator itCL=_listP2PMsg.find(chatId);
    if (itCL!=_listP2PMsg.end()) {
        if (!anchor.isEmpty()) {
            msg_map::iterator itMsg=itCL->second.find(anchor);
            if (itMsg!=itCL->second.end()) {
                MsgData msgData = itMsg->second;
                if(!onlyUnread || msgData.unread) {
                    itCL->second.erase(itMsg);
                    changed=true;
                }
            }
            count = itCL->second.size();
        }
        if (count==0) _listP2PMsg.erase(itCL);
    }
    if (changed) emit countChangedFromP2P(chatId, count);
    updateIcon();
}
#ifdef WINDOWS_SYS
void ChatLobbyUserNotify::subMenuClicked(QAction* action)
{
	ActionTag actionTag=action->data().value<ActionTag>();
	if(!actionTag.removeALL){
		MainWindow::showWindow(MainWindow::ChatLobby);
		ChatLobbyWidget *chatLobbyWidget = dynamic_cast<ChatLobbyWidget*>(MainWindow::getPage(MainWindow::ChatLobby));
		if (chatLobbyWidget) chatLobbyWidget->showLobbyAnchor(actionTag.cli ,actionTag.timeStamp);
	}

	lobby_map::iterator itCL=_listMsg.find(actionTag.cli);
	if (itCL!=_listMsg.end()) {
		unsigned int count=0;
		if(!actionTag.removeALL){
			msg_map::iterator itMsg=itCL->second.find(actionTag.timeStamp);
			if (itMsg!=itCL->second.end()) itCL->second.erase(itMsg);
			count = itCL->second.size();
		}
		if (count==0) _listMsg.erase(itCL);
		emit countChanged(actionTag.cli, count);
	} else if(actionTag.cli==0x0){
		while (!_listMsg.empty()) {
			itCL = _listMsg.begin();
			emit countChanged(itCL->first, 0);
			_listMsg.erase(itCL);
		}
	}
	QMenu *lobbyMenu=dynamic_cast<QMenu*>(action->parent());
	if (lobbyMenu) lobbyMenu->removeAction(action);

	updateIcon();
}

#endif
#ifdef WINDOWS_SYS
void ChatLobbyUserNotify::subMenuClickedP2P(QAction* action)
{
    ActionTag2 actionTag=action->data().value<ActionTag2>();
    if(!actionTag.removeALL){
        MainWindow::showWindow(MainWindow::ChatLobby);
        ChatLobbyWidget *chatLobbyWidget = dynamic_cast<ChatLobbyWidget*>(MainWindow::getPage(MainWindow::ChatLobby));
        //if (chatLobbyWidget) chatLobbyWidget->showLobbyAnchor(actionTag.cli ,actionTag.timeStamp);
        if (chatLobbyWidget) chatLobbyWidget->showContactAnchor(actionTag.cli ,actionTag.timeStamp);
    }

    p2pchat_map::iterator itCL=_listP2PMsg.find(ChatId(actionTag.cli));
    if (itCL!=_listP2PMsg.end())
    {
        unsigned int count=0;
        if(!actionTag.removeALL){
            msg_map::iterator itMsg=itCL->second.find(actionTag.timeStamp);
            if (itMsg!=itCL->second.end()) itCL->second.erase(itMsg);
            count = itCL->second.size();
        }
        if (count==0) _listP2PMsg.erase(itCL);
        emit countChangedFromP2P(ChatId(actionTag.cli), count);
    }
    else if(actionTag.cli == RsPeerId())
    {
        while (!_listP2PMsg.empty())
        {
            itCL = _listP2PMsg.begin();
            emit countChangedFromP2P(itCL->first, 0);
            _listP2PMsg.erase(itCL);
        }
    }
    QMenu *lobbyMenu=dynamic_cast<QMenu*>(action->parent());
    if (lobbyMenu) lobbyMenu->removeAction(action);

    updateIcon();
}
#endif

#ifdef WINDOWS_SYS
void ChatLobbyUserNotify::subMenuHovered(QAction* action)
{
	QMenu *lobbyMenu=dynamic_cast<QMenu*>(action->parent());
	if (lobbyMenu) lobbyMenu->setToolTip(action->toolTip());
}
#endif
