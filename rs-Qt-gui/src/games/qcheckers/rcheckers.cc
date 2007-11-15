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
//
// Russian Checkers


#include "rcheckers.h"


///////////////////////////////////////////////////
//
//  Player Functions
//
///////////////////////////////////////////////////


bool RCheckers::go1(int from,int field)
{
    from=internal(from);
    field=internal(field);

    to=field;

    if(checkCapture1())
    {
        bool capture=false;
        switch(board[from])
        {
        case MAN1:
            if(manCapture1(from,UL,capture)) return true;
            if(manCapture1(from,UR,capture)) return true;
            if(manCapture1(from,DL,capture)) return true;
            if(manCapture1(from,DR,capture)) return true;
            return false;
        case KING1:
            if(kingCapture1(from,UL,capture)) return true;
            if(kingCapture1(from,UR,capture)) return true;
            if(kingCapture1(from,DL,capture)) return true;
            if(kingCapture1(from,DR,capture)) return true;
            return false;
        }
    }
    else
    {
        switch(board[from])
        {
        case MAN1:
            if((to==(from-6))||(to==(from-5)))
            {
                board[from]=FREE;
                if(to<10) board[to]=KING1;
                else board[to]=MAN1;
                return true;
            }
            return false;
        case KING1:
            for(int i=from-6;;i-=6)
            {
                if(i==to)
                {
                    board[from]=FREE;
                    board[to]=KING1;
                    return true;
                }
                else if(board[i]==FREE) continue;
                else break;
            }
            for(int i=from-5;;i-=5)
            {
                if(i==to)
                {
                    board[from]=FREE;
                    board[to]=KING1;
                    return true;
                }
                else if(board[i]==FREE) continue;
                else break;
            }
            for(int i=from+5;;i+=5)
            {
                if(i==to)
                {
                    board[from]=FREE;
                    board[to]=KING1;
                    return true;
                }
                else if(board[i]==FREE) continue;
                else break;
            }
            for(int i=from+6;;i+=6)
            {
                if(i==to)
                {
                    board[from]=FREE;
                    board[to]=KING1;
                    return true;
                }
                else if(board[i]==FREE) continue;
                else break;
            }
            return false;
        }
    }
    return false;
}



bool RCheckers::checkCapture1() const
{
    for(int i=6;i<48;i++)
	if(checkCapture1(i))
	    return true;

    return false;
}


bool RCheckers::checkCapture1(int i) const
{
    switch(board[i])
    {
    case MAN1:
	if(board[i-6]==MAN2 || board[i-6]==KING2)
	    if(board[i-12]==FREE) return true;
	if(board[i-5]==MAN2 || board[i-5]==KING2)
	    if(board[i-10]==FREE) return true;
	if(board[i+5]==MAN2 || board[i+5]==KING2)
	    if(board[i+10]==FREE) return true;
	if(board[i+6]==MAN2 || board[i+6]==KING2)
	    if(board[i+12]==FREE) return true;
	break;
    case KING1:
	int k;
	for(k=i-6;board[k]==FREE;k-=6);
	if(board[k]==MAN2 || board[k]==KING2)
	    if(board[k-6]==FREE) return true;

	for(k=i-5;board[k]==FREE;k-=5);
	if(board[k]==MAN2 || board[k]==KING2)
	    if(board[k-5]==FREE) return true;

	for(k=i+5;board[k]==FREE;k+=5);
	if(board[k]==MAN2 || board[k]==KING2)
	    if(board[k+5]==FREE) return true;

	for(k=i+6;board[k]==FREE;k+=6);
	if(board[k]==MAN2 || board[k]==KING2)
	    if(board[k+6]==FREE) return true;
    }

    return false;
}


