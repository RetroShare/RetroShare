/*
 * libretroshare/src/services: p3gamelauncher.cc
 *
 * Other Bits for RetroShare.
 *
 * Copyright 2007-2008 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include "services/p3gamelauncher.h"
#include "services/p3gameservice.h"
#include "pqi/pqidebug.h"
#include <sstream>

/* global variable for GUI */
RsGameLauncher *rsGameLauncher = NULL;


/* So STATEs is always the best way to do things....
 *
 * CLIENT:
 * -------
 * state:         message          	result
 * NULL      
 *      <------- START     -----   	to INVITED state
 *      <------- RESUME    -----   	to INVITED(resume) etc...
 *
 * INVITED  
 * 	------ INTERESTED ----->	to READY state
 * 	------ REJECT     ----->	to NULL state
 *
 * READY 
 *      <------- CONFIRMED -----        to/stay READY (update)
 *  	<------- REJECTED  -----        to NULL (abort)
 *  	<------- PLAY      -----        to ACTIVE
 *
 * ----
 *  below here the game s setup (game / players / order / etc is defined)
 *
 * ACTIVE (game play happens).
 *      <------ PAUSE     ----->    	to  NULL
 *      <------ disconnect ---->     	to  NULL
 *      <------ QUIT      ----->    	to  NULL
 *
 ********************************************************
 *
 * SERVER:
 * -------
 * state:         message          	result
 * NULL      
 * 	------ START      ----->	to SETUP state
 *
 * SETUP  
 * 	<----- INTERESTED ------	to SETUP state
 * 	<----- REJECT     ------	to SETUP state
 *      ------ CONFIRMED  ----->        to SETUP state
 *  	------ REJECTED   ----->        to SETUP state
 *  	------ PLAY       ----->        to ACTIVE state
 *
 * ----
 *  below here the game s setup (game / players / order / etc is defined)
 *
 * ACTIVE (game play happens).
 *      <------- PAUSE     -----     	to  NULL
 *      ------- disconnect -----     	to  NULL
 *      <------- QUIT      -----     	to  NULL
 */

/* Game Setup States ****/
const uint32_t RS_GAME_INIT_INVITED = 1;  /* Client */
const uint32_t RS_GAME_INIT_READY   = 2;  /* Client */
const uint32_t RS_GAME_INIT_SETUP   = 3;  /* Server */
const uint32_t RS_GAME_INIT_CONFIRM = 4;  /* Server */
const uint32_t RS_GAME_INIT_ACTIVE  = 5;  /* Client/Server */


/* Game Setup Messages ****/
const uint32_t RS_GAME_MSG_START      = 0;  /* SERVER->CLIENT */
const uint32_t RS_GAME_MSG_RESUME     = 1;  /* SERVER->CLIENT * TODO later */
const uint32_t RS_GAME_MSG_INTERESTED = 2;  /* CLIENT->SERVER */
const uint32_t RS_GAME_MSG_CONFIRM    = 3;  /* SERVER->CLIENT */
const uint32_t RS_GAME_MSG_PLAY       = 4;  /* SERVER->CLIENT */
const uint32_t RS_GAME_MSG_PAUSE      = 5;  /*  ANY  -> ANY   */
const uint32_t RS_GAME_MSG_QUIT       = 6;  /*  ANY  -> ANY   */
const uint32_t RS_GAME_MSG_REJECT     = 6;  /*  ANY  -> ANY   */

const int p3gamezone = 1745;

p3GameLauncher::p3GameLauncher()
	:p3Service(RS_SERVICE_TYPE_GAME_LAUNCHER) 
{
	//addSerialType(new RsGameSerialiser());
	mOwnId = "mOwnId";
}

int	p3GameLauncher::tick()
{
	pqioutput(PQL_DEBUG_BASIC, p3gamezone, 
		"p3GameLauncher::tick()");
	return 0;
}

int	p3GameLauncher::status()
{
	pqioutput(PQL_DEBUG_BASIC, p3gamezone, 
		"p3GameLauncher::status()");
	return 1;
}


/**** Interface to GUI Game Launcher ****/


std::string generateRandomGameId()
{
	std::ostringstream out;
	out << std::hex;

	/* 4 bytes per random number: 4 x 4 = 16 bytes */
	for(int i = 0; i < 4; i++)
	{
		uint32_t rint = random();
		out << rint;
	}
	return out.str();
}

/**** GUI     Interface ****/


