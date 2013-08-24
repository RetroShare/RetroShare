/****************************************************************
 *
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2011, csoler  
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
#include <QInputDialog>
#include <QMenu>

#include "ChatLobbyDialog.h"
#include "gui/ChatLobbyWidget.h"
#include "ChatTabWidget.h"
#include "gui/settings/rsharesettings.h"
#include "gui/settings/RsharePeerSettings.h"
#include "gui/MainWindow.h"
#include "gui/FriendsDialog.h"
#include <gui/common/html.h>
#include "gui/common/RSTreeWidgetItem.h"
#include "gui/common/FriendSelectionDialog.h"
#include "util/HandleRichText.h"

#include <retroshare/rsnotify.h>

#include <time.h>

#define COLUMN_ICON  0
#define COLUMN_NAME  1
#define COLUMN_ACTIVITY  2
#define COLUMN_COUNT     3

/** Default constructor */
ChatLobbyDialog::ChatLobbyDialog(const ChatLobbyId& lid, QWidget *parent, Qt::WFlags flags)
	: ChatDialog(parent, flags), lobbyId(lid)
{
	/* Invoke Qt Designer generated QObject setup routine */
	ui.setupUi(this);

	connect(ui.participantsFrameButton, SIGNAL(toggled(bool)), this, SLOT(showParticipantsFrame(bool)));
	connect(ui.actionChangeNickname, SIGNAL(triggered()), this, SLOT(changeNickname()));
	connect(ui.participantsList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(participantsTreeWidgetCustomPopupMenu(QPoint)));
	connect(ui.participantsList, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(participantsTreeWidgetDoubleClicked(QTreeWidgetItem*,int)));

	ui.participantsList->setColumnCount(COLUMN_COUNT);
	ui.participantsList->setColumnWidth(COLUMN_ICON, 20);
    ui.participantsList->setColumnHidden(COLUMN_ACTIVITY,true);

	muteAct = new QAction(QIcon(), tr("Mute participant"), this);
	connect(muteAct, SIGNAL(triggered()), this, SLOT(changePartipationState()));

	// Add a button to invite friends.
	//
	inviteFriendsButton = new QPushButton ;
	inviteFriendsButton->setMinimumSize(QSize(28,28)) ;
	inviteFriendsButton->setMaximumSize(QSize(28,28)) ;
	inviteFriendsButton->setText(QString()) ;
	inviteFriendsButton->setToolTip(tr("Invite friends to this lobby"));

	{
	QIcon icon ;
	icon.addPixmap(QPixmap(":/images/edit_add24.png")) ;
	inviteFriendsButton->setIcon(icon) ;
	inviteFriendsButton->setIconSize(QSize(22,22)) ;
	}

	connect(inviteFriendsButton, SIGNAL(clicked()), this , SLOT(inviteFriends()));

	getChatWidget()->addChatBarWidget(inviteFriendsButton) ;

	unsubscribeButton = new QPushButton ;
	unsubscribeButton->setMinimumSize(QSize(28,28)) ;
	unsubscribeButton->setMaximumSize(QSize(28,28)) ;
	unsubscribeButton->setText(QString()) ;
	unsubscribeButton->setToolTip(tr("Leave this lobby (Unsubscribe)"));

	{
	QIcon icon ;
	icon.addPixmap(QPixmap(":/images/exit_32.png")) ;
	unsubscribeButton->setIcon(icon) ;
	unsubscribeButton->setIconSize(QSize(22,22)) ;
	}

	connect(unsubscribeButton, SIGNAL(clicked()), this , SLOT(leaveLobby()));

	getChatWidget()->addChatBarWidget(unsubscribeButton) ;
}

void ChatLobbyDialog::leaveLobby()
{
	emit lobbyLeave(id()) ;
}
void ChatLobbyDialog::inviteFriends()
{
	std::cerr << "Inviting friends" << std::endl;

	std::list<std::string> ids = FriendSelectionDialog::selectFriends(NULL,tr("Invite friends"),tr("Select friends to invite:")) ;

	std::cerr << "Inviting these friends:" << std::endl;

	ChatLobbyId lobby_id;
	if (!rsMsgs->isLobbyId(getPeerId(), lobby_id)) 
		return ;

	for(std::list<std::string>::const_iterator it(ids.begin());it!=ids.end();++it)
	{
		std::cerr << "    " << *it  << std::endl;

		rsMsgs->invitePeerToLobby(lobby_id,*it) ;
	}
}

