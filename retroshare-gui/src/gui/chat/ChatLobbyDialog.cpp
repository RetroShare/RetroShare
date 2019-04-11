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
 *
 ****************************************************************/


#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QWidgetAction>

#include "ChatLobbyDialog.h"

#include "ChatTabWidget.h"
#include "gui/ChatLobbyWidget.h"
#include "gui/FriendsDialog.h"
#include "gui/MainWindow.h"
#include "gui/common/html.h"
#include "gui/common/FriendSelectionDialog.h"
#include "gui/common/RSTreeWidgetItem.h"
#include "gui/gxs/GxsIdChooser.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/gxs/GxsIdTreeWidgetItem.h"
#include "gui/Identity/IdDialog.h"
#include "gui/msgs/MessageComposer.h"
#include "gui/settings/RsharePeerSettings.h"
#include "gui/settings/rsharesettings.h"
#include "util/HandleRichText.h"
#include "util/QtVersion.h"

#include "retroshare/rsnotify.h"
#include "util/rstime.h"

#include <time.h>
#include <unistd.h>

#define COLUMN_NAME      0
#define COLUMN_ACTIVITY  1
#define COLUMN_ID        2
#define COLUMN_ICON      3
#define COLUMN_COUNT     4

#define ROLE_SORT            Qt::UserRole + 1

const static uint32_t timeToInactivity = 60 * 10;   // in seconds