bool 	p3GameLauncher::resumeGame(std::string)
{
	/* get game details from p3gameService */
	/* add to status reports */
	/* send resume invites to peers in list */

	return ""; /* TODO */
}


/******************************************************************/
/******************************************************************/
        /***** EXTERNAL RsGameLauncher Interface *******/
/******************************************************************/
/******************************************************************/

	/* Server commands */

std::string p3GameLauncher::createGame(uint32_t gameType, std::string name)
{
	/* translate Id */
	uint16_t srvId = gameType;

	return newGame(srvId, name);
}

std::string  p3GameLauncher::newGame(uint16_t srvId, std::string name)
{
	/* generate GameId (random string) */
	std::string gameId = generateRandomGameId();

	gameStatus newGame;
	newGame.gameId = gameId;
	newGame.serviceId = srvId;
	newGame.numPlayers = 0;
	newGame.gameName = name;

	newGame.areServer = true;
	newGame.serverId = mOwnId; 
	newGame.allowedPeers.push_back(mOwnId);
	newGame.interestedPeers.push_back(mOwnId);
	newGame.peerIds.push_back(mOwnId);

	newGame.interestedPeers.clear();
	newGame.peerIds.clear();
	newGame.state = RS_GAME_INIT_SETUP;  /* Server Only */

	/* send messages to peers inviting to game */
	std::list<std::string>::const_iterator it;

	/* store gameStatus in list */
	gamesCurrent[gameId] = newGame;

	/* return new game Id */
	return gameId;
}

void  p3GameLauncher::cleanupGame(std::string gameId)
{
	deleteGame(gameId);
}

bool  p3GameLauncher::deleteGame(std::string gameId)
{
	std::map<std::string, gameStatus>::iterator git;
	git = gamesCurrent.find(gameId);
	if (git == gamesCurrent.end())
	{
		return false;
	}

	if (git->second.state != RS_GAME_INIT_SETUP)
	{
		/* send off quit messages */
		quitGame(gameId);
	}

	gamesCurrent.erase(git);

	return true;
}

bool  p3GameLauncher::inviteGame(std::string gameId)
{
	std::map<std::string, gameStatus>::iterator git;
	git = gamesCurrent.find(gameId);
	if (git == gamesCurrent.end())
	{
		return false;
	}

	if (git->second.state != RS_GAME_INIT_SETUP)
	{
		return false;
	}

	/* send messages to peers inviting to game */
	std::list<std::string>::const_iterator it;

	for(it = git->second.allowedPeers.begin(); 
		it != git->second.allowedPeers.end(); it++)
	{
		/* for an invite we need:
		 * serviceId, gameId, numPlayers....
		 */

		RsGameItem  *rgi = new RsGameItem();
		rgi->serviceId  = git->second.serviceId;
		rgi->gameId     = git->second.gameId;
		rgi->numPlayers = git->second.numPlayers;
		rgi->players    = git->second.allowedPeers;

		rgi->msg = RS_GAME_MSG_START;
		/* destination */
		rgi->PeerId(*it);

		/* send Msg */
		sendItem(rgi);
	}

	git->second.state = RS_GAME_INIT_CONFIRM;

	return true;
}


/* support Game */
bool    p3GameLauncher::confirmGame(std::string game)
{
	
	std::map<std::string, gameStatus>::iterator git;

	git = gamesCurrent.find(game);
	if (git == gamesCurrent.end())
	{
		return false;
	}

	/* send messages to peers inviting to game */
	std::list<std::string>::const_iterator it;

	for(it = git->second.peerIds.begin(); 
		it != git->second.peerIds.end(); it++)
	{
		/* for an invite we need:
		 * serviceId, gameId, numPlayers....
		 */

		RsGameItem  *rgi = new RsGameItem();
		rgi->serviceId  = git->second.serviceId;
		rgi->gameId     = git->second.gameId;
		rgi->numPlayers = git->second.numPlayers;
		rgi->players    = git->second.peerIds;

		rgi->msg = RS_GAME_MSG_CONFIRM;

		/* destination */
		rgi->PeerId(*it);

		/* send Msg */
		sendItem(rgi);
	}

	return true;
}


