/***************************************************************************
 *   Copyright (C) 2004-2005 Artur Wiebe                                   *
 *   wibix@gmx.de                                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _HUMANPLAYER_H_
#define _HUMANPLAYER_H_

#include "player.h"

#include <qobject.h>


class myHumanPlayer : public myPlayer
{
    Q_OBJECT

public:
    // when playing player vs. player on same computer. the second
    // player must invert some things.
    myHumanPlayer(const QString& name, bool white, bool second_player);
    ~myHumanPlayer();

    virtual void yourTurn(const Checkers* game);
    virtual bool fieldClicked(int fieldnumber, bool*, QString& err_msg);
    virtual void stop() {}

    virtual bool isHuman() const { return true; }


public slots:
//    virtual void getReady() { emit readyToPlay(); }

private:
    bool go(int fieldnumber);

private:
    bool m_second;

    Checkers* m_game;
    bool selected;

    int from;		// on Checkers board
    int fromField;	// on GUI board
};


#endif

