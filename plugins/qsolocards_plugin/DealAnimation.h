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

#ifndef __DEALANIMATION_H__
#define __DEALANIMATION_H__

#include <QtCore/QObject>

#include "CardStack.h"
#include "CardMoveRecord.h"
#include <list>
#include <vector>

#include <QtCore/QTimer>
#include <QtGui/QGraphicsPixmapItem>
#include <QtGui/QGraphicsItemAnimation>

// DealItems can be used to define the cards that will be dealt to a stack.
// The DealItem can then be added to the DealItemVector which is the overall
// definition of the deal.   
class DealItem
{
public:
    DealItem(CardStack * pDst,CardStack * pSrc);
    DealItem(const DealItem & rh);
    ~DealItem();

    inline bool isEmpty()const{return (m_flipList.empty() || m_pSrc->isEmpty());}
    inline void addCard(bool flip){m_flipList.push_back(flip);}

    // returns the flip value of the next card and pops it off the list.
    bool getNextCard();

    inline CardStack * dst(){return m_pDst;}
    inline CardStack * src(){return m_pSrc;}

    DealItem & operator=(const DealItem & rh);

private:
    CardStack *       m_pDst;
    CardStack *       m_pSrc;
    std::list<bool>   m_flipList;  // the list allows a record of how many cards to deal from the 
                                   // src to the dst and which ones need to be flipped.
                                   // The cards will only be flipped once added to the dst stack.
};

typedef std::vector<DealItem>  DealItemVector;

class DealGraphicsItem:public QObject,public QGraphicsPixmapItem
{
    Q_OBJECT
public:
    DealGraphicsItem(DealItem & dealItem,CardMoveRecord & moveRecord);
    virtual ~DealGraphicsItem();

    inline DealItem & getItem(){return m_dealItem;}

    void setupAnimation(unsigned int duration,unsigned int delay,unsigned int zValue);

    inline bool flipCard()const{return m_flipCard;}  
    
    inline const PlayingCard & card()const{return m_card;}

    void stopAni();

signals:
    void aniFinished(DealGraphicsItem *);
public slots:
    void slotAniFinished();
    void slotTimeToStart();

private:
    DealItem &                m_dealItem;
    bool                      m_flipCard;
    PlayingCard               m_card;
    QGraphicsItemAnimation *  m_pGItemAni;
    CardMoveRecord &          m_moveRecord;
    QTimeLine                 m_timeLine;
    QTimer                    m_delayTimer;
    bool                      m_emitFinished;
};

typedef std::vector<DealGraphicsItem *>   DealGraphicsItemVector;

class DealAnimation: public QObject
{
    Q_OBJECT
public:
    enum
    {
	PerDealDuration=400,
	PerCardDelay=60
    };

    DealAnimation();
    virtual ~DealAnimation();

    // this duration may not be the total for the entire deal.  This the duration for one round.
    // So, if you have ten stacks and are dealing 5 cards to eacy stack.  The entire duration will
    // be the duration x 5.
    inline void setDuration(int durInMilSecs=PerDealDuration){m_duration=durInMilSecs;}
    inline int getDuration()const{return m_duration;}

    // the option of sending the complete signal is for cases like the initial deal of the cards.
    // In that case the caller does not care when it is done as long as the user can't do anything
    // until the deal is complete.  The use of the CardAnimationLock will ensure this whether or not
    // the caller wants to get the notification.
    bool dealCards(const DealItemVector & itemVector,bool createMoveRecord=true);

    void stopAni();

signals:
    void cardsMoved(const CardMoveRecord & moveRecord);

public slots:
     void slotAniFinished(DealGraphicsItem * pDealGraphicsItem);

private:
    bool cardsRemaining();
    void cleanUpGraphicsItem(DealGraphicsItem * pDealGraphicsItem);
    void cleanUpGraphicsItems();
    void noAniStackUdpates();
    void buildAniStackUpdates();

    CardMoveRecord            m_moveRecord;
    DealItemVector            m_dealItemVector;
    DealGraphicsItemVector    m_graphicsItemVector;
    bool                      m_aniRunning;
    bool                      m_stopAni;
    int                       m_duration;
    bool                      m_createMoveRecord;
};

#endif
