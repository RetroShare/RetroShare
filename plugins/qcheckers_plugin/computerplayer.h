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

#ifndef _COMPUTERPLAYER_H_
#define _COMPUTERPLAYER_H_


#include <QThread>

#include "player.h"


class myThread;


class myComputerPlayer : public myPlayer
{
    Q_OBJECT

public:
    myComputerPlayer(const QString& name, bool white, int skill);
    ~myComputerPlayer();

    virtual void yourTurn(const Checkers* game);
    virtual void stop();

    // need this to process thread's events
    virtual void customEvent(QEvent*);

private:
    myThread* m_thread;
    Checkers* m_game;
    int m_skill;
};



/****************************************************************************/
class myThread : public QThread {
public:
    myThread(myComputerPlayer* p, Checkers* g)
	: m_player(p), m_game(g), m_aborted(false) {}

    virtual void run();

    void stop();

    Checkers* game() const { return m_game; }

private:
    myComputerPlayer* m_player;
    Checkers* m_game;
    bool m_aborted;
};


#endif

