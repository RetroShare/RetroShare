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

#ifndef __FREECELLSTACK_H__
#define __FREECELLSTACK_H__

#include "VCardStack.h"

class FreeCellStack : public VCardStack
{
    Q_OBJECT
public:
    FreeCellStack();
    ~FreeCellStack();
    inline bool isCheating() const{return m_cheat;}

    inline void setCheat(bool cheat=true){m_cheat=cheat;}

    bool canAddCards(const PlayingCardVector &);

    inline void setMaxMoveCards(unsigned int maxMoveCards){m_maxMoveCards=((maxMoveCards>1)?(maxMoveCards):(1));}
protected:
    bool canMoveCard(unsigned int index) const;

private:
    bool m_cheat;
    unsigned int m_maxMoveCards;
};

#endif // __FREECELLSTACK_H__
