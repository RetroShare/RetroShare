#ifndef RS_GAME_GUI_INTERFACE_H
#define RS_GAME_GUI_INTERFACE_H

/*
 * libretroshare/src/rsiface: rsgame.h
 *
 * RetroShare C++ Interface.
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


#include "rstypes.h"


class RsGameLauncher;

/* declare single RsIface for everyone to use! */

extern RsGameLauncher  *rsGameLauncher;
	
#include <map>
#include <string>
#include <inttypes.h>

class RsGameInfo
{
	public:
	
	std::string gameId;
	std::string serverId;
	
	std::string gameType;
	std::string gameName;
	std::string serverName;
	std::string status;
	uint16_t    numPlayers;
	
};
	
class RsGamePeer
{
	public:
	std::string id;
	bool invite;
	bool interested;
	bool play;
};
	
class RsGameDetail
{
	public:
	std::string gameId;
	std::string gameType;
	std::string gameName;
	
	bool areServer;         /* are we the server? */
	std::string serverId;   /* if not, who is? */
	std::string serverName;
	
	std::string status;
	
	uint16_t numPlayers;
	std::map<std::string, RsGamePeer> gamers;
	
};

class RsGameLauncher
{
        public:

/* server commands */
virtual std::string createGame(uint32_t gameType, std::string name) = 0;
virtual bool    deleteGame(std::string gameId) = 0;
virtual bool    inviteGame(std::string gameId) = 0;
virtual bool    playGame(std::string gameId) = 0;
//virtual bool    quitGame(std::string gameId) = 0;

virtual bool    invitePeer(std::string gameId, std::string peerId) = 0;
virtual bool    uninvitePeer(std::string gameId, std::string peerId) = 0;
virtual bool    confirmPeer(std::string gameId, std::string peerId,
                                                int16_t pos = -1) = 0;
virtual bool    unconfirmPeer(std::string gameId, std::string peerId) = 0;

/* client commands */
virtual bool    interestedPeer(std::string gameId) = 0;
virtual bool    uninterestedPeer(std::string gameId) = 0;

/* get details */
virtual bool    getGameList(std::list<RsGameInfo> &gameList) = 0;
virtual bool    getGameDetail(std::string gameId, RsGameDetail &detail) = 0;

};


#endif
