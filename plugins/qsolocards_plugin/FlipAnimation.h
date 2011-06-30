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
#ifndef __FLIPANIMATION_H__
#define __FLIPANIMATION_H__

#include "CardMoveRecord.h"

#include <QtCore/QObject>
#include <QtCore/QTimeLine>
#include <QtGui/QGraphicsItemAnimation>
#include <QtGui/QGraphicsPixmapItem>
#include <QtGui/QPixmap>

class CardStack;

// This animation is different from the DealAnimation and StackToStackAniMove in that
// it will require the stack to change during the animation.  Since, the effect of the
// animation is to show a card flipping the stack below it will be shown.  And the stack
// needs to be shown without the card in it when it is exposed by the animation.  So, the
// animation is a combination of the FlipAnimation object and CardStack object to get the 
// flip effect.
class FlipAnimation: public QObject
{
    Q_OBJECT
public:
    FlipAnimation();
    virtual ~FlipAnimation();

    // This animation is specifically for flipping the last card in a
    // stack.  Cases where a card is flipped that is under other cards
    // is not covered.  If needed it can be added later.  Something where
    // the cards on top of it are pivoted out of the way, doing
    // a flip of the desired card, and pivoting the cards back down on
    // top of the flip card would be a very nice effect.
    void flipCard(CardStack * pSrc,int duration=300);

    void stopAni();

    bool isAniRunning() const;

signals:
    void flipComplete(CardStack * pSrc);

public slots:
    void slotAniFinished();
    void slotFlipAniProgress(qreal currProgress);

private:
    void runAnimation(int duration);

    CardStack *              m_pSrc;
    PlayingCard              m_card;
    QTimeLine                m_timeLine;
    QGraphicsItemAnimation * m_pItemAni;
    QGraphicsPixmapItem *    m_pPixmapItem;
    bool                     m_flipPtReached;
    bool                     m_aniRunning;
};

#endif