void ChatLobbyDialog::participantsTreeWidgetCustomPopupMenu(QPoint)
{
	QList<QTreeWidgetItem*> selectedItems = ui.participantsList->selectedItems();

	QMenu contextMnu(this);

	contextMnu.addAction(muteAct);
	muteAct->setCheckable(true);
	muteAct->setEnabled(false);
	muteAct->setChecked(false);
	if (selectedItems.size()) {

        std::string nickName;
        rsMsgs->getNickNameForChatLobby(lobbyId, nickName);
        if(selectedItems.count()>1 || (selectedItems.at(0)->text(COLUMN_NAME).toStdString()!=nickName))
        {
		muteAct->setEnabled(true);

		QList<QTreeWidgetItem*>::iterator item;
		for (item = selectedItems.begin(); item != selectedItems.end(); ++item) {
			if (isParticipantMuted((*item)->text(COLUMN_NAME))) {
				muteAct->setChecked(true);
				break;
			}
		}
	}
    }

	contextMnu.exec(QCursor::pos());
}

void ChatLobbyDialog::init(const std::string &peerId, const QString &title)
{
	std::list<ChatLobbyInfo> lobbies;
	rsMsgs->getChatLobbyList(lobbies);

	std::list<ChatLobbyInfo>::const_iterator lobbyIt;
	for (lobbyIt = lobbies.begin(); lobbyIt != lobbies.end(); ++lobbyIt) {
		std::string vpid;
		if (rsMsgs->getVirtualPeerId(lobbyIt->lobby_id, vpid)) {
			if (vpid == peerId) {
				QString msg = tr("Welcome to lobby %1").arg(RsHtml::plainText(lobbyIt->lobby_name));
				_lobby_name = QString::fromUtf8(lobbyIt->lobby_name.c_str()) ;
				if (!lobbyIt->lobby_topic.empty()) {
					msg += "\n" + tr("Topic: %1").arg(RsHtml::plainText(lobbyIt->lobby_topic));
				}
				ui.chatWidget->setWelcomeMessage(msg);
				break;
			}
		}
	}

	ChatDialog::init(peerId, title);

	std::string nickName;
	rsMsgs->getNickNameForChatLobby(lobbyId, nickName);
	ui.chatWidget->setName(QString::fromUtf8(nickName.c_str()));

	ui.chatWidget->addToolsAction(ui.actionChangeNickname);
	ui.chatWidget->setDefaultExtraFileFlags(RS_FILE_REQ_ANONYMOUS_ROUTING);

	lastUpdateListTime = 0;

	/* Hide or show the participants frames */
	showParticipantsFrame(PeerSettings->getShowParticipantsFrame(peerId));

	// add to window

	dynamic_cast<ChatLobbyWidget*>(MainWindow::getPage(MainWindow::ChatLobby))->addChatPage(this) ;

	/** List of muted Participants */
	mutedParticipants = new QStringList;
	
	// load settings
	processSettings(true);
}

/** Destructor. */
ChatLobbyDialog::~ChatLobbyDialog()
{
	// announce leaving of lobby

	// check that the lobby still exists.
	ChatLobbyId lid;
	if (rsMsgs->isLobbyId(getPeerId(), lid)) {
		rsMsgs->unsubscribeChatLobby(lobbyId);
	}

	// save settings
	processSettings(false);
}

ChatWidget *ChatLobbyDialog::getChatWidget()
{
	return ui.chatWidget;
}

bool ChatLobbyDialog::notifyBlink()
{
	return (Settings->getChatLobbyFlags() & RS_CHATLOBBY_BLINK);
}

void ChatLobbyDialog::processSettings(bool load)
{
	Settings->beginGroup(QString("ChatLobbyDialog"));

	if (load) {
		// load settings
	} else {
		// save settings
	}

	Settings->endGroup();
}

/**
 * Change your Nickname
 * 
 * - send a Message to all Members => later: send hidden message to clients, so they can actualize there mutedParticipants list
 */
void ChatLobbyDialog::setNickname(const QString &nickname)
{
	rsMsgs->setNickNameForChatLobby(lobbyId, nickname.toUtf8().constData());

	// get new nick name
	std::string newNickname;
	if (rsMsgs->getNickNameForChatLobby(lobbyId, newNickname)) {
		ui.chatWidget->setName(QString::fromUtf8(newNickname.c_str()));
	}
}

/**
 * Dialog: Change your Nickname in the ChatLobby
 */