bool    p3GameLauncher::playGame(std::string gameId)
{
	std::map<std::string, gameStatus>::iterator git;

	git = gamesCurrent.find(gameId);
	if (git == gamesCurrent.end())
	{
		return false;
	}

	if (git->second.state != RS_GAME_INIT_CONFIRM)
	{
		return false;
	}

	/* send messages to peers inviting to game */
	std::list<std::string>::const_iterator it;

	for(it = git->second.peerIds.begin(); 
		it != git->second.peerIds.end(); it++)
	{
		/* for an invite we need:
		 * serviceId, gameId, numPlayers....
		 */

		RsGameItem  *rgi = new RsGameItem();
		rgi->serviceId  = git->second.serviceId;
		rgi->gameId     = git->second.gameId;
		rgi->numPlayers = git->second.numPlayers;
		rgi->players    = git->second.peerIds;

		rgi->msg = RS_GAME_MSG_PLAY;

		/* destination */
		rgi->PeerId(*it);

		/* send Msg */
		sendItem(rgi);
	}

	/* inform all the other peers that we've started the game */
	for(it = git->second.interestedPeers.begin(); 
		it != git->second.interestedPeers.end(); it++)
	{
		if (git->second.peerIds.end() == (std::find(git->second.peerIds.begin(),
				git->second.peerIds.end(), *it)))
		{
			/* tell the them they're not needed */

			RsGameItem  *rgi = new RsGameItem();
			rgi->serviceId  = git->second.serviceId;
			rgi->gameId     = git->second.gameId;
			rgi->numPlayers = 0;
			rgi->players.clear();

			rgi->msg = RS_GAME_MSG_REJECT;

			/* destination */
			rgi->PeerId(*it);

			/* send Msg */
			sendItem(rgi);
		}
	}

	/* Finally start the actual Game */

	/* TODO */


	return true;

}

bool    p3GameLauncher::quitGame(std::string gameId)
{
	/* TODO */

	return false;
}



bool    p3GameLauncher::invitePeer(std::string gameId, std::string peerId)
{
	std::map<std::string, gameStatus>::iterator git;
	git = gamesCurrent.find(gameId);
	if (git == gamesCurrent.end())
	{
		return false;
	}

	if (git->second.state != RS_GAME_INIT_SETUP)
	{
		return false;
	}

	if (peerId == mOwnId)
	{
		return false;
	}

	/* send messages to peers inviting to game */
	std::list<std::string>::const_iterator it;
	if (git->second.allowedPeers.end() != 
		(it = std::find(git->second.allowedPeers.begin(), 
			 git->second.allowedPeers.end(), peerId)))
	{
		return true;
	}

	git->second.allowedPeers.push_back(peerId);
	return true;
}


/* Server GUI commands */
bool    p3GameLauncher::uninvitePeer(std::string gameId, std::string peerId)
{
	std::map<std::string, gameStatus>::iterator git;
	git = gamesCurrent.find(gameId);
	if (git == gamesCurrent.end())
	{
		return false;
	}

	if (git->second.state != RS_GAME_INIT_SETUP)
	{
		return false;
	}

	if (peerId == mOwnId)
	{
		return false;
	}

	/* send messages to peers inviting to game */
	std::list<std::string>::iterator it;
	if (git->second.allowedPeers.end() == 
		(it = std::find(git->second.allowedPeers.begin(), 
			 git->second.allowedPeers.end(), peerId)))
	{
		return true;
	}

	git->second.allowedPeers.erase(it);
	return true;
}


bool    p3GameLauncher::confirmPeer(std::string game, std::string peerId, 
							int16_t pos)
{
	std::map<std::string, gameStatus>::iterator git;

	git = gamesCurrent.find(game);
	if (git == gamesCurrent.end())
	{
		return false;
	}

	if (git->second.state != RS_GAME_INIT_CONFIRM)
	{
		return false;
	}

	std::list<std::string>::iterator it;
	if (git->second.interestedPeers.end() == 
		(it = std::find(git->second.interestedPeers.begin(), 
			 git->second.interestedPeers.end(), peerId)))
	{
		return false;
	}

	it = std::find(git->second.peerIds.begin(), 
			 git->second.peerIds.end(), peerId);
	if (it != git->second.peerIds.end())
	{
		git->second.peerIds.erase(it);
	}

	int32_t i = 0;
	for(it = git->second.peerIds.begin(); (i < pos) &&
		(it != git->second.peerIds.end()); it++, i++);

	if ((pos < 0) || (it == git->second.peerIds.end()))
	{
		/*   */
		git->second.peerIds.push_back(peerId);
	}
		
	git->second.peerIds.insert(it, peerId);
	return confirmGame(game);
}


