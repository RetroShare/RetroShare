/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,2007 crypton
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

#include <QFile>
#include <QFileInfo>

#include "GamesDialog.h"
#include <retroshare/rsiface.h>
#include <retroshare/rsgame.h>
#include <retroshare/rspeers.h>

#include <iostream>

#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QPixmap>
#include <QMessageBox>
#include <QHeaderView>

const uint32_t GAME_LIST_TYPE = 0;
const uint32_t GAME_LIST_SERVER = 1;
const uint32_t GAME_LIST_STATUS = 2;
const uint32_t GAME_LIST_NAME = 3;
const uint32_t GAME_LIST_ID = 4;

const uint32_t GAME_PEER_PLAYER = 0;
const uint32_t GAME_PEER_INVITE = 1;
const uint32_t GAME_PEER_INTEREST = 2;
const uint32_t GAME_PEER_PLAY = 3;
const uint32_t GAME_PEER_ID = 4;


/* Images for context menu icons */
#define IMAGE_REMOVEFRIEND       ":/images/removefriend16.png"
#define IMAGE_CHAT               ":/images/chat.png"
/* Images for Status icons */
#define IMAGE_ONLINE             ":/images/im-user.png"
#define IMAGE_OFFLINE            ":/images/im-user-offline.png"

/** Constructor */
GamesDialog::GamesDialog(QWidget *parent)
: MainPage(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  connect( ui.gameTreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( gameListPopupMenu( QPoint ) ) );
  connect( ui.peertreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( gamePeersPopupMenu( QPoint ) ) );

  connect( ui.createButton, SIGNAL( pressed( void ) ), this, SLOT( createGame( void ) ) );
  connect( ui.deleteButton, SIGNAL( pressed( void ) ), this, SLOT( deleteGame( void ) ) );
  connect( ui.inviteButton, SIGNAL( pressed( void ) ), this, SLOT( inviteGame( void ) ) );
  connect( ui.playButton,   SIGNAL( pressed( void ) ), this, SLOT( playGame  ( void ) ) );

  connect( ui.gameTreeWidget,   SIGNAL( itemSelectionChanged( void ) ), this, SLOT( updateGameDetails( void ) ) );

  /* hide the Tree +/- */
  ui.peertreeWidget -> setRootIsDecorated( false );
  
    /* Set header resize modes and initial section sizes */
//	QHeaderView * _header = ui.peertreeWidget->header () ;
//   	_header->setResizeMode (0, QHeaderView::Custom);
//	_header->setResizeMode (1, QHeaderView::Interactive);
//	_header->setResizeMode (2, QHeaderView::Interactive);
//	_header->setResizeMode (3, QHeaderView::Interactive);
//	_header->setResizeMode (4, QHeaderView::Interactive);
//	_header->setResizeMode (5, QHeaderView::Interactive);
//	_header->setResizeMode (6, QHeaderView::Interactive);
//	_header->setResizeMode (7, QHeaderView::Interactive);
//	_header->setResizeMode (8, QHeaderView::Interactive);
//	_header->setResizeMode (9, QHeaderView::Interactive);
//	_header->setResizeMode (10, QHeaderView::Interactive);
//	_header->setResizeMode (11, QHeaderView::Interactive);
//    
//	_header->resizeSection ( 0, 25 );
//	_header->resizeSection ( 1, 100 );
//	_header->resizeSection ( 2, 100 );
//	_header->resizeSection ( 3, 100 );
//	_header->resizeSection ( 4, 100 );
//	_header->resizeSection ( 5, 200 );
//	_header->resizeSection ( 6, 100 );
//	_header->resizeSection ( 7, 100 );
//	_header->resizeSection ( 8, 100 );
//	_header->resizeSection ( 9, 100 );
//	_header->resizeSection ( 10, 100 );
}


void GamesDialog::updateGameList()
{
	/* get the list of games from the server */
	std::list<RsGameInfo> gameList;
	std::list<RsGameInfo>::iterator it;

	rsGameLauncher->getGameList(gameList);

        /* get a link to the table */
        QTreeWidget *gameWidget = ui.gameTreeWidget;
	QTreeWidgetItem *oldSelect = getCurrentGame();
	QTreeWidgetItem *newSelect = NULL;
	std::string oldId;
	if (oldSelect)
	{
		oldId = (oldSelect->text(GAME_LIST_ID)).toStdString();
	}


        QList<QTreeWidgetItem *> items;
	for(it = gameList.begin(); it != gameList.end(); it++)
	{
		/* make a widget per game */
           	QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);
		std::string serverName = rsPeers->getPeerName(it->serverId);
		item -> setText(GAME_LIST_TYPE, QString::fromStdString(it->gameType));
		item -> setText(GAME_LIST_SERVER, QString::fromUtf8(serverName.c_str()));
		item -> setText(GAME_LIST_NAME, QString::fromStdWString(it->gameName));
		item -> setText(GAME_LIST_STATUS, QString::fromStdString(it->status));
		item -> setText(GAME_LIST_ID, QString::fromStdString(it->gameId));

		if ((oldSelect) && (oldId == it->gameId))
		{
			newSelect = item;
		}

		/* add to the list */
		items.append(item);
	}

	gameWidget->clear();
	gameWidget->setColumnCount(5);

	/* add the items in! */
	gameWidget->insertTopLevelItems(0, items);
	if (newSelect)
	{
		gameWidget->setCurrentItem(newSelect);
	}
	gameWidget->update(); /* update display */

	updateGameDetails();
}

