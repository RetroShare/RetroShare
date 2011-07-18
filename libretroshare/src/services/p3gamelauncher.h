/*
 * libretroshare/src/services: p3gamelauncher.h
 *
 * Services for RetroShare.
 *
 * Copyright 2004-2008 by Robert Fernie.
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


#ifndef SERVICE_GAME_LAUNCHER_HEADER
#define SERVICE_GAME_LAUNCHER_HEADER

/* 
 * A central point to setup games between peers.
 *
 */

#include <list>
#include <string>

#include "services/p3service.h"
#include "serialiser/rsgameitems.h"
#include "retroshare/rsgame.h"

class p3LinkMgr;


class gameAvail
{
	uint32_t    serviceId;
	std::string gameName;
	uint16_t minPlayers;
	uint16_t maxPlayers;
};

class gameStatus
{
	public:

	uint32_t serviceId;
	std::string gameId;
	std::wstring gameName;

	bool areServer;  	/* are we the server? */
	std::string serverId; 	/* if not, who is? */

	uint16_t numPlayers;
	std::list<std::string> allowedPeers; /* who can play ( controlled by server) */
	std::list<std::string> interestedPeers; /* who wants to play ( controlled by server) */
	std::list<std::string> peerIds; /* in order of turns */

	uint32_t state;
};

class p3GameService;

/* We're going to add the external Interface - directly on here! */

class p3GameLauncher: public p3Service, public RsGameLauncher
{
	public:
	p3GameLauncher(p3LinkMgr *lm);

	/***** EXTERNAL RsGameLauncher Interface *******/
/* server commands */
virtual std::string createGame(uint32_t gameType, std::wstring name);
virtual bool    deleteGame(std::string gameId);
virtual bool    inviteGame(std::string gameId);
virtual bool    playGame(std::string gameId);
//virtual bool    quitGame(std::string gameId);

virtual bool    invitePeer(std::string gameId, std::string peerId);
virtual bool    uninvitePeer(std::string gameId, std::string peerId);
virtual bool    confirmPeer(std::string gameId, std::string peerId, 
						int16_t pos = -1);
virtual bool    unconfirmPeer(std::string gameId, std::string peerId);

/* client commands */
virtual bool    interestedPeer(std::string gameId);
virtual bool    uninterestedPeer(std::string gameId);

/* get details */
virtual bool    getGameList(std::list<RsGameInfo> &gameList);
virtual bool    getGameDetail(std::string gameId, RsGameDetail &detail);
	/***** EXTERNAL RsGameLauncher Interface *******/

	/* support functions */
	private:
std::string  	newGame(uint16_t srvId, std::wstring name);
bool    	confirmGame(std::string gameId);
bool    	quitGame(std::string gameId);
bool 		inviteResponse(std::string gameId, bool interested);


	/* p3Service Overloaded */
virtual int   tick();
virtual int   status();

	/* add in the Game */
int	addGameService(p3GameService *game);
	/* notify gameService/peers */

//int	getGameList(std::list<gameAvail> &games);
//int	getGamesCurrent(std::list<std::string> &games);
//int	getGameDetails(std::string gid, gameStatus &status);

	/**** GUI     Interface ****/

bool	resumeGame(std::string gameId);

	/**** Network Interface ****/
int 	checkIncoming();
int 	handleIncoming(RsGameItem *gi);

int	handleClientStart(RsGameItem *gi);    /* START msg */
int	handleClientInvited(RsGameItem *gi);  /* REJECT msg */
int	handleClientReady(RsGameItem *gi);    /* CONFIRM / REJECT / PLAY msg */
int	handleClientActive(RsGameItem *gi);   /* PAUSE / QUIT msg */

int	handleServerSetup(RsGameItem *gi);    /* INTERESTED / REJECT msg */
int	handleServerActive(RsGameItem *gi);   /* PAUSE / QUIT msg */

int	sendRejectMsg(RsGameItem *gi);	      /* --- error msg */
void	cleanupGame(std::string gameId);      /* remove from list */
bool 	checkGameProperties(uint16_t serviceId, uint16_t players);

std::map<uint16_t, p3GameService *> gameList;
std::map<std::string, gameStatus> gamesCurrent;

	p3LinkMgr *mLinkMgr;
	std::string mOwnId;
};

#endif // SERVICE_GAME_LAUNCHER_HEADER
