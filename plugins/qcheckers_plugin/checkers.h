/***************************************************************************
 *   Copyright (C) 2002-2003 Andi Peredri                                  *
 *   andi@ukr.net                                                          *
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

#ifndef CHECKERS_H
#define CHECKERS_H


#include <QString>


// do not change - hard coded
#define NONE   0
#define MAN1   1
#define KING1  2
#define FREE   3
#define KING2  4
#define MAN2   5
#define FULL   6

#define UL -6
#define UR -5
#define DL  5
#define DR  6


class Checkers
{
public:
	Checkers();
	virtual ~Checkers() {}

	bool setup(int setupboard[]);
	virtual bool go1(int from, int to)=0;

	void go2();

	void setSkill(int i) { levelmax=i; };
	int skill() const { return levelmax; }
	virtual int type() const = 0;

	int item(int i) const { return board[internal(i)]; }
	void setItem(int i, int item) { board[internal(i)] = item; }

	// string representation of the game board.
	// set rotate to switch player sides.
	QString toString(bool rotate) const;
	bool fromString(const QString&);

	// checks for a capture/move for particular stone in external
	// representation. human player only.
	bool canCapture1(int i) { return checkCapture1(internal(i)); }
	bool canMove1(int i) { return checkMove1(internal(i)); }

	bool checkMove1() const;
	bool checkMove2() const;
	virtual bool checkCapture1() const = 0;
	virtual bool checkCapture2() const = 0;

protected:
	bool checkMove1(int) const;
	virtual bool checkCapture1(int) const = 0;

	int level;        // Current level
	int levelmax;     // Maximum level

	int  turn();
	void turn(int&, bool capture=false);

	int to;
	int board[54];
	int bestboard[54];
	int bestcounter;

	virtual void kingMove2(int,int &)=0;

	virtual bool manCapture2(int,int &)=0;
	virtual bool kingCapture2(int,int,int &)=0;

	virtual bool manCapture1(int,int,bool &)=0;
	virtual bool kingCapture1(int,int,bool &)=0;

	int internal(int) const;	// Return internal board position
};


#endif