bool    p3GameLauncher::unconfirmPeer(std::string game, std::string peerId)
{
	std::map<std::string, gameStatus>::iterator git;

	git = gamesCurrent.find(game);
	if (git == gamesCurrent.end())
	{
		return false;
	}

	if (git->second.state != RS_GAME_INIT_CONFIRM)
	{
		return false;
	}

	std::list<std::string>::iterator it;

	it = std::find(git->second.peerIds.begin(), 
			 git->second.peerIds.end(), peerId);
	if (it != git->second.peerIds.end())
	{
		git->second.peerIds.erase(it);
	}
	return confirmGame(game);
}


/* Client GUI Commands */
bool    p3GameLauncher::interestedPeer(std::string gameId)
{
	return inviteResponse(gameId, true);
}

bool    p3GameLauncher::uninterestedPeer(std::string gameId)
{
	return inviteResponse(gameId, false);
}

bool    p3GameLauncher::inviteResponse(std::string gameId, bool interested)
{
	std::map<std::string, gameStatus>::iterator git;

	git = gamesCurrent.find(gameId);
	if (git == gamesCurrent.end())
	{
		return 0;
	}

	/* TODO */

	RsGameItem  *rgi = new RsGameItem();
	rgi->serviceId  = git->second.serviceId;
	rgi->gameId     = git->second.gameId;
	rgi->numPlayers = 0;
	rgi->players.clear();

	if (interested)
	{
		rgi->msg = RS_GAME_MSG_INTERESTED;
	}
	else
	{
		rgi->msg = RS_GAME_MSG_REJECT;
	}

	/* destination */
	rgi->PeerId(git->second.serverId);

	/* send Msg */
	sendItem(rgi);

	return 1;
}


/***** Details ****/

/* get details */
bool    p3GameLauncher::getGameList(std::list<RsGameInfo> &gameList)
{
	std::map<std::string, gameStatus>::iterator git;
	for(git = gamesCurrent.begin(); git != gamesCurrent.end(); git++)
	{
		RsGameInfo info;

		info.gameId = git->first;
		info.serverId = git->second.serverId;

		std::ostringstream out;
		out << "GameType: " << git->second.serviceId;
		info.gameType = out.str();

		info.serverName = "ServerName";
		info.numPlayers = git->second.numPlayers;
		info.gameName =   git->second.gameName;

		if ((git->second.state == RS_GAME_INIT_SETUP) ||
			(git->second.state == RS_GAME_INIT_INVITED))
		{
			info.status = "Invite";
		}
		else if ((git->second.state == RS_GAME_INIT_CONFIRM) ||
			(git->second.state == RS_GAME_INIT_READY))
		{
			info.status = "Confirm";
		}
		else if (git->second.state == RS_GAME_INIT_ACTIVE)
		{
			info.status = "Playing";
		}
		else
		{
			info.status = "Unknown";
		}

		gameList.push_back(info);
	}
	return true;
}

bool    p3GameLauncher::getGameDetail(std::string gameId, RsGameDetail &detail)
{
	std::map<std::string, gameStatus>::iterator git;
	std::list<std::string>::iterator it;
	git = gamesCurrent.find(gameId);
	if (git == gamesCurrent.end())
	{
		return false;
	}

	/* fill in the details */
	detail.gameId = gameId;
	detail.gameType = git->second.serviceId;
	detail.gameName =   git->second.gameName;
	detail.areServer = git->second.areServer;
	detail.serverId =  git->second.serverId;
	detail.serverName = "Server???";
	detail.numPlayers = git->second.numPlayers;

	if ((git->second.state == RS_GAME_INIT_SETUP) ||
		(git->second.state == RS_GAME_INIT_INVITED))
	{
		detail.status = "Invite";

		/* copy from invited List */
		for(it = git->second.allowedPeers.begin();
			it != git->second.allowedPeers.end(); it++)
		{
			RsGamePeer rgp;
			rgp.name = "PEER???";
			rgp.invite = true;
			rgp.interested = false;
			rgp.play = false;

			detail.gamers[*it] = rgp;
		}
	}
	else if ((git->second.state == RS_GAME_INIT_CONFIRM) ||
			(git->second.state == RS_GAME_INIT_READY))
	{
		detail.status = "Confirm";

		/* copy from invited List */
		for(it = git->second.allowedPeers.begin();
			it != git->second.allowedPeers.end(); it++)
		{
			RsGamePeer rgp;
			rgp.name = "PEER???";
			rgp.invite = true;

			if (git->second.interestedPeers.end() !=
				std::find(git->second.interestedPeers.begin(), 
			 		git->second.interestedPeers.end(), 
					git->first))
			{
				rgp.interested = true;
			}
			else
			{
				rgp.interested = false;
			}
			/* if in peerIds */
			if (git->second.peerIds.end() !=
				std::find(git->second.peerIds.begin(), 
			 		git->second.peerIds.end(), 
					git->first))
			{
				rgp.play = true;
			}
			else
			{
				rgp.play = false;
			}
			detail.gamers[*it] = rgp;
		}


	}
	else if (git->second.state == RS_GAME_INIT_ACTIVE)
	{
		detail.status = "Playing";

		/* copy from invited List */
		for(it = git->second.peerIds.begin();
			it != git->second.peerIds.end(); it++)
		{
			RsGamePeer rgp;
			rgp.name = "PEER???";
			rgp.invite = true;
			rgp.interested = true;
			rgp.play = true;

			detail.gamers[*it] = rgp;
		}
	}
	else
	{
		return false;
	}
	return true;
}