/* ORIG func aw
bool RCheckers::checkCapture1()
{
    for(int i=6;i<48;i++)
    {
        switch(board[i])
        {
        case MAN1:
            if(board[i-6]==MAN2 || board[i-6]==KING2)
                if(board[i-12]==FREE) return true;
            if(board[i-5]==MAN2 || board[i-5]==KING2)
                if(board[i-10]==FREE) return true;
            if(board[i+5]==MAN2 || board[i+5]==KING2)
                if(board[i+10]==FREE) return true;
            if(board[i+6]==MAN2 || board[i+6]==KING2)
                if(board[i+12]==FREE) return true;
            break;
        case KING1:
            int k;
            for(k=i-6;board[k]==FREE;k-=6);
            if(board[k]==MAN2 || board[k]==KING2)
                if(board[k-6]==FREE) return true;

            for(k=i-5;board[k]==FREE;k-=5);
            if(board[k]==MAN2 || board[k]==KING2)
                if(board[k-5]==FREE) return true;

            for(k=i+5;board[k]==FREE;k+=5);
            if(board[k]==MAN2 || board[k]==KING2)
                if(board[k+5]==FREE) return true;

            for(k=i+6;board[k]==FREE;k+=6);
            if(board[k]==MAN2 || board[k]==KING2)
                if(board[k+6]==FREE) return true;
        }
    }
    return false;
}
*/


// Return TRUE if a course of the player true
// Return FALSE if a course of the player incorrect

bool RCheckers::manCapture1(int from,int direction,bool &capture)
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

            if(k<10)
            {
                board[k]=KING1;
		if(kingCapture1(k,direction+11,next))
		{
		    board[i]=FREE;
		    return true;
		}
            }
            else
            {
                board[k]=MAN1;
                if(direction==UL || direction==DR)
                {
                    if(manCapture1(k,UR,next)) {board[i]=FREE;return true;}
                    if(manCapture1(k,DL,next)) {board[i]=FREE;return true;}
                }
                else
                {
                    if(manCapture1(k,UL,next)) {board[i]=FREE;return true;}
                    if(manCapture1(k,DR,next)) {board[i]=FREE;return true;}
                }
                if(manCapture1(k,direction,next)) {board[i]=FREE;return true;}
            }

            if((!next) && k==to) {board[i]=FREE;return true;}

            board[k]=FREE;
            board[i]=save;
            board[from]=MAN1;
            capture=true;
        }
    }
    return false;
}


