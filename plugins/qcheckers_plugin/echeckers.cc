/***************************************************************************
 *   Copyright (C) 2002-2003 Andi Peredri                                  *
 *   andi@ukr.net                                                          *
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


// aw?1 - due to english rules a man reaching the king-row becomes a king
//	and the is complete.
//
// English Checkers


#include "echeckers.h"


///////////////////////////////////////////////////
//
//  Player Functions
//
///////////////////////////////////////////////////


bool ECheckers::go1(int from, int field)
{
    from=internal(from);
    field=internal(field);

    to=field;

    if(checkCapture1()) {
        bool capture=false;

        switch(board[from]) {
        case MAN1:
            if(manCapture1(from, UL, capture)) return true;
            if(manCapture1(from, UR, capture)) return true;
            return false;
        case KING1:
            if(kingCapture1(from, UL, capture)) return true;
            if(kingCapture1(from, UR, capture)) return true;
            if(kingCapture1(from, DL, capture)) return true;
            if(kingCapture1(from, DR, capture)) return true;
            return false;
        }

    } else {
        switch(board[from]) {
        case MAN1:
            if((to==(from-6))||(to==(from-5))) {
                board[from]=FREE;
                if(to<10)
		    board[to]=KING1;
                else
		    board[to]=MAN1;
                return true;
            }
            return false;
        case KING1:
	    if((to==(from-6))||(to==(from-5)) ||
		    (to==(from+5))||(to==(from+6)) ) {
		board[from]=FREE;
                board[to]=KING1;
                return true;
            }
            return false;
        }
    }

    return false;
}


bool ECheckers::checkCapture1() const
{
    for(int i=6;i<48;i++)
	if(checkCapture1(i))
	    return true;

    return false;
}


bool ECheckers::checkCapture1(int i) const
{
    switch(board[i]) {
    case MAN1:
	// forward-left
	if(board[i-6]==MAN2 || board[i-6]==KING2)
	    if(board[i-12]==FREE) return true;
	// forward-right
	if(board[i-5]==MAN2 || board[i-5]==KING2)
	    if(board[i-10]==FREE) return true;
	break;

    case KING1:
	// forward-left
	if(board[i-6]==MAN2 || board[i-6]==KING2)
	    if(board[i-12]==FREE) return true;
	// forward-right
	if(board[i-5]==MAN2 || board[i-5]==KING2)
	    if(board[i-10]==FREE) return true;
	// backward-left
	if(board[i+5]==MAN2 || board[i+5]==KING2)
	    if(board[i+10]==FREE) return true;
	// backward-right
	if(board[i+6]==MAN2 || board[i+6]==KING2)
	    if(board[i+12]==FREE) return true;
    }

    return false;
}


/* ORIG FUNC aw???
bool ECheckers::checkCapture1()
{
    for(int i=6;i<48;i++) {
        switch(board[i]) {
        case MAN1:
	    // forward-left
            if(board[i-6]==MAN2 || board[i-6]==KING2)
                if(board[i-12]==FREE) return true;
	    // forward-right
            if(board[i-5]==MAN2 || board[i-5]==KING2)
                if(board[i-10]==FREE) return true;
            break;

        case KING1:
	    // forward-left
            if(board[i-6]==MAN2 || board[i-6]==KING2)
                if(board[i-12]==FREE) return true;
	    // forward-right
            if(board[i-5]==MAN2 || board[i-5]==KING2)
                if(board[i-10]==FREE) return true;
	    // backward-left
            if(board[i+5]==MAN2 || board[i+5]==KING2)
                if(board[i+10]==FREE) return true;
	    // backward-right
            if(board[i+6]==MAN2 || board[i+6]==KING2)
                if(board[i+12]==FREE) return true;
        }
    }

    return false;
}
*/


// Return TRUE if a course of the player true
// Return FALSE if a course of the player incorrect

bool ECheckers::manCapture1(int from, int direction, bool& capture)
{
    int i=from+direction;

    if(board[i]==MAN2 || board[i]==KING2) {
        int k=i+direction;
        if(board[k]==FREE) {
            bool next=false;
            int save=board[i];
            board[from]=FREE;
            board[i]=NONE;

	    // become a king!
            if(k<10) {
                board[k]=KING1;
		/*aw?1
		if(kingCapture1(k, direction+11, next)) {
		    board[i]=FREE;
		    return true;
		}
		*/
            } else {
                board[k]=MAN1;
                if(manCapture1(k,UL,next)) {board[i]=FREE; return true;}
                if(manCapture1(k,UR,next)) {board[i]=FREE; return true;}
            }

	    //?? make move here, too???
            if((!next) && k==to) {board[i]=FREE; return true;}// move success

	    // move failed, restore
            board[k]=FREE;
            board[i]=save;
            board[from]=MAN1;
            capture=true;
        }
    }

    return false;
}