/** Default constructor */
ChatLobbyDialog::ChatLobbyDialog(const ChatLobbyId& lid, QWidget *parent, Qt::WindowFlags flags)
        : ChatDialog(parent, flags), lobbyId(lid),
          bullet_red_128(":/icons/bullet_red_128.png"), bullet_grey_128(":/icons/bullet_grey_128.png"),
          bullet_green_128(":/icons/bullet_green_128.png"), bullet_yellow_128(":/icons/bullet_yellow_128.png")
{
	/* Invoke Qt Designer generated QObject setup routine */
	ui.setupUi(this);

	lastUpdateListTime = 0;

        //connect(ui.actionChangeNickname, SIGNAL(triggered()), this, SLOT(changeNickname()));
	connect(ui.participantsList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(participantsTreeWidgetCustomPopupMenu(QPoint)));
	connect(ui.participantsList, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(participantsTreeWidgetDoubleClicked(QTreeWidgetItem*,int)));

	connect(ui.filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));

            int S = QFontMetricsF(font()).height() ;
    ui.participantsList->setIconSize(QSize(1.4*S,1.4*S));

    ui.participantsList->setColumnCount(COLUMN_COUNT);
    ui.participantsList->setColumnWidth(COLUMN_ICON, 1.7*S);
    ui.participantsList->setColumnHidden(COLUMN_ACTIVITY,true);
    ui.participantsList->setColumnHidden(COLUMN_ID,true);
    
    /* Set header resize modes and initial section sizes */
	QHeaderView * header = ui.participantsList->header();
	QHeaderView_setSectionResizeModeColumn(header, COLUMN_NAME, QHeaderView::Stretch);

    muteAct = new QAction(QIcon(), tr("Mute participant"), this);
    voteNegativeAct = new QAction(QIcon(":/icons/png/thumbs-down.png"), tr("Ban this person (Sets negative opinion)"), this);
    voteNeutralAct = new QAction(QIcon(":/icons/png/thumbs-neutral.png"), tr("Give neutral opinion"), this);
    votePositiveAct = new QAction(QIcon(":/icons/png/thumbs-up.png"), tr("Give positive opinion"), this);
    distantChatAct = new QAction(QIcon(":/images/chat_24.png"), tr("Start private chat"), this);
    sendMessageAct = new QAction(QIcon(":/images/mail_new.png"), tr("Send Message"), this);
    showinpeopleAct = new QAction(QIcon(), tr("Show author in people tab"), this);
	
    QActionGroup *sortgrp = new QActionGroup(this);
    actionSortByName = new QAction(QIcon(), tr("Sort by Name"), this);
    actionSortByName->setCheckable(true);
    actionSortByName->setChecked(true); 
    actionSortByName->setActionGroup(sortgrp);

    actionSortByActivity = new QAction(QIcon(), tr("Sort by Activity"), this);
    actionSortByActivity->setCheckable(true);
    actionSortByActivity->setChecked(false); 
    actionSortByActivity->setActionGroup(sortgrp);


    connect(muteAct, SIGNAL(triggered()), this, SLOT(changePartipationState()));
    connect(distantChatAct, SIGNAL(triggered()), this, SLOT(distantChatParticipant()));
    connect(sendMessageAct, SIGNAL(triggered()), this, SLOT(sendMessage()));
    connect(votePositiveAct, SIGNAL(triggered()), this, SLOT(voteParticipant()));
    connect(voteNeutralAct, SIGNAL(triggered()), this, SLOT(voteParticipant()));
    connect(voteNegativeAct, SIGNAL(triggered()), this, SLOT(voteParticipant()));
    connect(showinpeopleAct, SIGNAL(triggered()), this, SLOT(showInPeopleTab()));

    votePositiveAct->setData(RsReputations::OPINION_POSITIVE);
    voteNeutralAct->setData(RsReputations::OPINION_NEUTRAL);
    voteNegativeAct->setData(RsReputations::OPINION_NEGATIVE);

    connect(actionSortByName, SIGNAL(triggered()), this, SLOT(sortParcipants()));
    connect(actionSortByActivity, SIGNAL(triggered()), this, SLOT(sortParcipants()));
    
    	/* Add filter actions */
	QTreeWidgetItem *headerItem = ui.participantsList->headerItem();
	QString headerText = headerItem->text(COLUMN_NAME );
	ui.filterLineEdit->addFilter(QIcon(), headerText, COLUMN_NAME , QString("%1 %2").arg(tr("Search"), headerText));

	// Add a button to invite friends.
	//
	inviteFriendsButton = new QToolButton ;
  inviteFriendsButton->setMinimumSize(QSize(2.4*S,2.4*S)) ;
  inviteFriendsButton->setMaximumSize(QSize(2.4*S,2.4*S)) ;
	inviteFriendsButton->setText(QString()) ;
	inviteFriendsButton->setAutoRaise(true) ;
	inviteFriendsButton->setToolTip(tr("Invite friends to this lobby"));

        mParticipantCompareRole = new RSTreeWidgetItemCompareRole;
        mParticipantCompareRole->setRole(COLUMN_ACTIVITY, ROLE_SORT);

	{
	QIcon icon ;
	icon.addPixmap(QPixmap(":/icons/png/invite.png")) ;
	inviteFriendsButton->setIcon(icon) ;
    inviteFriendsButton->setIconSize(QSize(2.4*S,2.4*S)) ;
	}

	connect(inviteFriendsButton, SIGNAL(clicked()), this , SLOT(inviteFriends()));

    getChatWidget()->addTitleBarWidget(inviteFriendsButton) ;

    RsGxsId current_id;
    rsMsgs->getIdentityForChatLobby(lobbyId, current_id);

    uint32_t idChooserFlag = IDCHOOSER_ID_REQUIRED;
    ChatLobbyInfo lobbyInfo ;
    if(rsMsgs->getChatLobbyInfo(lobbyId,lobbyInfo)) {
        if (lobbyInfo.lobby_flags & RS_CHAT_LOBBY_FLAGS_PGP_SIGNED) {
            idChooserFlag |= IDCHOOSER_NON_ANONYMOUS;
        }
    }
    ownIdChooser = new GxsIdChooser() ;
    ownIdChooser->loadIds(idChooserFlag, current_id) ;
    
    QWidgetAction *checkableAction = new QWidgetAction(this);
    checkableAction->setDefaultWidget(ownIdChooser);

    ui.chatWidget->addToolsAction(checkableAction);
    //getChatWidget()->addChatBarWidget(ownIdChooser);
    


    connect(ownIdChooser,SIGNAL(currentIndexChanged(int)),this,SLOT(changeNickname())) ;

    unsubscribeButton = new QToolButton ;
    unsubscribeButton->setMinimumSize(QSize(2.4*S,2.4*S)) ;
    unsubscribeButton->setMaximumSize(QSize(2.4*S,2.4*S)) ;
	unsubscribeButton->setText(QString()) ;
	unsubscribeButton->setAutoRaise(true) ;
	unsubscribeButton->setToolTip(tr("Leave this chat room (Unsubscribe)"));

	{
	QIcon icon ;
	icon.addPixmap(QPixmap(":/icons/png/leave.png")) ;
	unsubscribeButton->setIcon(icon) ;
    unsubscribeButton->setIconSize(QSize(2.4*S,2.4*S)) ;
	}

	/* Initialize splitter */
	ui.splitter->setStretchFactor(0, 1);
	ui.splitter->setStretchFactor(1, 0);

	connect(unsubscribeButton, SIGNAL(clicked()), this , SLOT(leaveLobby()));

	getChatWidget()->addTitleBarWidget(unsubscribeButton) ;
}