QTreeWidgetItem *GamesDialog::getCurrentGame()
{
	return ui.gameTreeWidget->currentItem();
}

QTreeWidgetItem *GamesDialog::getCurrentPeer()
{
	return ui.peertreeWidget->currentItem();
}

void GamesDialog::updateGameDetails()
{
	/* get the list of games from the server */
	RsGameDetail detail;

        /* get a link to the table */
        QTreeWidget *detailWidget = ui.peertreeWidget;
	QTreeWidgetItem *gameSelect = getCurrentGame();
	if (!gameSelect)
	{
		/* clear and finished */
		detailWidget->clear();
		//detailWidget->update(); 
		mCurrentGame = "";
		mCurrentGameStatus = "";
		return;
	}

	std::string gameId = (gameSelect->text(GAME_LIST_ID)).toStdString();

	rsGameLauncher->getGameDetail(gameId, detail);

	QTreeWidgetItem *oldSelect = getCurrentPeer();
	QTreeWidgetItem *newSelect = NULL;
	std::string oldId;
	if (mCurrentGame != gameId) 
		oldSelect = NULL;   /* if we've changed game -> clear select */

	if (oldSelect)
	{
		oldId = (oldSelect->text(GAME_PEER_ID)).toStdString();
	}

        QList<QTreeWidgetItem *> items;
	/* layout depends on the game status */
	std::map<std::string, RsGamePeer>::iterator it;
	for(it = detail.gamers.begin(); it != detail.gamers.end(); it++)
	{
		bool showPeer = false;
		if ((detail.status == "Setup") ||
			(detail.status == "Invite"))
		{
			showPeer = true;
		}
		else if ((detail.status == "Confirm") ||
			 (detail.status == "Ready"))
		{
			if ((it->second).invite == true)
				showPeer = true;
		}
		else if (detail.status == "Playing")
		{
			if ((it->second).play == true)
				showPeer = true;
		}
	
		/* display */
		if (showPeer)
		{
	           	QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);
			std::string name = rsPeers->getPeerName(it->second.id);
			if (it->second.id == rsPeers->getOwnId())
			{
				name = "Yourself";
			}

			item -> setText(GAME_PEER_PLAYER, QString::fromUtf8(name.c_str()));

			if (it->second.invite)
				item -> setText(GAME_PEER_INVITE, "Yes");
			else
				item -> setText(GAME_PEER_INVITE, "No");

			if (it->second.interested)
				item -> setText(GAME_PEER_INTEREST, "Yes");
			else
				item -> setText(GAME_PEER_INTEREST, "No");

			if (it->second.play)
				item -> setText(GAME_PEER_PLAY, "Yes");
			else
				item -> setText(GAME_PEER_PLAY, "No");

			/* add a checkItem here */
			//item -> setText(GAME_PEER_PLAY, "Maybe");
			item -> setText(GAME_PEER_ID, QString::fromStdString(it->second.id));
		
			if ((oldSelect) && (oldId == it->first))
			{
				newSelect = item;
			}

			/* add to the list */
			items.append(item);
		}
	}
	if (detail.status == "Setup")
	{
		std::list<std::string> friends;
		std::list<std::string>::iterator fit;

		rsPeers->getOnlineList(friends);

        	for(fit = friends.begin(); fit != friends.end(); fit++)
	        {
			if (detail.gamers.end() != detail.gamers.find(*fit))
			{
				/* already present */
				continue;
			}

			std::string name = rsPeers->getPeerName(*fit);

	                /* make a widget per friend */
	                QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);
			item -> setText(GAME_PEER_PLAYER, QString::fromUtf8(name.c_str()));
			item -> setText(GAME_PEER_INVITE, "No");
			item -> setText(GAME_PEER_INTEREST, "?");
			item -> setText(GAME_PEER_PLAY, "?");
			item -> setText(GAME_PEER_ID, QString::fromStdString(*fit));
		
			if ((oldSelect) && (oldId == *fit))
			{
				newSelect = item;
			}

			/* add to the list */
			items.append(item);
		}
	}

	detailWidget->clear();
	detailWidget->setColumnCount(5);

	/* add the items in! */
	detailWidget->insertTopLevelItems(0, items);
	if (newSelect)
	{
		detailWidget->setCurrentItem(newSelect);
	}
	detailWidget->update(); /* update display */

	/* store the game Id */
	mCurrentGame = gameId;
        mCurrentGameStatus = detail.status;
}



