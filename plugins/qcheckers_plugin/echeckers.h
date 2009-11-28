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

#ifndef ECHECKERS_H
#define ECHECKERS_H

#include "checkers.h"
#include "pdn.h"


class ECheckers:public Checkers
{
public:
    virtual bool go1(int,int);


    virtual int type() const { return ENGLISH; }

    virtual bool checkCapture1() const;
    virtual bool checkCapture2() const;

protected:
    virtual bool checkCapture1(int) const;

private:
    void kingMove2(int,int &);

    bool manCapture1(int,int,bool &);
    bool kingCapture1(int,int,bool &);

    bool manCapture2(int,int &);
    bool kingCapture2(int,int,int &);

};

#endif