void ChatLobbyDialog::leaveLobby()
{
	emit lobbyLeave(id()) ;
}
void ChatLobbyDialog::inviteFriends()
{
	std::cerr << "Inviting friends" << std::endl;

    std::set<RsPeerId> ids = FriendSelectionDialog::selectFriends_SSL(NULL,tr("Invite friends"),tr("Select friends to invite:")) ;

	std::cerr << "Inviting these friends:" << std::endl;

    if (!mChatId.isLobbyId())
		return ;

    for(std::set<RsPeerId>::const_iterator it(ids.begin());it!=ids.end();++it)
	{
		std::cerr << "    " << *it  << std::endl;

        rsMsgs->invitePeerToLobby(mChatId.toLobbyId(),*it) ;
	}
}

void ChatLobbyDialog::participantsTreeWidgetCustomPopupMenu(QPoint)
{
	QList<QTreeWidgetItem*> selectedItems = ui.participantsList->selectedItems();

	QMenu contextMnu(this);

    contextMnu.addAction(distantChatAct);
	contextMnu.addAction(sendMessageAct);
    contextMnu.addSeparator();
    contextMnu.addAction(actionSortByActivity);
    contextMnu.addAction(actionSortByName);
    contextMnu.addSeparator();
    contextMnu.addAction(muteAct);
    contextMnu.addAction(votePositiveAct);
    contextMnu.addAction(voteNeutralAct);
    contextMnu.addAction(voteNegativeAct);
	contextMnu.addAction(showinpeopleAct);

	distantChatAct->setEnabled(false);
	sendMessageAct->setEnabled(selectedItems.count()==1);
	muteAct->setCheckable(true);
    muteAct->setEnabled(false);
    muteAct->setChecked(false);
    votePositiveAct->setEnabled(false);
    voteNeutralAct->setEnabled(false);
    voteNegativeAct->setEnabled(false);
	showinpeopleAct->setEnabled(selectedItems.count()==1);
    if(selectedItems.count()==1)
    {
        RsGxsId gxsid(selectedItems.at(0)->text(COLUMN_ID).toStdString());

		if(!gxsid.isNull() && !rsIdentity->isOwnId(gxsid))
		{
			distantChatAct->setEnabled(true);
			votePositiveAct->setEnabled(rsReputations->overallReputationLevel(gxsid) != RsReputations::REPUTATION_LOCALLY_POSITIVE);
			voteNeutralAct->setEnabled((rsReputations->overallReputationLevel(gxsid) == RsReputations::REPUTATION_LOCALLY_POSITIVE) || (rsReputations->overallReputationLevel(gxsid) == RsReputations::REPUTATION_LOCALLY_NEGATIVE) );
			voteNegativeAct->setEnabled(rsReputations->overallReputationLevel(gxsid) != RsReputations::REPUTATION_LOCALLY_NEGATIVE);
			muteAct->setEnabled(true);
			muteAct->setChecked(isParticipantMuted(gxsid));
		}
    }
	contextMnu.exec(QCursor::pos());
}

