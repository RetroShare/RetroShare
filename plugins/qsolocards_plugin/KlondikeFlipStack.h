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

#ifndef KLONDIKEFLIPSTACK_H
#define KLONDIKEFLIPSTACK_H

#include "CardStack.h"
#include <vector>
#include <QtCore/QRectF>

class KlondikeFlipStack : public CardStack
{
    Q_OBJECT
public:
    static const qreal ExposedPrecent;

    KlondikeFlipStack();
    virtual ~KlondikeFlipStack();

    inline void cardsToShow(unsigned int cardsToShow=1){m_cardsShown=((0==cardsToShow)?1:cardsToShow);}

    // this function gets the point in the scene that a card would be added to this stack.
    virtual QPointF getGlobalCardAddPt() const;
    virtual QPointF getGlobalLastCardPt() const;
    virtual QPointF getGlobalCardPt(int index) const;

    virtual void updateStack();

protected:
    virtual bool getCardIndex(const QPointF & pos,unsigned int & index);

    void calcPixmapSize(const PlayingCardVector & cardVector,
			QSize & size,unsigned int showIndex);

    int getOverlapIncrement(const PlayingCardVector & cardVector,
			    unsigned int index,
			    unsigned int showIndex) const;

    bool canMoveCard(unsigned int index) const;

private:
    std::vector<QRectF>  m_bRectVector;
    unsigned int         m_cardsShown;
    unsigned int         m_firstShowCard;
};

#endif // KLONDIKEFLIPSTACK_H
