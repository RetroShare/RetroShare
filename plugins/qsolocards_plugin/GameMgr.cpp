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

#include "GameMgr.h"
#include "SpiderBoard.h"
#include "Spider3DeckBoard.h"
#include "SpideretteBoard.h"
#include "KlondikeBoard.h"
#include "FreeCellBoard.h"
#include "YukonBoard.h"
#include <QtGui/QActionGroup>

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
GameMgr::GameMgr()
    :m_lastGameId(GameMgr::NoGame)
{
    // build the list of games
    m_gameDspNameList<<QObject::tr("Spider").trimmed()
		     <<QObject::tr("Klondike").trimmed()
		     <<QObject::tr("Freecell").trimmed()
		     <<QObject::tr("Three Deck Spider").trimmed()
		     <<QObject::tr("Spiderette").trimmed()
		     <<QObject::tr("Yukon").trimmed();
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
GameMgr::~GameMgr()
{
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
void GameMgr::buildGameListMenu(QMenu & menu) const
{
    QActionGroup * pGameGroup=new QActionGroup(&menu);

    for (int i=0;i<GameMgr::NoGame;i++)
    {
        QAction * pCurrAction=new QAction(this->m_gameDspNameList.at(i),pGameGroup);
        pCurrAction->setData(QVariant(i));
	pCurrAction->setCheckable(true);
	if (i==this->m_lastGameId)
	{
	    pCurrAction->setChecked(true);
	}
        menu.addAction(pCurrAction);
    }
}

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
GameBoard * GameMgr::getGame(GameId gameId)
{
    GameBoard * pGame=NULL;

    switch(gameId)
    {
    case GameMgr::Spider:
	pGame=new SpiderBoard(NULL);
        break;
    case GameMgr::Klondike:
	pGame=new KlondikeBoard(NULL);
        break;
    case GameMgr::FreeCell:
	pGame=new FreeCellBoard;
        break;
    case GameMgr::ThreeDeckSpider:
	pGame=new Spider3DeckBoard;
        break;
    case GameMgr::Spiderette:
	pGame=new SpideretteBoard;
        break;
    case GameMgr::Yukon:
	pGame=new YukonBoard;
        break;
    case GameMgr::NoGame:
    default:
	pGame=NULL;
        break;
    };

    // if we were able to create a valid game board.  Set the last
    // game id
    if (NULL!=pGame)
    {
        this->m_lastGameId=gameId;
    }

    return pGame;
}
