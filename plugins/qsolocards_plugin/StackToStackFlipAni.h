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

#ifndef __STACKTOSTACKFLIPANI_H__
#define __STACKTOSTACKFLIPANI_H__

#include "CardStack.h"
#include "CardMoveRecord.h"

#include <QtCore/QObject>
#include <QtCore/QTimeLine>
#include <QtGui/QGraphicsItemAnimation>
#include <QtGui/QGraphicsPixmapItem>
#include <QtGui/QPixmap>

class StackToStackFlipAni: public QObject
{
    Q_OBJECT
public:
    static const qreal ExposedPrecentShownCards;
    static const qreal ExposedPrecentHiddenCards; 

    StackToStackFlipAni();
    virtual ~StackToStackFlipAni();

    inline bool isAniRunning() const{return m_aniRunning;}

    inline void stopAni(){if (m_aniRunning){slotAniFinished(false);m_aniRunning=false;}}

    void moveCards(CardStack * pDst,CardStack * pSrc,
		   int numCards=1,int cardsShown=-1,
		   int duration=500);

signals:
    void cardsMoved(const CardMoveRecord & moveRecord);

public slots:
    void slotAniFinished(bool emitSignal=true);
    void slotAniProgress(qreal currProgress);

protected:
    virtual void runAnimation(int duration);

    virtual QPixmap * getAniPixmap();

    QSize calcPixmapSize();
    int getOverlapIncrement(unsigned int index);

    void flipCards();

private:
    CardStack *  m_pDst;
    CardStack *  m_pSrc;
    PlayingCardVector m_cardVector;
    unsigned int      m_firstCardToShow; // first index into m_cardVector to show
    bool              m_flipPtReached;
    CardMoveRecord    m_moveRecord;

    QTimeLine  * m_pTimeLine;
    QGraphicsItemAnimation * m_pItemAni;
    QGraphicsPixmapItem * m_pPixmapItem;    
    bool m_aniRunning;
};

#endif
