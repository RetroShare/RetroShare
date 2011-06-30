/*
 * libretroshare/src/services: p3gameservice.h
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


#ifndef P3_SERVICE_GAME_HEADER
#define P3_SERVICE_GAME_HEADER

/* 
 * A central point to setup games between peers.
 *
 */

#include <list>
#include <string>

#include "services/p3service.h"

class StoredGame
{
	public:
	std::string gameId;
	time_t      startTime;
	std::list<std::string> peerIds; /* in order of turns */
};

class p3GameLauncher;
class p3Service;

class p3GameService
{
	public:

	p3GameService(uint16_t sId, std::string name, uint16_t min, uint16_t max, p3GameLauncher *l)
	:serviceId(sId), gameName(name), minPlayers(min), maxPlayers(max), 
	service(NULL), launcher(l)
	{ return; }

virtual ~p3GameService() 
	{ return; }

	/*************** Game Interface ******************/
	/* saved games */
virtual	void getSavedGames(std::list<StoredGame> &gList);

	/* start a game */
virtual	void startGame(StoredGame &newGame, bool resume);
virtual	void quitGame(std::string gameId);
virtual	void deleteGame(std::string gameId);
	/*************** Game Interface ******************/

	/* details for the Launcher */
	uint16_t getServiceId() 	{ return serviceId;  }
	std::string getGameName() 	{ return gameName;   }
	uint16_t getMinPlayers()	{ return minPlayers; }
	uint16_t getMaxPlayers()	{ return maxPlayers; }
	p3GameLauncher *getLauncher()   { return launcher;   }

	private:

	uint16_t serviceId;
	std::string gameName;
	uint16_t minPlayers, maxPlayers;

	p3Service      *service;
	p3GameLauncher *launcher;
};

#endif // P3_SERVICE_GAME_HEADER