void ChatLobbyDialog::changeNickname()
{
	QInputDialog dialog;
	dialog.setWindowTitle(tr("Change nick name"));
	dialog.setLabelText(tr("Please enter your new nick name"));
	dialog.setWindowIcon(QIcon(":/images/rstray3.png"));

	std::string nickName;
	rsMsgs->getNickNameForChatLobby(lobbyId, nickName);
	dialog.setTextValue(QString::fromUtf8(nickName.c_str()));

	if (dialog.exec() == QDialog::Accepted && !dialog.textValue().isEmpty()) {
		setNickname(dialog.textValue());
	}
}

/**
 * We get a new Message from a chat participant
 * 
 * - Ignore Messages from muted chat participants
 */
void ChatLobbyDialog::addIncomingChatMsg(const ChatInfo& info)
{
	QDateTime sendTime = QDateTime::fromTime_t(info.sendTime);
	QDateTime recvTime = QDateTime::fromTime_t(info.recvTime);
	QString message = QString::fromStdWString(info.msg);
	QString name = QString::fromUtf8(info.peer_nickname.c_str());
	QString rsid = QString::fromUtf8(info.rsid.c_str());

	std::cerr << "message from rsid " << info.rsid.c_str() << std::endl;
	
	if(!isParticipantMuted(name)) {
	  ui.chatWidget->addChatMsg(true, name, sendTime, recvTime, message, ChatWidget::TYPE_NORMAL);
		emit messageReceived(id()) ;
	}
	
	// This is a trick to translate HTML into text.
	QTextEdit editor;
	editor.setHtml(message);
	QString notifyMsg = name + ": " + editor.toPlainText();

	if(notifyMsg.length() > 30)
		MainWindow::displayLobbySystrayMsg(tr("Lobby chat") + ": " + _lobby_name, notifyMsg.left(30) + QString("..."));
	else
		MainWindow::displayLobbySystrayMsg(tr("Lobby chat") + ": " + _lobby_name, notifyMsg);

	// also update peer list.

	time_t now = time(NULL);

   QList<QTreeWidgetItem*>  qlFoundParticipants=ui.participantsList->findItems(name,Qt::MatchExactly,COLUMN_NAME);
    if (qlFoundParticipants.count()!=0) qlFoundParticipants.at(0)->setText(COLUMN_ACTIVITY,QString::number(now));

	if (now > lastUpdateListTime) {
		lastUpdateListTime = now;
		updateParticipantsList();
	}
}

/**
 * Regenerate the QTreeWidget participant list of a Chat Lobby
 * 
 * Show yellow icon for muted Participants
 */
void ChatLobbyDialog::updateParticipantsList()
{
    std::list<ChatLobbyInfo> lInfos;
    rsMsgs->getChatLobbyList(lInfos);

    std::list<ChatLobbyInfo>::const_iterator it(lInfos.begin());

    // Set it to the current ChatLobby
    for (; it!=lInfos.end() && (*it).lobby_id != lobbyId; ++it);

    if (it != lInfos.end()) {
        ChatLobbyInfo cliInfo=(*it);
        QList<QTreeWidgetItem*>  qlOldParticipants=ui.participantsList->findItems("*",Qt::MatchWildcard);
        foreach(QTreeWidgetItem *qtwiCur,qlOldParticipants){
            QString qsOldParticipant = qtwiCur->text(COLUMN_NAME);

            std::map<std::string,time_t>::iterator itFound(cliInfo.nick_names.find(qsOldParticipant.toStdString())) ;

            if(itFound == cliInfo.nick_names.end())
            {
                //Old Participant go out, remove it
                ui.participantsList->removeItemWidget(qtwiCur,0);
            }
        }



		for (std::map<std::string,time_t>::const_iterator it2((*it).nick_names.begin()); it2 != (*it).nick_names.end(); ++it2) {
			QString participant = QString::fromUtf8( (it2->first).c_str() );
            QList<QTreeWidgetItem*>  qlFoundParticipants=ui.participantsList->findItems(participant,Qt::MatchExactly,COLUMN_NAME);
            QTreeWidgetItem *widgetitem;

            if (qlFoundParticipants.count()==0)
            {
			// TE: Add Wigdet to participantsList with Checkbox, to mute Participant
                widgetitem = new RSTreeWidgetItem;
                widgetitem->setText(COLUMN_NAME, participant);
                widgetitem->setText(COLUMN_ACTIVITY,QString::number(time(NULL)));
                ui.participantsList->addTopLevelItem(widgetitem);
            } else { widgetitem = qlFoundParticipants.at(0);}

			if (isParticipantMuted(participant)) {
                widgetitem->setIcon(COLUMN_ICON, QIcon(":/images/redled.png"));
                widgetitem->setTextColor(COLUMN_NAME,QColor(255,0,0));
			} else {
				widgetitem->setIcon(COLUMN_ICON, QIcon(":/images/greenled.png"));
                widgetitem->setTextColor(COLUMN_NAME,ui.participantsList->palette().color(QPalette::Active, QPalette::Text));
			}

            time_t tLastAct=widgetitem->text(COLUMN_ACTIVITY).toInt();
            time_t now = time(NULL);
            if (tLastAct<now-60*30) widgetitem->setIcon(COLUMN_ICON, QIcon(":/images/grayled.png"));

            std::string nickName;
            rsMsgs->getNickNameForChatLobby(lobbyId, nickName);
            if (participant.toStdString()==nickName) widgetitem->setIcon(COLUMN_ICON, QIcon(":/images/yellowled.png"));


            QTime qtLastAct=QTime(0,0,0).addSecs(now-tLastAct);
            widgetitem->setToolTip(COLUMN_NAME,tr("Right click to mute/unmute participants<br/>Double click to address this person<br/>")
                                                  +tr("This participant is not active since:")
                                                  +qtLastAct.toString()
                                                  +tr(" seconds")
                                                  );
		}
	}
	ui.participantsList->setSortingEnabled(true);
	ui.participantsList->sortItems(COLUMN_NAME, Qt::AscendingOrder);
}