void ChatLobbyDialog::voteParticipant()
{
    QList<QTreeWidgetItem*> selectedItems = ui.participantsList->selectedItems();
    if (selectedItems.isEmpty())
	    return;
    QList<QTreeWidgetItem*>::iterator item;

    QAction *act = dynamic_cast<QAction*>(sender()) ;
    if(!act)
    {
        std::cerr << "No sender! Some bug in the code." << std::endl;
        return ;
    }

	RsReputations::Opinion op = RsReputations::Opinion(act->data().toUInt()) ;

    for (item = selectedItems.begin(); item != selectedItems.end(); ++item)
	{
		RsGxsId nickname;
	    dynamic_cast<GxsIdRSTreeWidgetItem*>(*item)->getId(nickname) ;

		rsReputations->setOwnOpinion(nickname, op);
		std::cerr << "Giving opinion to GXS id " << nickname << " to " << op<< std::endl;
		dynamic_cast<GxsIdRSTreeWidgetItem*>(*item)->forceUpdate();
		if (op == RsReputations::OPINION_NEGATIVE)
		{
			ui.chatWidget->eraseBannedMessages(nickname);
		}
    }
}

void ChatLobbyDialog::showInPeopleTab()
{
    QList<QTreeWidgetItem*> selectedItems = ui.participantsList->selectedItems();
    if (selectedItems.count()!=1)
        return;
    RsGxsId nickname;
    dynamic_cast<GxsIdRSTreeWidgetItem*>(*selectedItems.begin())->getId(nickname);
    IdDialog *idDialog = dynamic_cast<IdDialog*>(MainWindow::getPage(MainWindow::People));
    if (!idDialog)
        return ;
    MainWindow::showWindow(MainWindow::People);
    idDialog->navigate(nickname);
}

void ChatLobbyDialog::init(const ChatId &/*id*/, const QString &/*title*/)
{
    ChatLobbyInfo linfo ;

    QString title;

    if(rsMsgs->getChatLobbyInfo(lobbyId,linfo))
    {
        title = QString::fromUtf8(linfo.lobby_name.c_str());

        QString msg = tr("Welcome to chat room %1").arg(RsHtml::plainText(linfo.lobby_name));
        _lobby_name = QString::fromUtf8(linfo.lobby_name.c_str()) ;
        if (!linfo.lobby_topic.empty()) {
            msg += "\n" + tr("Topic: %1").arg(RsHtml::plainText(linfo.lobby_topic));
        }
        ui.chatWidget->setWelcomeMessage(msg);
    }

    ChatDialog::init(ChatId(lobbyId), title);

    RsGxsId gxs_id;
    rsMsgs->getIdentityForChatLobby(lobbyId, gxs_id);

    RsIdentityDetails details ;

    // This lets the backend some time to load our own identity in cache.
    // It will only loop for at most 1 second the first time.

    for(int i=0;i<3;++i)
        if(rsIdentity->getIdDetails(gxs_id,details))
            break ;
        else
            rstime::rs_usleep(1000*300) ;

    ui.chatWidget->setName(QString::fromUtf8(details.mNickname.c_str()));
    //ui.chatWidget->addToolsAction(ui.actionChangeNickname);
    ui.chatWidget->setDefaultExtraFileFlags(RS_FILE_REQ_ANONYMOUS_ROUTING);

    lastUpdateListTime = 0;

    // add to window

    ChatLobbyWidget *chatLobbyPage = dynamic_cast<ChatLobbyWidget*>(MainWindow::getPage(MainWindow::ChatLobby));
    if (chatLobbyPage) {
        chatLobbyPage->addChatPage(this) ;
    }

    /** List of muted Participants */
    mutedParticipants.clear() ;

    // load settings
    processSettings(true);
}