/**** Network interface ****/

int p3GameLauncher::checkIncoming()
{
	/* check for incoming items */

	RsGameItem *gi = NULL;

	while(NULL != (gi = (RsGameItem *) recvItem()))
	{
		handleIncoming(gi);
		delete gi;
	}
	return 1;
}


int p3GameLauncher::handleIncoming(RsGameItem *gi)
{
	/* check that its a valid packet...
	 * and that there is a gameStatus.
	 */

	/* Always check the Properties */
	if (!checkGameProperties(gi->serviceId, gi->numPlayers))
	{
		sendRejectMsg(gi);
		return 0;
	}

	/* check if there is an existing game? */
	std::map<std::string, gameStatus>::iterator it;
	bool haveStatus = (gamesCurrent.end() != 
		(it = gamesCurrent.find(gi->gameId)));

	/* handle startup first */
	if (!haveStatus)
	{
		if (gi->msg == RS_GAME_MSG_START)
		{
			/***** CLIENT HANDLING ****/
			/* we are the client -> start it up! */
			return handleClientStart(gi);
		}
		sendRejectMsg(gi);
		return 0;
	}

	/* have a current status - if we get here 
	 * switch on 
	 * 	1) server/client.
	 * 	2) state, 
	 * 	3) msg.
	 */

	if (it->second.areServer)
	{
		/***** SERVER HANDLING ****/
		switch(it->second.state)
		{
			case RS_GAME_INIT_SETUP:
				/* only accept INTERESTED | REJECT */
				if ((gi->msg == RS_GAME_MSG_INTERESTED) ||
				    (gi->msg == RS_GAME_MSG_REJECT))
				{
					handleServerSetup(gi);
					return 1;
				}
				else
				{
					sendRejectMsg(gi);
					return 0;
				}
				break;

			case RS_GAME_INIT_ACTIVE:
				if ((gi->msg == RS_GAME_MSG_PAUSE) ||
				    (gi->msg == RS_GAME_MSG_QUIT))
				{
					handleServerActive(gi);
					return 1;
				}
				else
				{
					sendRejectMsg(gi);
					return 0;
				}
				break;

			case RS_GAME_INIT_INVITED:  /* invalid state */
			case RS_GAME_INIT_READY:    /* invalid state */
			default:
				sendRejectMsg(gi);
				return 0;
				break;
		}
	}
	else
	{
		/***** CLIENT HANDLING ****/
		switch(it->second.state)
		{
			case RS_GAME_INIT_INVITED:
				/* only accept REJECT */
				if (gi->msg == RS_GAME_MSG_REJECT)
				{
					handleClientInvited(gi);
					return 1;
				}
				else
				{
					sendRejectMsg(gi);
					return 0;
				}
				break;

			case RS_GAME_INIT_READY:

				if ((gi->msg == RS_GAME_MSG_CONFIRM) ||
				    (gi->msg == RS_GAME_MSG_REJECT)  ||
				    (gi->msg == RS_GAME_MSG_PLAY))
				{
					handleClientReady(gi);
					return 1;
				}
				else
				{
					sendRejectMsg(gi);
					return 0;
				}
				break;

			case RS_GAME_INIT_ACTIVE:
				if ((gi->msg == RS_GAME_MSG_PAUSE) ||
				    (gi->msg == RS_GAME_MSG_QUIT))
				{
					handleClientActive(gi);
					return 1;
				}
				else
				{
					sendRejectMsg(gi);
					return 0;
				}
				break;


			case RS_GAME_INIT_SETUP:    /* invalid state */
			default:
				sendRejectMsg(gi);
				return 0;
				break;
		}
	}

	/* should never get here */
	sendRejectMsg(gi);
	return 0;
}



