/*
    QSoloCards is a collection of Solitaire card games written using Qt
    Copyright (C) 2009  Steve Moore

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GAMEMGR_H
#define GAMEMGR_H

#include "GameBoard.h"
#include <QtGui/QMenu>
#include <QtCore/QStringList>

class GameMgr
{
public:
    enum GameId
    {
        Spider=0,
        Klondike=1,
	FreeCell=2,
	ThreeDeckSpider=3,
	Spiderette=4,
	Yukon=5,
        NoGame=6,
        DefaultGame=Spider
    };
    GameMgr();
    virtual ~GameMgr();

    // Add menu items for the games.  A menu item will be created with the data object set
    // as an integer with the GameId value.  The caller can connect to the menus triggered
    // signal and then get the data object from the QAction and call getGame with the id
    // to create a game board.
    void buildGameListMenu(QMenu & menu) const;

    // get a game based on the gameId.  The returned GameBoard ptr
    // is the responsibility of the caller to delete.  If the game id is not valid
    // NULL is returned.
    GameBoard * getGame(GameId gameId=DefaultGame);

    GameId getGameId() const {return m_lastGameId;}

private:
    GameId        m_lastGameId;   // the game id of the game requested at the last call to
                                  // getGame.  The default is NoGame.
    QStringList   m_gameDspNameList;
};

#endif // GAMEMGR_H