/**
 * Called when a Participant in QTree get Clicked / Changed
 * 
 * Check if the Checkbox altered and Mute User
 * 
 * @todo auf rsid
 * 
 * @param QTreeWidgetItem Participant to check
 */
void ChatLobbyDialog::changePartipationState()
{
	QList<QTreeWidgetItem*> selectedItems = ui.participantsList->selectedItems();
	if (selectedItems.size() == 0) {
		return;
	}

	QList<QTreeWidgetItem*>::iterator item;
	for (item = selectedItems.begin(); item != selectedItems.end(); ++item) {
		QString nickname = (*item)->text(COLUMN_NAME);

		std::cerr << "check Partipation status for '" << nickname.toUtf8().constData() << std::endl;

		if (muteAct->isChecked()) {
			muteParticipant(nickname);
		} else {
			unMuteParticipant(nickname);
		}
	}

	mutedParticipants->removeDuplicates();

	updateParticipantsList();
}

void ChatLobbyDialog::participantsTreeWidgetDoubleClicked(QTreeWidgetItem *item, int column)
{
	if (!item) {
		return;
	}

	if(column == COLUMN_NAME)
	{
		getChatWidget()->pasteText("@" + RsHtml::plainText(item->text(COLUMN_NAME))) ;
		return ;
	}

//	if (column == COLUMN_ICON) {
//		return;
//	}
//
//	QString nickname = item->text(COLUMN_NAME);
//	if (isParticipantMuted(nickname)) {
//		unMuteParticipant(nickname);
//	} else {
//		muteParticipant(nickname);
//	}
//
//	mutedParticipants->removeDuplicates();
//
//	updateParticipantsList();
}

void ChatLobbyDialog::muteParticipant(const QString &nickname) {
	std::cerr << " Mute " << std::endl;
    std::string myNickName;
    rsMsgs->getNickNameForChatLobby(lobbyId, myNickName);
    if (nickname.toStdString()!=myNickName)
	mutedParticipants->append(nickname);
}

void ChatLobbyDialog::unMuteParticipant(const QString &nickname) {
	std::cerr << " UnMute " << std::endl;
	mutedParticipants->removeAll(nickname);
}

/**
 * Is this nickName already known in the lobby
 */
bool ChatLobbyDialog::isNicknameInLobby(const QString &nickname) {

	std::list<ChatLobbyInfo> linfos;
	rsMsgs->getChatLobbyList(linfos);

	std::list<ChatLobbyInfo>::const_iterator it(linfos.begin());
	
	// Set it to the current ChatLobby
	for (; it!=linfos.end() && (*it).lobby_id != lobbyId; ++it);

	if (it != linfos.end()) {
		for (std::map<std::string,time_t>::const_iterator it2((*it).nick_names.begin()); it2 != (*it).nick_names.end(); ++it2) {
			QString participant = QString::fromUtf8( (it2->first).c_str() );
			if (participant==nickname) {
				return true;
			}
		}
	}
	return false;
}

/** 
 * Should Messages from this Nickname be muted?
 * 
 * At the moment it is not possible to 100% know which peer sendet the message, and only
 * the nickname is available. So this couldn't work for 100%. So, for example,  if a peer 
 * change his name to the name of a other peer, we couldn't block him. A real implementation 
 * will be possible if we transfer a temporary Session ID from the sending Retroshare client
 * version 0.6
 * 
 * @param QString nickname to check
 */