/** Destructor. */
ChatLobbyDialog::~ChatLobbyDialog()
{
	// announce leaving of lobby

	// check that the lobby still exists.
    if (mChatId.isLobbyId()) {
        rsMsgs->unsubscribeChatLobby(mChatId.toLobbyId());
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

		// state of splitter
		ui.splitter->restoreState(Settings->value("splitter").toByteArray());
		
		// load sorting
		actionSortByActivity->setChecked(Settings->value("sortbyActivity", QVariant(false)).toBool());
		actionSortByName->setChecked(Settings->value("sortbyName", QVariant(true)).toBool());
		
	} else {
		// save settings

		// state of splitter
		Settings->setValue("splitter", ui.splitter->saveState());
		
		//save sorting
		Settings->setValue("sortbyActivity", actionSortByActivity->isChecked());
		Settings->setValue("sortbyName", actionSortByName->isChecked());
	}

	Settings->endGroup();
}

/**
 * Change your Nickname
 * 
 * - send a Message to all Members => later: send hidden message to clients, so they can actualize there mutedParticipants list
 */
void ChatLobbyDialog::setIdentity(const RsGxsId& gxs_id)
{
    rsMsgs->setIdentityForChatLobby(lobbyId, gxs_id) ;

    // get new nick name
    RsGxsId newid;

    if (rsMsgs->getIdentityForChatLobby(lobbyId, newid))
    {
        RsIdentityDetails details ;
        rsIdentity->getIdDetails(gxs_id,details) ;

        ui.chatWidget->setName(QString::fromUtf8(details.mNickname.c_str()));
    }
}

/**
 * Dialog: Change your Nickname in the ChatLobby
 */
void ChatLobbyDialog::changeNickname()
{
    RsGxsId current_id;
    rsMsgs->getIdentityForChatLobby(lobbyId, current_id);

    RsGxsId new_id ;
    ownIdChooser->getChosenId(new_id) ;

    if(!new_id.isNull() && new_id != current_id)
        setIdentity(new_id);
}

/**
 * We get a new Message from a chat participant
 * 
 * - Ignore Messages from muted chat participants
 */