void GamesDialog::gameListPopupMenu( QPoint point )
{

      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

      QAction *deleteAct = new QAction(QIcon(), tr( "Cancel Game" ), this );
      connect( deleteAct , SIGNAL( triggered() ), this, SLOT( deleteGame() ) );

      contextMnu.clear();
      contextMnu.addSeparator(); 
      contextMnu.addAction(deleteAct);
      contextMnu.addSeparator(); 
      contextMnu.exec( mevent->globalPos() );
}


void GamesDialog::gamePeersPopupMenu( QPoint point )
{

      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

      if (mCurrentGame == "")
      {
      		return;
      }

      if (mCurrentGameStatus == "Setup")
      {
      	/* invite */
	/* uninvite */

        QAction *inviteAct = new QAction(QIcon(IMAGE_EXPIORTFRIEND), tr( "Add to Invite List" ), this );
        connect( inviteAct , SIGNAL( triggered() ), this, SLOT( invitePeer() ) );
        QAction *uninviteAct = new QAction(QIcon(IMAGE_EXPIORTFRIEND), tr( "Remove from Invite List" ), this );
        connect( uninviteAct , SIGNAL( triggered() ), this, SLOT( uninvitePeer() ) );

        contextMnu.clear();
        contextMnu.addAction(inviteAct);
        contextMnu.addAction(uninviteAct);
        contextMnu.exec( mevent->globalPos() );
      }
      else if (mCurrentGameStatus == "Invite")
      {
      	/* invite */
	/* uninvite */

        QAction *inviteAct = new QAction(QIcon(IMAGE_EXPIORTFRIEND), tr( "Interested in Playing" ), this );
        connect( inviteAct , SIGNAL( triggered() ), this, SLOT( interested() ) );
        QAction *uninviteAct = new QAction(QIcon(IMAGE_EXPIORTFRIEND), tr( "Not Interested in Game" ), this );
        connect( uninviteAct , SIGNAL( triggered() ), this, SLOT( uninterested() ) );

        contextMnu.clear();
        contextMnu.addAction(inviteAct);
        contextMnu.addAction(uninviteAct);
        contextMnu.exec( mevent->globalPos() );
      }
      else if (mCurrentGameStatus == "Invite")
      {
      	/* invite */
	/* uninvite */

        QAction *interestedAct = new QAction(QIcon(IMAGE_EXPIORTFRIEND), tr( "Interested in Playing" ), this );
        connect( interestedAct , SIGNAL( triggered() ), this, SLOT( interested() ) );
        QAction *uninterestedAct = new QAction(QIcon(IMAGE_EXPIORTFRIEND), tr( "Not Interested" ), this );
        connect( uninterestedAct , SIGNAL( triggered() ), this, SLOT( uninterested() ) );

        contextMnu.clear();
        contextMnu.addAction(interestedAct);
        contextMnu.addAction(uninterestedAct);
        contextMnu.exec( mevent->globalPos() );
      }
      else if (mCurrentGameStatus == "Confirm")
      {
      	/* invite */
	/* uninvite */

        QAction *inviteAct = new QAction(QIcon(IMAGE_EXPIORTFRIEND), tr( "Confirm Peer in Game" ), this );
        connect( inviteAct , SIGNAL( triggered() ), this, SLOT( confirmPeer() ) );
        QAction *uninviteAct = new QAction(QIcon(IMAGE_EXPIORTFRIEND), tr( "Remove Peer from Game" ), this );
        connect( uninviteAct , SIGNAL( triggered() ), this, SLOT( unconfirmPeer() ) );

        contextMnu.clear();
        contextMnu.addAction(inviteAct);
        contextMnu.addAction(uninviteAct);
        contextMnu.exec( mevent->globalPos() );
      }
      else if (mCurrentGameStatus == "Ready")
      {
      	/* invite */
	/* uninvite */

        QAction *interestedAct = new QAction(QIcon(IMAGE_EXPIORTFRIEND), tr( "Interested in Game" ), this );
        connect( interestedAct , SIGNAL( triggered() ), this, SLOT( interested() ) );
        QAction *uninterestedAct = new QAction(QIcon(IMAGE_EXPIORTFRIEND), tr( "Not Interested" ), this );
        connect( uninterestedAct , SIGNAL( triggered() ), this, SLOT( uninterested() ) );

        contextMnu.clear();
        contextMnu.addAction(interestedAct);
        contextMnu.addAction(uninterestedAct);
        contextMnu.exec( mevent->globalPos() );
      }
      else /* PLAYING */
      {
      	/* no menu for playing */
        QAction *quitAct = new QAction(QIcon(IMAGE_EXPIORTFRIEND), tr( "Quit Game" ), this );
        contextMnu.clear();
        contextMnu.addAction(quitAct);
        contextMnu.exec( mevent->globalPos() );
      }

      return;
}



