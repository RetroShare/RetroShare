/*******************************************************************************
 * gui/chat/ChatLobbyUserNotify.cpp                                            *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) 2014 Retroshare Team <retroshare.project@gmail.com>           *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include <QTime>
#include <QMenu>

#include "gui/common/FilesDefs.h"
#include "ChatLobbyUserNotify.h"

#include "gui/ChatLobbyWidget.h"
#include "gui/MainWindow.h"
#include "gui/notifyqt.h"
#include "gui/SoundManager.h"
#include "gui/settings/rsharesettings.h"
#include "util/DateTime.h"
#include <util/HandleRichText.h>

#include <retroshare/rsidentity.h>

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
    return FilesDefs::getIconFromQtResourcePath(":/icons/png/chat-lobbies.png");
}

QIcon ChatLobbyUserNotify::getMainIcon(bool hasNew)
{
    return hasNew ? FilesDefs::getIconFromQtResourcePath(":/icons/png/chat-lobbies-notify.png") : FilesDefs::getIconFromQtResourcePath(":/icons/png/chat-lobbies.png");
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
	return iNum;
}

QString ChatLobbyUserNotify::getTrayMessage(bool plural)
{
	return plural ? tr("You have %1 mentions") : tr("You have %1 mention");
}

QString ChatLobbyUserNotify::getNotifyMessage(bool plural)
{
	return plural ? tr("%1 mentions") : tr("%1 mention");
}

void ChatLobbyUserNotify::iconClicked()
{
    #if defined(Q_OS_DARWIN)
    std::list<ChatLobbyId> lobbies;
    rsMsgs->getChatLobbyList(lobbies);
    bool doUpdate=false;

    for (lobby_map::iterator itCL=_listMsg.begin(); itCL!=_listMsg.end();)
    {
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
                bFound=true;
                break;
            }
        }

        if (bFound)
        {
            MainWindow::showWindow(MainWindow::ChatLobby);
            ChatLobbyWidget *chatLobbyWidget = dynamic_cast<ChatLobbyWidget*>(MainWindow::getPage(MainWindow::ChatLobby));
            if (chatLobbyWidget) chatLobbyWidget->showLobbyAnchor(itCL->first,strLobbyName);
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
    #else

	/// Tray icon Menu ///
	QMenu* trayMenu = new QMenu(MainWindow::getInstance());
	std::list<ChatLobbyId> lobbies;
	rsMsgs->getChatLobbyList(lobbies);
	bool doUpdate=false;

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
                icoLobby=(clInfo.lobby_flags & RS_CHAT_LOBBY_FLAGS_PUBLIC) ? FilesDefs::getIconFromQtResourcePath(":/images/chat_red24.png") : FilesDefs::getIconFromQtResourcePath(":/images/chat_x24.png");
                bFound=true;
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

	if (notifyCombined()) {
		QSystemTrayIcon* trayIcon=getTrayIcon();
		if (trayIcon!=NULL) trayIcon->setContextMenu(trayMenu);
	} else {
		QAction* action=getNotifyIcon();
		if (action!=NULL) {
			action->setMenu(trayMenu);
		}
	}

	QString strName=tr("Remove All");
	QAction *pAction = new QAction( QIcon(), strName, trayMenu);
	ActionTag actionTag={0x0, "", true};
	pAction->setData(qVariantFromValue(actionTag));
	connect(trayMenu, SIGNAL(triggered(QAction*)), this, SLOT(subMenuClicked(QAction*)));
	connect(trayMenu, SIGNAL(hovered(QAction*)), this, SLOT(subMenuHovered(QAction*)));
	trayMenu->addAction(pAction);

	trayMenu->exec(QCursor::pos());
	if (doUpdate) updateIcon();

	#endif
}

void ChatLobbyUserNotify::makeSubMenu(QMenu* parentMenu, QIcon icoLobby, QString strLobbyName, ChatLobbyId id)
{
	lobby_map::iterator itCL=_listMsg.find(id);
	if (itCL==_listMsg.end()) return;
	msg_map msgMap = itCL->second;

	unsigned int msgCount=msgMap.size();

	if(!parentMenu) parentMenu = new QMenu(MainWindow::getInstance());
	QMenu *lobbyMenu = parentMenu->addMenu(icoLobby, strLobbyName);
	connect(lobbyMenu, SIGNAL(triggered(QAction*)), this, SLOT(subMenuClicked(QAction*)));
	connect(lobbyMenu, SIGNAL(hovered(QAction*)), this, SLOT(subMenuHovered(QAction*)));

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

	QString strName=tr("Remove All");
	QAction *pAction = new QAction( icoLobby, strName, lobbyMenu);
	ActionTag actionTag={itCL->first, "", true};
	pAction->setData(qVariantFromValue(actionTag));
	lobbyMenu->addAction(pAction);

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

bool ChatLobbyUserNotify::checkWord(QString message, QString word)
{
	bool bFound = false;
	int nFound = -1;
	if (((nFound=message.indexOf(word,0,_bTextCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive)) != -1)
	    && (!word.isEmpty())) {
		QString eow=" ~!@#$%^&*()_+{}|:\"<>?,./;'[]\\-="; // end of word
		bool bFirstCharEOW = (nFound==0)?true:(eow.indexOf(message.at(nFound-1)) != -1);
		bool bLastCharEOW = (nFound+word.length() < message.length())
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

void ChatLobbyUserNotify::subMenuHovered(QAction* action)
{
	QMenu *lobbyMenu=dynamic_cast<QMenu*>(action->parent());
	if (lobbyMenu) lobbyMenu->setToolTip(action->toolTip());
}