bool ChatLobbyDialog::isParticipantMuted(const QString &participant)
{
 	// nickname in Mute list
	return mutedParticipants->contains(participant);
}

void ChatLobbyDialog::displayLobbyEvent(int event_type, const QString& nickname, const QString& str)
{
    QString qsParticipant="";
	switch (event_type) {
	case RS_CHAT_LOBBY_EVENT_PEER_LEFT:
        qsParticipant=str;
		ui.chatWidget->addChatMsg(true, tr("Lobby management"), QDateTime::currentDateTime(), QDateTime::currentDateTime(), tr("%1 has left the lobby.").arg(RsHtml::plainText(str)), ChatWidget::TYPE_SYSTEM);
		emit peerLeft(id()) ;
		break;
	case RS_CHAT_LOBBY_EVENT_PEER_JOINED:
        qsParticipant=str;
		ui.chatWidget->addChatMsg(true, tr("Lobby management"), QDateTime::currentDateTime(), QDateTime::currentDateTime(), tr("%1 joined the lobby.").arg(RsHtml::plainText(str)), ChatWidget::TYPE_SYSTEM);
		emit peerJoined(id()) ;
		break;
	case RS_CHAT_LOBBY_EVENT_PEER_STATUS:
        qsParticipant=nickname;
		ui.chatWidget->updateStatusString(RsHtml::plainText(nickname) + " %1", RsHtml::plainText(str));
		if (!isParticipantMuted(nickname)) {
			emit typingEventReceived(id()) ;
		}
		break;
	case RS_CHAT_LOBBY_EVENT_PEER_CHANGE_NICKNAME:
        qsParticipant=str;
		ui.chatWidget->addChatMsg(true, tr("Lobby management"), QDateTime::currentDateTime(), QDateTime::currentDateTime(), tr("%1 changed his name to: %2").arg(RsHtml::plainText(nickname), RsHtml::plainText(str)), ChatWidget::TYPE_SYSTEM);
		
		// TODO if a user was muted and changed his name, update mute list, but only, when the muted peer, dont change his name to a other peer in your chat lobby
		if (isParticipantMuted(nickname) && !isNicknameInLobby(str)) {
			muteParticipant(str);
		}
		
	break;
	case RS_CHAT_LOBBY_EVENT_KEEP_ALIVE:
		//std::cerr << "Received keep alive packet from " << nickname.toStdString() << " in lobby " << getPeerId() << std::endl;
		break;
	default:
		std::cerr << "ChatLobbyDialog::displayLobbyEvent() Unhandled lobby event type " << event_type << std::endl;
	}

    if (qsParticipant!=""){
        QList<QTreeWidgetItem*>  qlFoundParticipants=ui.participantsList->findItems(str,Qt::MatchExactly,COLUMN_NAME);
        if (qlFoundParticipants.count()!=0) qlFoundParticipants.at(0)->setText(COLUMN_ACTIVITY,QString::number(time(NULL)));
    }

	updateParticipantsList() ;
}

bool ChatLobbyDialog::canClose()
{
	// check that the lobby still exists.
	ChatLobbyId lid;
	if (!rsMsgs->isLobbyId(getPeerId(), lid)) {
		return true;
	}

	if (QMessageBox::Yes == QMessageBox::question(this, tr("Unsubscribe to lobby"), tr("Do you want to unsubscribe to this chat lobby?"), QMessageBox::Yes | QMessageBox::No)) {
		return true;
	}

	return false;
}

void ChatLobbyDialog::showDialog(uint chatflags)
{
	if (chatflags & RS_CHAT_FOCUS) 
	{
		MainWindow::showWindow(MainWindow::ChatLobby);
		dynamic_cast<ChatLobbyWidget*>(MainWindow::getPage(MainWindow::ChatLobby))->setCurrentChatPage(this) ;
	}
}

void ChatLobbyDialog::showParticipantsFrame(bool show)
{
	ui.participantsFrame->setVisible(show);
	ui.participantsFrameButton->setChecked(show);

	if (show) {
		ui.participantsFrameButton->setToolTip(tr("Hide Participants"));
		ui.participantsFrameButton->setIcon(QIcon(":images/hide_toolbox_frame.png"));
	} else {
		ui.participantsFrameButton->setToolTip(tr("Show Participants"));
		ui.participantsFrameButton->setIcon(QIcon(":images/show_toolbox_frame.png"));
	}

	PeerSettings->setShowParticipantsFrame(getPeerId(), show);
}