void ChatLobbyDialog::addChatMsg(const ChatMessage& msg)
{
    QDateTime sendTime = QDateTime::fromTime_t(msg.sendTime);
    QDateTime recvTime = QDateTime::fromTime_t(msg.recvTime);
    QString message = QString::fromUtf8(msg.msg.c_str());
    RsGxsId gxs_id = msg.lobby_peer_gxs_id ;
	
    if(!isParticipantMuted(gxs_id))
    {
        // We could change addChatMsg to display the peers icon, passing a ChatId

        RsIdentityDetails details ;

        QString name ;
        if(rsIdentity->getIdDetails(gxs_id,details))
            name = QString::fromUtf8(details.mNickname.c_str()) ;
        else
            name = QString::fromUtf8(msg.peer_alternate_nickname.c_str()) + " (" + QString::fromStdString(gxs_id.toStdString()) + ")" ;

        ui.chatWidget->addChatMsg(msg.incoming, name, gxs_id, sendTime, recvTime, message, ChatWidget::MSGTYPE_NORMAL);
        emit messageReceived(msg.incoming, id(), sendTime, name, message) ;

        // This is a trick to translate HTML into text.
        QTextEdit editor;
        editor.setHtml(message);
        QString notifyMsg = name + ": " + editor.toPlainText();

        if(notifyMsg.length() > 30)
            MainWindow::displayLobbySystrayMsg(tr("Room chat") + ": " + _lobby_name, notifyMsg.left(30) + QString("..."));
        else
            MainWindow::displayLobbySystrayMsg(tr("Room chat") + ": " + _lobby_name, notifyMsg);
    }

	// also update peer list.

	time_t now = time(NULL);

   QList<QTreeWidgetItem*>  qlFoundParticipants=ui.participantsList->findItems(QString::fromStdString(gxs_id.toStdString()),Qt::MatchExactly,COLUMN_ID);
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
    ChatLobbyInfo linfo;

    if(rsMsgs->getChatLobbyInfo(lobbyId,linfo))
    {
        ChatLobbyInfo cliInfo=linfo;
        QList<QTreeWidgetItem*>  qlOldParticipants=ui.participantsList->findItems("*",Qt::MatchWildcard,COLUMN_ID);

        foreach(QTreeWidgetItem *qtwiCur,qlOldParticipants)
            if(cliInfo.gxs_ids.find(RsGxsId((*qtwiCur).text(COLUMN_ID).toStdString())) == cliInfo.gxs_ids.end())
            {
                //Old Participant go out, remove it
                int index = ui.participantsList->indexOfTopLevelItem(qtwiCur);
                delete ui.participantsList->takeTopLevelItem(index);
            }

        for (std::map<RsGxsId,time_t>::const_iterator it2(linfo.gxs_ids.begin()); it2 != linfo.gxs_ids.end(); ++it2)
        {
            QString participant = QString::fromUtf8( (it2->first).toStdString().c_str() );

            QList<QTreeWidgetItem*>  qlFoundParticipants=ui.participantsList->findItems(participant,Qt::MatchExactly,COLUMN_ID);
            GxsIdRSTreeWidgetItem *widgetitem;

            if (qlFoundParticipants.count()==0)
            {
                // TE: Add Wigdet to participantsList with Checkbox, to mute Participant

                widgetitem = new GxsIdRSTreeWidgetItem(mParticipantCompareRole,GxsIdDetails::ICON_TYPE_AVATAR);
                widgetitem->setId(it2->first,COLUMN_NAME, true) ;
                //widgetitem->setText(COLUMN_NAME, participant);
                // set activity time to the oast so that the peer is marked as inactive
                widgetitem->setText(COLUMN_ACTIVITY,QString::number(time(NULL) - timeToInactivity));
                widgetitem->setText(COLUMN_ID,QString::fromStdString(it2->first.toStdString()));

                ui.participantsList->addTopLevelItem(widgetitem);
            }
            else
                widgetitem = dynamic_cast<GxsIdRSTreeWidgetItem*>(qlFoundParticipants.at(0));

            if (isParticipantMuted(it2->first)) {
                widgetitem->setTextColor(COLUMN_NAME,QColor(255,0,0));
            } else {
                widgetitem->setTextColor(COLUMN_NAME,ui.participantsList->palette().color(QPalette::Active, QPalette::Text));
            }

            time_t tLastAct=widgetitem->text(COLUMN_ACTIVITY).toInt();
            time_t now = time(NULL);
            
                widgetitem->setSizeHint(COLUMN_ICON, QSize(20,20));


            if(isParticipantMuted(it2->first))
                widgetitem->setIcon(COLUMN_ICON, bullet_red_128);
            else if (tLastAct + timeToInactivity < now)
                widgetitem->setIcon(COLUMN_ICON, bullet_grey_128);
            else
                widgetitem->setIcon(COLUMN_ICON, bullet_green_128);

            RsGxsId gxs_id;
            rsMsgs->getIdentityForChatLobby(lobbyId, gxs_id);

            if (RsGxsId(participant.toStdString()) == gxs_id) widgetitem->setIcon(COLUMN_ICON, bullet_yellow_128);

	    widgetitem->updateBannedState();
        
            QTime qtLastAct=QTime(0,0,0).addSecs(now-tLastAct);
            widgetitem->setToolTip(COLUMN_ICON,tr("Right click to mute/unmute participants<br/>Double click to address this person<br/>")
                                   +tr("This participant is not active since:")
                                   +qtLastAct.toString()
                                   +tr(" seconds")
                                   );
        }
    }
    ui.participantsList->setSortingEnabled(true);
    sortParcipants();
    filterIds();
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

	if (selectedItems.isEmpty()) {
		return;
	}

	QList<QTreeWidgetItem*>::iterator item;
    for (item = selectedItems.begin(); item != selectedItems.end(); ++item) {

        RsGxsId gxs_id ;
        dynamic_cast<GxsIdRSTreeWidgetItem*>(*item)->getId(gxs_id) ;

        std::cerr << "check Partipation status for '" << gxs_id << std::endl;

		if (muteAct->isChecked()) {
            muteParticipant(gxs_id);
		} else {
            unMuteParticipant(gxs_id);
		}
	}

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

void ChatLobbyDialog::distantChatParticipant()
{
    std::cerr << " initiating distant chat" << std::endl;

    QList<QTreeWidgetItem*> selectedItems = ui.participantsList->selectedItems();

    if (selectedItems.isEmpty())
        return;

    if(selectedItems.size() != 1)
        return ;

    GxsIdRSTreeWidgetItem *item = dynamic_cast<GxsIdRSTreeWidgetItem*>(selectedItems.front());

    if(!item)
        return ;

    RsGxsId gxs_id ;
    item->getId(gxs_id) ;
    RsGxsId own_id;

    rsMsgs->getIdentityForChatLobby(lobbyId, own_id);

    uint32_t error_code ;
    DistantChatPeerId tunnel_id;

    if(! rsMsgs->initiateDistantChatConnexion(gxs_id,own_id,tunnel_id,error_code))
    {
        QString error_str ;
        switch(error_code)
        {
        case RS_DISTANT_CHAT_ERROR_DECRYPTION_FAILED   : error_str = tr("Decryption failed.") ; break ;
        case RS_DISTANT_CHAT_ERROR_SIGNATURE_MISMATCH  : error_str = tr("Signature mismatch") ; break ;
        case RS_DISTANT_CHAT_ERROR_UNKNOWN_KEY         : error_str = tr("Unknown key") ; break ;
        case RS_DISTANT_CHAT_ERROR_UNKNOWN_HASH        : error_str = tr("Unknown hash") ; break ;
        default:
            error_str = tr("Unknown error.") ;
        }
        QMessageBox::warning(NULL,tr("Cannot start distant chat"),tr("Distant chat cannot be initiated:")+" "+error_str
                             +QString::number(error_code)) ;
    }
}

void ChatLobbyDialog::sendMessage()
{

    QList<QTreeWidgetItem*> selectedItems = ui.participantsList->selectedItems();

    if (selectedItems.isEmpty())
        return;

    QList<QTreeWidgetItem*>::iterator item;
    for (item = selectedItems.begin(); item != selectedItems.end(); ++item) {

        RsGxsId gxs_id ;
        dynamic_cast<GxsIdRSTreeWidgetItem*>(*item)->getId(gxs_id) ;


        MessageComposer *nMsgDialog = MessageComposer::newMsg();
        if (nMsgDialog == NULL) {
          return;
        }

        nMsgDialog->addRecipient(MessageComposer::TO,  RsGxsId(gxs_id));
        nMsgDialog->show();
        nMsgDialog->activateWindow();

        /* window will destroy itself! */
    
    }

}


void ChatLobbyDialog::muteParticipant(const RsGxsId& nickname)
{
    std::cerr << " Mute " << std::endl;

    RsGxsId gxs_id;
    rsMsgs->getIdentityForChatLobby(lobbyId, gxs_id);

    if (gxs_id!=nickname)
        mutedParticipants.insert(nickname);
}

void ChatLobbyDialog::unMuteParticipant(const RsGxsId& id)
{
    std::cerr << " UnMute " << std::endl;
    mutedParticipants.erase(id);
}

/**
 * Is this nickName already known in the lobby
 */
bool ChatLobbyDialog::isNicknameInLobby(const RsGxsId& nickname)
{
    ChatLobbyInfo clinfo;

    if(! rsMsgs->getChatLobbyInfo(lobbyId,clinfo))
        return false ;

    return clinfo.gxs_ids.find(nickname) != clinfo.gxs_ids.end() ;
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
bool ChatLobbyDialog::isParticipantMuted(const RsGxsId& participant)
{
 	// nickname in Mute list
    return mutedParticipants.find(participant) != mutedParticipants.end();
}

QString ChatLobbyDialog::getParticipantName(const RsGxsId& gxs_id) const
{
    RsIdentityDetails details ;

    QString name ;
    if(rsIdentity->getIdDetails(gxs_id,details))
        name = QString::fromUtf8(details.mNickname.c_str()) ;
    else
        name = QString::fromUtf8("[Unknown] (") + QString::fromStdString(gxs_id.toStdString()) + ")" ;

    return name ;
}


void ChatLobbyDialog::displayLobbyEvent(int event_type, const RsGxsId& gxs_id, const QString& str)
{
    RsGxsId qsParticipant;

    QString name= getParticipantName(gxs_id) ;

    switch (event_type)
    {
    case RS_CHAT_LOBBY_EVENT_PEER_LEFT:
        qsParticipant=gxs_id;
        ui.chatWidget->addChatMsg(true, tr("Chat room management"), QDateTime::currentDateTime(), QDateTime::currentDateTime(), tr("%1 has left the room.").arg(RsHtml::plainText(name)), ChatWidget::MSGTYPE_SYSTEM);
        emit peerLeft(id()) ;
        break;
    case RS_CHAT_LOBBY_EVENT_PEER_JOINED:
        qsParticipant=gxs_id;
        ui.chatWidget->addChatMsg(true, tr("Chat room management"), QDateTime::currentDateTime(), QDateTime::currentDateTime(), tr("%1 joined the room.").arg(RsHtml::plainText(name)), ChatWidget::MSGTYPE_SYSTEM);
        emit peerJoined(id()) ;
        break;
    case RS_CHAT_LOBBY_EVENT_PEER_STATUS:
    {

        qsParticipant=gxs_id;

        ui.chatWidget->updateStatusString(RsHtml::plainText(name) + " %1", RsHtml::plainText(str));

        if (!isParticipantMuted(gxs_id))
            emit typingEventReceived(id()) ;

    }
        break;
    case RS_CHAT_LOBBY_EVENT_PEER_CHANGE_NICKNAME:
    {
        qsParticipant=gxs_id;

        QString newname= getParticipantName(RsGxsId(str.toStdString())) ;

        ui.chatWidget->addChatMsg(true, tr("Chat room management"), QDateTime::currentDateTime(),
                                  QDateTime::currentDateTime(),
                                  tr("%1 changed his name to: %2").arg(RsHtml::plainText(name)).arg(RsHtml::plainText(newname)),
                                  ChatWidget::MSGTYPE_SYSTEM);

        // TODO if a user was muted and changed his name, update mute list, but only, when the muted peer, dont change his name to a other peer in your chat lobby
        if (isParticipantMuted(gxs_id))
            muteParticipant(RsGxsId(str.toStdString())) ;
    }
        break;
    case RS_CHAT_LOBBY_EVENT_KEEP_ALIVE:
        //std::cerr << "Received keep alive packet from " << nickname.toStdString() << " in chat room " << getPeerId() << std::endl;
        break;
    default:
        std::cerr << "ChatLobbyDialog::displayLobbyEvent() Unhandled chat room event type " << event_type << std::endl;
    }

    if (!qsParticipant.isNull())
    {
        QList<QTreeWidgetItem*>  qlFoundParticipants=ui.participantsList->findItems(QString::fromStdString(qsParticipant.toStdString()),Qt::MatchExactly,COLUMN_ID);

        if (qlFoundParticipants.count()!=0)
        qlFoundParticipants.at(0)->setText(COLUMN_ACTIVITY,QString::number(time(NULL)));
    }

    updateParticipantsList() ;
}

bool ChatLobbyDialog::canClose()
{
	// check that the lobby still exists.
    /* TODO
	ChatLobbyId lid;
	if (!rsMsgs->isLobbyId(getPeerId(), lid)) {
		return true;
	}
    */

	if (QMessageBox::Yes == QMessageBox::question(this, tr("Unsubscribe from chat room"), tr("Do you want to unsubscribe to this chat room?"), QMessageBox::Yes | QMessageBox::No)) {
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

void ChatLobbyDialog::sortParcipants()
{

	if (actionSortByActivity->isChecked()) {
        ui.participantsList->sortItems(COLUMN_ACTIVITY, Qt::DescendingOrder);
	} else if (actionSortByName->isChecked()) {
        ui.participantsList->sortItems(COLUMN_NAME, Qt::AscendingOrder);
	}
  
}

void ChatLobbyDialog::filterChanged(const QString& /*text*/)
{
	filterIds();
}

void ChatLobbyDialog::filterIds()
{
	int filterColumn = ui.filterLineEdit->currentFilter();
	QString text = ui.filterLineEdit->text();

	ui.participantsList->filterItems(filterColumn, text);
}
