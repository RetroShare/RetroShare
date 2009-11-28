/***************************************************************************
 *   Copyright (C) 2004-2005 by Artur Wiebe                                *
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

#ifndef _PLAYER_H_
#define _PLAYER_H_

#include <QObject>


class Checkers;
class Field;


class myPlayer : public QObject
{
    Q_OBJECT

public:
    myPlayer(const QString& name, bool white)
	: m_name(name), m_white(white), m_opponent(0) {}

    // information
    bool isWhite() const { return m_white; }
    void setWhite(bool b) { m_white = b; }
    //
    const QString& name() const { return m_name; }
    void setName(const QString& n) { m_name = n; }
    //
    virtual bool isHuman() const { return false; }

    //
    virtual void yourTurn(const Checkers* game) = 0;
    // return false on incorrect course.
    // set select to true to select the select, or unselect.
    // QString contains error msg.
    virtual bool fieldClicked(int, bool*,QString&) { return true; }
    // computerplayer terminates his thinking thread.
    // humanplayer
    // networkplayer disconnects from the server.
    virtual void stop() = 0;

    void setOpponent(myPlayer* o) { m_opponent=o; }
    myPlayer* opponent() const { return m_opponent; }

signals:
    // emitted when move is done, provides the complete board converted to
    // string.
    void moveDone(const QString&);

private:
    QString m_name;
    bool m_white;

    myPlayer* m_opponent;
};


#endif
