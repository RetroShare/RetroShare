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

#ifndef __STACKTOSTACKANIMOVE_H__
#define __STACKTOSTACKANIMOVE_H__

#include "CardStack.h"
#include "CardMoveRecord.h"

#include <QtCore/QObject>
#include <QtCore/QTimeLine>
#include <QtCore/QTimer>
#include <QtGui/QGraphicsItemAnimation>
#include <QtGui/QGraphicsPixmapItem>

/*
  StackToStackAniMove is just for moving one set of cards to another stack.  Normal usage is
  when a card in one stack is clicked on and then it is determined that this should result
  in moving the card(s) to a different stack.  (Auto)flipping
  a card when the move is complete is also done if included in the moveRecord.    
*/

class StackToStackAniMoveItem
{
public:
    StackToStackAniMoveItem(const CardMoveRecord & startMoveRecord,int duration);
    StackToStackAniMoveItem(const  StackToStackAniMoveItem & rh);
    StackToStackAniMoveItem();

    virtual ~StackToStackAniMoveItem();

    inline CardStack * src() {return m_pSrc;}
    inline CardStack * dst() {return m_pDst;}
    inline CardStack * flipStack() {return m_pFlipStack;}
    inline int flipIndex()const{return m_flipIndex;}
    inline int srcTopCardIndex()const {return m_srcTopCardIndex;}
    inline const PlayingCardVector & getCardVector() const{return m_cardVector;}
    inline int duration()const{return m_duration;}
    inline const CardMoveRecord & moveRecord(){return m_moveRecord;}

    StackToStackAniMoveItem & operator=(const StackToStackAniMoveItem & rh);

private:
    CardStack * m_pSrc;
    CardStack * m_pDst;
    CardStack * m_pFlipStack;
    int   m_flipIndex;
    int m_srcTopCardIndex;
    PlayingCardVector m_cardVector;
    int m_duration;
    CardMoveRecord    m_moveRecord;
};


class StackToStackAniMove:public QObject
{
    Q_OBJECT
public:
    StackToStackAniMove();
    virtual ~StackToStackAniMove();

    void moveCards(const CardMoveRecord & startMoveRecord,int duration=400);
    void stopAni();

signals:
    void cardsMoved(const CardMoveRecord & moveRecord);

public slots:
    void slotAniFinished(bool emitSignal=true);
    void slotWaitForFlipComplete();

protected:

    void runAnimation();

private:
    QTimeLine  * m_pTimeLine;
    QTimer       m_flipDelayTimer;
    QGraphicsItemAnimation * m_pItemAni;
    QGraphicsPixmapItem * m_pPixmapItem;    
    StackToStackAniMoveItem  m_aniMoveItem;
    bool m_aniRunning;
};



#endif
