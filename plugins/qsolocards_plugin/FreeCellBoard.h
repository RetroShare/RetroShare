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

#ifndef __FREECELLBOARD_H__
#define __FREECELLBOARD_H__

#include "GameBoard.h"
#include "FreeCellDeck.h"
#include "FreeCellFree.h"
#include "FreeCellStack.h"
#include "FreeCellHome.h"
#include "VCardStack.h"

#include <vector>

class FreeCellBoard: public GameBoard
{
    Q_OBJECT
public:
    FreeCellBoard();
    virtual ~FreeCellBoard();

    virtual void undoMove();
    virtual void redoMove();

    bool getHint(CardStack * & pSrc,
		 unsigned int & srcIndex,
		 CardStack * & pDst);

    inline bool hasDemo() const {return true;}

    void newGame();

    void addGameMenuItems(QMenu & menu);

    void loadSettings(const QSettings & settings);
    void saveSettings(QSettings & settings);

    inline bool isCheating() const {return m_cheat;}

    void setCheat(bool cheat);

    inline bool supportsScore() const{return true;}
public slots:
    virtual void slotCardsMoved(const CardMoveRecord &);

    void slotFreeCardsClicked(CardStack * pCardStackWidget,
			      const PlayingCardVector & cardVector,
			      const CardMoveRecord &);
    void slotStackCardsClicked(CardStack * pCardStackWidget,
			       const PlayingCardVector & cardVector,
			       const CardMoveRecord &);

protected:
    void calcScore();

    virtual void resizeEvent (QResizeEvent * event);

    bool runDemo(bool stopWhenNoMore=true);

    void createStacks();
    bool isGameWon()const;
    bool isGameWonNotComplete()const;

    void setNumStackMoveCards();

private:

    FreeCellDeck  *    m_pDeck;  // the deck will only be used for the initial deal of cards

    std::vector<FreeCellFree *>  m_freeVector;  // free cells that any card can be placed in there will be 4
    std::vector<FreeCellStack *>  m_stackVector; // we will have 8 items in this vector
    std::vector<FreeCellHome *>  m_homeVector;  // we will have 4 items in this vector one for each suit

    bool m_cheat;
};

#endif
