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

#ifndef __YUKONBOARD_H__
#define __YUKONBOARD_H__

#include "GameBoard.h"
#include "FreeCellHome.h"
#include "FreeCellDeck.h"
#include "KlondikeStack.h"

#include <vector>


class YukonBoard : public GameBoard
{
    Q_OBJECT
public:
    YukonBoard();
    ~YukonBoard();

    bool getHint(CardStack * & pSrc,
		 unsigned int & srcStackIndex,
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
    void slotStackCardsClicked(CardStack * pCardStack,
			       const PlayingCardVector & cardVector,
			       const CardMoveRecord &);

protected:
    void calcScore();

    virtual void resizeEvent (QResizeEvent * event);

    virtual void createStacks();

    bool isGameWon()const;
    bool isGameWonNotComplete()const;

private:
    FreeCellDeck  *    m_pDeck;

    std::vector<FreeCellHome *>  m_homeVector;  // we will have 4 items in this vector one for each suit
    std::vector<KlondikeStack *> m_stackVector; // we will have 7 items in this vector

    bool m_cheat;
};

#endif // YUKONBOARD_H
