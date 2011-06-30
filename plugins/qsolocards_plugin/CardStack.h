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

#ifndef CARDSTACK_H
#define CARDSTACK_H

#include <QtCore/QObject>
#include <QtGui/QGraphicsPixmapItem>

#include "CardMoveRecord.h"

#include "FlipAnimation.h"
#include "DragCardStack.h"

#include <map>
#include <string>

class CardStack;

typedef std::map<std::string,CardStack *> CardStackMap;


class CardStack: public QObject,public QGraphicsPixmapItem
{
    Q_OBJECT
public:
    enum ProcessCardMoveRecordType
    {
	UndoMove=0,
	RedoMove=1
    };

    enum
    {
	HintHighlightNoCards=-2
    };

    CardStack();
    virtual ~CardStack();

    inline const std::string stackName()const{return m_stackName.toStdString();}

    inline bool isFlipAniRunning()const {return m_flipAni.isAniRunning();}

    
    bool allCardsFaceUp()const;

    // top is in this case the last card in the stack.  bottom is index 0 
    bool cardsAscendingTopToBottom()const;
    bool cardsDecendingTopToBottom()const;

    void setTopCardUp(bool faceUp=true);

    bool flipCard(int index,bool aniIfEnabled=true);
    bool flipCard(int index,CardMoveRecord & moveRecord,bool aniIfEnabled=true);

    inline void setAutoTopCardUp(bool autoFaceUp=true){this->m_autoFaceUp=autoFaceUp;}
    inline bool isAutoTopCardUp()const {return m_autoFaceUp;}


    inline bool isHighlighted() const {return m_highlighted;}
    inline void setHighlighted(bool state=true){m_highlighted=state;}

    inline int  hintHighlightIndex() const {return  m_hintHighlightIndex;}


    inline bool showRedealCircle(){return m_showRedealCircle;}
    inline void setShowRedealCircle(bool state=true){m_showRedealCircle=state;}
    
    void addCard(const PlayingCard & newCard);
    void addCards(const PlayingCardVector & cardVector);

    void addCard(const PlayingCard & newCard,CardMoveRecord & moveRecord,bool justUpdateRec=false);
    void addCards(const PlayingCardVector & cardVector,CardMoveRecord & moveRecord,bool justUpdateRec=false);


    // pull the top card off of the stack
    PlayingCard removeTopCard();
    PlayingCard removeTopCard(CardMoveRecord & moveRecord);

    // the cardVector contains the cards that were removed if the index is valid.
    bool removeCardsStartingAt(unsigned int index,PlayingCardVector & removedCards);
    bool removeCardsStartingAt(unsigned int index,PlayingCardVector & removedCards,
			       CardMoveRecord & moveRecord,bool justUpdateRec=false);
    
    void removeAllCards();
    
    inline bool isEmpty() const {return m_cardVector.empty();}

    const PlayingCardVector & getCardVector()const{return m_cardVector;}

    virtual bool canAddCards(const PlayingCardVector &){return false;}

    // this function will return the cards at the end of the stack that can be moved.
    // the function canMoveCards that can be overridden by subclasses controls the
    // cards that can be moved.  This function can also be overridden if something other
    // than the default of just cards at the end of the stack is desired.
    //
    // the cards that can be moved will be added to the end of the card vector passed in
    // the start index of the cards is also set if the function returns true
    // the function returns true if it has cards that can be moved or false if it has none.
    
    // this function has been added to aid in creating hints for moving cards
    virtual bool getMovableCards(PlayingCardVector &, unsigned int & index) const;

    // this function gets the point in the scene that a card would be added to this stack.
    inline virtual QPointF getGlobalCardAddPt() const {return mapToScene(QPointF(0,0));};
    inline virtual QPointF getGlobalLastCardPt() const {return mapToScene(QPointF(0,0));};
    inline virtual QPointF getGlobalCardPt(int index) const {Q_UNUSED(index);return mapToScene(QPointF(0,0));};

    // this function will update the bounding rectangle for the cards.  And update the
    // pixmap for the stack.  The default implementation implementation just calls 
    // getStackPixmap.  Subclasses may want to override this version to record bounding
    // rectangles for cards and then call the base class.
    virtual void updateStack();