/***** Network Msg Functions *****
 *
 *
 *
	handleClientStart(gi)     * START msg *
	handleClientInvited(gi);  * REJECT msg *
	handleClientReady(gi);    * CONFIRM / REJECT / PLAY msg *
	handleClientActive(gi);   * PAUSE / QUIT msg *

	handleServerSetup(gi);    * INTERESTED / REJECT msg *
	handleServerActive(gi);   * PAUSE / QUIT msg *
	sendRejectMsg(gi);
 *
 *
 *
 */

	/* START msg */
int p3GameLauncher::handleClientStart(RsGameItem *gi)   
{
	/* Already checked existance / Properties */

	/* else -> add into the list of games */
	gameStatus gs;
	gs.serviceId  = gi->serviceId;
	gs.gameId     = gi->gameId;
	gs.areServer  = false;
	gs.serverId   = gi->PeerId();
	gs.state = RS_GAME_INIT_INVITED;  /* Client */
	gs.numPlayers = gi->numPlayers;
	gs.peerIds    = gi->players;

	gamesCurrent[gi->gameId] = gs;

	return 1;
}

	/* REJECT msg */
int p3GameLauncher::handleClientInvited(RsGameItem *gi)   
{
	/* cleanup game */
	cleanupGame(gi->gameId);
	return 1;
}

	/* CONFIRM / REJECT / PLAY msg */
int p3GameLauncher::handleClientReady(RsGameItem *gi)   
{
	/* get game */
	std::map<std::string, gameStatus>::iterator it;
	if (gamesCurrent.end() == (it = gamesCurrent.find(gi->gameId)))
	{
		/* no game exists */
		return 0;
	}

	switch(gi->msg)
	{
	  case RS_GAME_MSG_CONFIRM:

		/* update the information 
		 * (other info should be the same)
		 */
		it->second.numPlayers = gi->numPlayers;
		it->second.peerIds    = gi->players;

	  	return 1;
	  	break;

	  case RS_GAME_MSG_REJECT:
	  	cleanupGame(gi->gameId);
		return 1;
		break;

	  case RS_GAME_MSG_PLAY:
	  	/* TODO */
		return 1;
		break;
	  default:
	  	break;
	}
	return 0;
}

	/* PAUSE / QUIT msg */
int p3GameLauncher::handleClientActive(RsGameItem *gi)   
{


	return 1;
}


	/* INTERESTED / REJECT msg */
int p3GameLauncher::handleServerSetup(RsGameItem *gi)
{
	/* check if there is an existing game? */
	std::map<std::string, gameStatus>::iterator it;
	if (gamesCurrent.end() == (it = gamesCurrent.find(gi->gameId)))
	{
		/* no game exists */
		return 0;
	}

	/* we only care about this notice -> if we're the server */
	if (it->second.areServer)
	{
		std::list<std::string>::iterator it2, it3;
		it2 = std::find(it->second.allowedPeers.begin(),
				it->second.allowedPeers.end(), gi->PeerId());
		it3 = std::find(it->second.interestedPeers.begin(), 
				it->second.interestedPeers.end(), gi->PeerId());

		if ((it2 != it->second.allowedPeers.end()) &&
		    (it3 == it->second.interestedPeers.end()))
		{
			it->second.interestedPeers.push_back(gi->PeerId());
			return 1;
		}
	}
	return 0;
}

/* This is a setup update from server
 * only updates the players...
 */
int p3GameLauncher::handleServerActive(RsGameItem *gi)   
{

	return 1;
}

int	p3GameLauncher::sendRejectMsg(RsGameItem *gi)
{
	/* all should be okay ... except msg */
	gi->msg = RS_GAME_MSG_REJECT;
	sendItem(gi);
	return 1;
}

bool p3GameLauncher::checkGameProperties(uint16_t serviceId, uint16_t players)
{
	std::map<uint16_t, p3GameService *>::iterator it;
	if (gameList.end() == (it = gameList.find(serviceId)))
	{
		return false; /* we don't support the game */
	}

	if ((players <= it->second->getMaxPlayers()) && 
		(players >= it->second->getMinPlayers()))
	{
		return true;
	}
	return false;
}