bool ECheckers::kingCapture1(int from, int direction, bool& capture)
{
    int i=from+direction;
    if(board[i]==MAN2 || board[i]==KING2)
    {
        int k=i+direction;
        if(board[k]==FREE)
        {
            bool next=false;
            int save=board[i];
            board[from]=FREE;
            board[i]=NONE;
            board[k]=KING1;

            if(direction==UL || direction==DR) {
                if(kingCapture1(k,UR,next)) {board[i]=FREE;return true;}
                if(kingCapture1(k,DL,next)) {board[i]=FREE;return true;}
            } else {
                if(kingCapture1(k,UL,next)) {board[i]=FREE;return true;}
                if(kingCapture1(k,DR,next)) {board[i]=FREE;return true;}
            }
	    if(kingCapture1(k,direction,next)) {board[i]=FREE;return true;}

            if((!next) && k==to) {board[i]=FREE;return true;}// move ok

	    // move failed, restore
            board[k]=FREE;
            board[i]=save;
            board[from]=KING1;
            capture=true;
        }
    }
    return false;
}


////////////////////////////////////////////////////
//
// Computer Functions
//
////////////////////////////////////////////////////


void ECheckers::kingMove2(int from, int& resMax)
{
    board[from]=FREE;

    int i=from-6;
    if(board[i]==FREE) {
        board[i]=KING2;
        turn(resMax);
        board[i]=FREE;
    }

    i=from-5;
    if(board[i]==FREE) {
        board[i]=KING2;
        turn(resMax);
        board[i]=FREE;
    }

    i=from+5;
    if(board[i]==FREE) {
        board[i]=KING2;
        turn(resMax);
        board[i]=FREE;
    }

    i=from+6;
    if(board[i]==FREE) {
        board[i]=KING2;
        turn(resMax);
        board[i]=FREE;
    }

    board[from]=KING2;
}


bool ECheckers::checkCapture2() const
{
    for(int i=6;i<48;i++)
    {
        switch(board[i])
        {
        case MAN2:
            if(board[i+5]==MAN1 || board[i+5]==KING1)
                if(board[i+10]==FREE) return true;
            if(board[i+6]==MAN1 || board[i+6]==KING1)
                if(board[i+12]==FREE) return true;
            break;
        case KING2:
            if(board[i-6]==MAN1 || board[i-6]==KING1)
                if(board[i-12]==FREE) return true;
            if(board[i-5]==MAN1 || board[i-5]==KING1)
                if(board[i-10]==FREE) return true;
            if(board[i+5]==MAN1 || board[i+5]==KING1)
                if(board[i+10]==FREE) return true;
            if(board[i+6]==MAN1 || board[i+6]==KING1)
                if(board[i+12]==FREE) return true;
        }
    }
    return false;
}


// Return TRUE if it is possible to capture
// Return FALSE if it is impossible to capture
bool ECheckers::manCapture2(int from, int& resMax)
{
    bool capture=false;

    // try left-down
    int i=from+5;
    if(board[i]==MAN1 || board[i]==KING1) {
        int k=from+10;
        if(board[k]==FREE) {
            int save=board[i];
            board[from]=FREE;
            board[i]=NONE;
            resMax--;

	    // become a king!
            if(from>32) {
                board[k]=KING2;
		// aw?1
		turn(resMax, true);	//aw???
                //aw??if(!kingCapture2(k, UL, resMax)) turn(resMax, true);
            } else {
                board[k]=MAN2;
                if(!manCapture2(k, resMax)) turn(resMax, true);
            }

	    // restore
            resMax++;
            board[k]=FREE;
            board[i]=save;
            board[from]=MAN2;
            capture=true;
        }
    }

    // now right-down
    i=from+6;
    if(board[i]==MAN1 || board[i]==KING1) {
        int k=from+12;
        if(board[k]==FREE) {
            int save=board[i];
            board[from]=FREE;
            board[i]=NONE;
            resMax--;

	    // become a king!
            if(from>32) {
                board[k]=KING2;
		// aw?1
		turn(resMax, true);	// aw???
                //aw???if(!kingCapture2(k,UR,resMax)) turn(resMax,true);
            } else {
                board[k]=MAN2;
                if(!manCapture2(k,resMax)) turn(resMax,true);
            }

	    // restore
            resMax++;
            board[k]=FREE;
            board[i]=save;
            board[from]=MAN2;
            capture=true;
        }
    }

    if(capture) return true;
    return false;
}


bool ECheckers::kingCapture2(int from, int direction, int &resMax)
{
    int i=from+direction;
    if(board[i]==MAN1 || board[i]==KING1)
    {
        int k=i+direction;
        if(board[k]==FREE)
        {
            bool capture=false;
            int save=board[i];
            board[from]=FREE;
            board[i]=NONE;
            resMax--;

            board[k]=KING2;
            if(direction==UL || direction==DR) {
                if(kingCapture2(k,UR,resMax)) capture=true;
                if(kingCapture2(k,DL,resMax)) capture=true;
            } else {
                if(kingCapture2(k,UL,resMax)) capture=true;
                if(kingCapture2(k,DR,resMax)) capture=true;
            }
            if(kingCapture2(k,direction,resMax)) capture=true;

            if(!capture) turn(resMax,true);
            board[k]=FREE;

	    //restore
            resMax++;
            board[i]=save;
            board[from]=KING2;
            return true;
        }
    }
    return false;
}