    // this function returns a pointer to a pixmap of the stack.  The pixmap is the responsiblity
    // of the caller to free.  This function can be overridden in subclasses to create
    // different look for the stack when drawn or cards from it are dragged to a different
    // stack.
    virtual QPixmap * getStackPixmap(const PlayingCardVector & cardVector,
				     bool highlighted=false,
				     int hintHighlightIndex=-1);

    // override in subclasses for scoring per stack if desired.
    virtual int score() const{return 0;}

    ///////////////////////////////////////////////////////////////////////////////
    // public static functions
    ///////////////////////////////////////////////////////////////////////////////
    static void updateAllStacks();

    static void clearAllStacks();

    static inline void lockUserInteration(bool lock=true){m_lockUserInteraction=lock;}
    static inline bool isUserInteractionLocked() {return m_lockUserInteraction;}

    static CardStack * getStackByName(const std::string & stackName);

    static bool isCardStack(QGraphicsItem *);

    static void processCardMoveRecord(ProcessCardMoveRecordType type,
				      CardMoveRecord moveRecord);

    // this is a helper function that will show a move hint by highlighting the src widget starting with the
    // srcCardIndex in the stack for one second.  And then highlighting the last card in the dest stack for
    // one second.
    static void showHint(CardStack * pSrcWidget,unsigned int srcCardIndex,CardStack * pDstWidget);

public slots:
    // the slotDelayedHintHighlight() allows you to call slotHintHighlight() for the src stack
    // and slotDelayedHintHighlight() for the destination stack at the same time.
    void slotDelayedHintHighlight();  // show a hint highlight delayed
    void slotHintHighlight(int index=-1); // show the highlight hint immediately it will timeout in one second
    void slotHintHighlightComplete();

    void slotFlipComplete(CardStack * pSrc);

    void slotDragCardsMoved(const CardMoveRecord & moveRecord);

signals:
    // when a card is clicked this signal is emitted
    void cardClicked(CardStack * pCardStackWidget,unsigned int index);
    // This signal is emitted when there are no cards
    // in the stack and the pad area where the first card would be is clicked
    void padClicked(CardStack * pCardStackWidget);

    // this signal indicates that cards where clicked, but they are also movable
    // and it includes a move record of what would happen if the card was moved.  This
    // can be used to make the move by calling processCardMoveRecord.  If the move is accepted
    // the add move can be added to the move record.  And then processCardMoveRecord can be
    // called to perform the move.  The index is the index of the card that was clicked.
    void movableCardsClicked(CardStack * pCardStack,
			     const PlayingCardVector & cardVector,
			     const CardMoveRecord &);

    // the card record will contain all changes made by the drag and
    // drop. The remove from one stack.  The flip of the card if in
    // autoCardUp mode and the card under the remove cards was face
    // down.  And then the add of the cards to the other stack.
    // The signal will be emitted by the stack that started the drag
    // operation.
    void cardsMovedByDragDrop(const CardMoveRecord &);

protected:
    int                     m_hintHighlightIndex;

    // subclasses should reimplement canMoveCard and canAddCards
    // for the rules of whatever game they are implementing.
    virtual bool canMoveCard(unsigned int index) const{Q_UNUSED(index); return false;}
    virtual bool getCardIndex(const QPointF & pos,unsigned int & index);

    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

    virtual void hoverMoveEvent ( QGraphicsSceneHoverEvent * event );

private:
    QString                 m_stackName;            
    PlayingCardVector       m_cardVector;
    bool                    m_highlighted;
    bool                    m_showRedealCircle;
    bool                    m_autoFaceUp;
    bool                    m_mouseMoved;  // set to true if the mouse is moved while the button is down
                                           // using this to determine a mouse click on the CardStack.  The
                                           // mouse should not move.
    QPointF                 m_dragStartPos; // used to figure out when the mouse has been moved enough to
                                            // start a drag operation

    FlipAnimation           m_flipAni;
    DragCardStack           m_dragStack;

    ///////////////////////////////////////////////////////////////////////////////
    // static variables
    /////////////////////////////////////////////////////////////////////////////// 
    static CardStackMap             m_cardStackMap;  // this is a map that contains all of the current instances
                                                     // of CardStack.  They are mapped by a unique name to a 
                                                     // pointer to the instance.
    static bool                     m_lockUserInteraction;
};

#endif