void GamesDialog::createGame()
{
	/* extract the Game Type and number of players from GUI */
	std::wstring gameName = ui.gameNameEdit->text().toStdWString();
	uint32_t gameType   = ui.gameComboBox->currentIndex();
	bool     addAll     = ui.checkInviteAll->isChecked();
	std::list<std::string> playerList;

	std::string gameId = rsGameLauncher->createGame(gameType, gameName);

	if (addAll)
	{
		std::list<std::string> friends;
		std::list<std::string>::iterator fit;

		rsPeers->getOnlineList(friends);
        	for(fit = friends.begin(); fit != friends.end(); fit++)
	        {
			rsGameLauncher -> invitePeer(gameId, *fit);
		}
	}

	/* call to the GameControl */
	std::string tmpName(gameName.begin(), gameName.end());
	std::cerr << "GamesDialog::createGame() Game: " << gameType << " name: " << tmpName;
	std::cerr << std::endl;

	updateGameList();
}

void GamesDialog::deleteGame()
{
	QTreeWidgetItem *gameSelect = getCurrentGame();
	if (!gameSelect)
		return;

	std::string gameId = (gameSelect->text(GAME_LIST_ID)).toStdString();
	rsGameLauncher->deleteGame(gameId);

	updateGameList();
}


void GamesDialog::inviteGame()
{
	QTreeWidgetItem *gameSelect = getCurrentGame();
	if (!gameSelect)
		return;

	std::string gameId = (gameSelect->text(GAME_LIST_ID)).toStdString();
	rsGameLauncher->inviteGame(gameId);

	updateGameList();
}


void GamesDialog::playGame()
{
	QTreeWidgetItem *gameSelect = getCurrentGame();
	if (!gameSelect)
		return;

	std::string gameId = (gameSelect->text(GAME_LIST_ID)).toStdString();
	rsGameLauncher->playGame(gameId);

	updateGameList();
}

void GamesDialog::invitePeer()
{
	QTreeWidgetItem *peerSelect = getCurrentPeer();
	if (!peerSelect)
		return;

	std::string peerId = (peerSelect->text(GAME_PEER_ID)).toStdString();
	rsGameLauncher->invitePeer(mCurrentGame, peerId);
	updateGameDetails();
}


void GamesDialog::uninvitePeer()
{
	QTreeWidgetItem *peerSelect = getCurrentPeer();
	if (!peerSelect)
		return;

	std::string peerId = (peerSelect->text(GAME_PEER_ID)).toStdString();
	rsGameLauncher->uninvitePeer(mCurrentGame, peerId);
	updateGameDetails();
}


void GamesDialog::confirmPeer()
{
	QTreeWidgetItem *peerSelect = getCurrentPeer();
	if (!peerSelect)
		return;

	std::string peerId = (peerSelect->text(GAME_PEER_ID)).toStdString();
	rsGameLauncher->confirmPeer(mCurrentGame, peerId);
	updateGameDetails();
}


void GamesDialog::unconfirmPeer()
{
	QTreeWidgetItem *peerSelect = getCurrentPeer();
	if (!peerSelect)
		return;

	std::string peerId = (peerSelect->text(GAME_PEER_ID)).toStdString();
	rsGameLauncher->unconfirmPeer(mCurrentGame, peerId);
	updateGameDetails();
}


void GamesDialog::interested()
{
	QTreeWidgetItem *gameSelect = getCurrentGame();
	if (!gameSelect)
		return;

	std::string gameId = (gameSelect->text(GAME_LIST_ID)).toStdString();
	rsGameLauncher->interestedPeer(gameId);
	updateGameDetails();
}


void GamesDialog::uninterested()
{
	QTreeWidgetItem *gameSelect = getCurrentGame();
	if (!gameSelect)
		return;

	std::string gameId = (gameSelect->text(GAME_LIST_ID)).toStdString();
	rsGameLauncher->uninterestedPeer(gameId);
	updateGameDetails();
}