bool RCheckers::kingCapture1(int from,int direction,bool &capture)
{
    int i;
    for(i=from+direction;board[i]==FREE;i+=direction);

    if(board[i]==MAN2 || board[i]==KING2)
    {
        int k=i+direction;
        if(board[k]==FREE)
        {
            bool next=false;
            int save=board[i];
            board[from]=FREE;
            board[i]=NONE;

            for(;board[k]==FREE;k+=direction)
            {
                board[k]=KING1;
                if(direction==UL || direction==DR)
                {
                    if(kingCapture1(k,UR,next)) {board[i]=FREE;return true;}
                    if(kingCapture1(k,DL,next)) {board[i]=FREE;return true;}
                }
                else
                {
                    if(kingCapture1(k,UL,next)) {board[i]=FREE;return true;}
                    if(kingCapture1(k,DR,next)) {board[i]=FREE;return true;}
                }
                board[k]=FREE;
            }

            board[k-=direction]=KING1;
            if(kingCapture1(k,direction,next)) {board[i]=FREE;return true;}
            board[k]=FREE;

            if(!next)
                for(;k!=i;k-=direction)
                    if(k==to) {board[i]=FREE;board[k]=KING1;return true;}

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


void RCheckers::kingMove2(int from,int &resMax)
{
    board[from]=FREE;
    for(int i=from-6;board[i]==FREE;i-=6)
    {
        board[i]=KING2;
        turn(resMax);
        board[i]=FREE;
    }
    for(int i=from-5;board[i]==FREE;i-=5)
    {
        board[i]=KING2;
        turn(resMax);
        board[i]=FREE;
    }
    for(int i=from+5;board[i]==FREE;i+=5)
    {
        board[i]=KING2;
        turn(resMax);
        board[i]=FREE;
    }
    for(int i=from+6;board[i]==FREE;i+=6)
    {
        board[i]=KING2;
        turn(resMax);
        board[i]=FREE;
    }
    board[from]=KING2;
}


bool RCheckers::checkCapture2() const
{
    for(int i=6;i<48;i++)
    {
        switch(board[i])
        {
        case MAN2:
            if(board[i-6]==MAN1 || board[i-6]==KING1)
                if(board[i-12]==FREE) return true;
            if(board[i-5]==MAN1 || board[i-5]==KING1)
                if(board[i-10]==FREE) return true;
            if(board[i+5]==MAN1 || board[i+5]==KING1)
                if(board[i+10]==FREE) return true;
            if(board[i+6]==MAN1 || board[i+6]==KING1)
                if(board[i+12]==FREE) return true;
            break;
        case KING2:
            int k;
            for(k=i-6;board[k]==FREE;k-=6);
            if(board[k]==MAN1 || board[k]==KING1)
                if(board[k-6]==FREE) return true;

            for(k=i-5;board[k]==FREE;k-=5);
            if(board[k]==MAN1 || board[k]==KING1)
                if(board[k-5]==FREE) return true;

            for(k=i+5;board[k]==FREE;k+=5);
            if(board[k]==MAN1 || board[k]==KING1)
                if(board[k+5]==FREE) return true;

            for(k=i+6;board[k]==FREE;k+=6);
            if(board[k]==MAN1 || board[k]==KING1)
                if(board[k+6]==FREE) return true;
        }
    }
    return false;
}


// Return TRUE if it is possible to capture
// Return FALSE if it is impossible to capture

bool RCheckers::manCapture2(int from,int &resMax)
{
    bool capture=false;

    int i=from-6;
    if(board[i]==MAN1 || board[i]==KING1)
    {
        int k=from-12;
        if(board[k]==FREE)
        {
            int save=board[i];
            board[from]=FREE;
            board[i]=NONE;
            board[k]=MAN2;
            resMax--;
            if(!manCapture2(k,resMax)) turn(resMax,true);
            resMax++;
            board[k]=FREE;
            board[i]=save;
            board[from]=MAN2;
            capture=true;
        }
    }

    i=from-5;
    if(board[i]==MAN1 || board[i]==KING1)
    {
        int k=from-10;
        if(board[k]==FREE)
        {
            int save=board[i];
            board[from]=FREE;
            board[i]=NONE;
            board[k]=MAN2;
            resMax--;
            if(!manCapture2(k,resMax)) turn(resMax,true);
            resMax++;
            board[k]=FREE;
            board[i]=save;
            board[from]=MAN2;
            capture=true;
        }
    }

    i=from+5;
    if(board[i]==MAN1 || board[i]==KING1)
    {
        int k=from+10;
        if(board[k]==FREE)
        {
            int save=board[i];
            board[from]=FREE;
            board[i]=NONE;
            resMax--;
            if(from>32)
            {
                board[k]=KING2;
                if(!kingCapture2(k,UL,resMax)) turn(resMax,true);
            }
            else
            {
                board[k]=MAN2;
                if(!manCapture2(k,resMax)) turn(resMax,true);
            }
            resMax++;
            board[k]=FREE;
            board[i]=save;
            board[from]=MAN2;
            capture=true;
        }
    }

    i=from+6;
    if(board[i]==MAN1 || board[i]==KING1)
    {
        int k=from+12;
        if(board[k]==FREE)
        {
            int save=board[i];
            board[from]=FREE;
            board[i]=NONE;
            resMax--;
            if(from>32)
            {
                board[k]=KING2;
                if(!kingCapture2(k,UR,resMax)) turn(resMax,true);
            }
            else
            {
                board[k]=MAN2;
                if(!manCapture2(k,resMax)) turn(resMax,true);
            }
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


bool RCheckers::kingCapture2(int from,int direction,int &resMax)
{
    int i;
    for(i=from+direction;board[i]==FREE;i+=direction);

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

            for(;board[k]==FREE;k+=direction)
            {
                board[k]=KING2;
                if(direction==UL || direction==DR)
                {
                    if(kingCapture2(k,UR,resMax)) capture=true;
                    if(kingCapture2(k,DL,resMax)) capture=true;
                }
                else
                {
                    if(kingCapture2(k,UL,resMax)) capture=true;
                    if(kingCapture2(k,DR,resMax)) capture=true;
                }
                board[k]=FREE;
            }

            board[k-=direction]=KING2;
            if(kingCapture2(k,direction,resMax)) capture=true;
            board[k]=FREE;

            if(!capture)
                for(;k!=i;k-=direction)
                {
                    board[k]=KING2;
                    turn(resMax,true);
                    board[k]=FREE;
                }

            resMax++;
            board[i]=save;
            board[from]=KING2;
            return true;
        }
    }
    return false;
}


