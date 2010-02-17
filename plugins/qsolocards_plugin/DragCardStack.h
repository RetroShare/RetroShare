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
#ifndef __DRAGCARDSTACK_H__
#define __DRAGCARDSTACK_H__

#include <QtCore/QObject>
#include <QtGui/QGraphicsPixmapItem>
#include "CardDeck.h"
#include "CardMoveRecord.h"

class CardStack;

class DragCardStack:public QObject,public QGraphicsPixmapItem
{
    Q_OBJECT
public:
    DragCardStack(CardStack * pSrc);
    virtual ~DragCardStack();

    inline bool cardDragStarted()const{return m_dragStarted;}

    void startCardMove(const QPointF & globalMousePt,
		       unsigned int startCardIndex,
		       const PlayingCardVector & moveCards);

signals:
    void cardsMoved(const CardMoveRecord & moveRecord);

protected:
    friend class CardStack;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

private:
    PlayingCardVector    m_cardVector;
    CardStack *          m_pSrc;
    QSize                m_size;
    QPoint               m_cursorPt;
    bool                 m_dragStarted;
    CardStack *          m_pDst;
};

#endif
