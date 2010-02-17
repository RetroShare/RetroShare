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

#ifndef KLONDIKEBOARD_H
#define KLONDIKEBOARD_H

#include "GameBoard.h"
#include "KlondikeStack.h"
#include "KlondikeHomeStack.h"
#include "KlondikeFlipStack.h"
#include "CardStack.h"
#include "StackToStackFlipAni.h"

#include <vector>


class KlondikeBoard : public GameBoard
{
    Q_OBJECT
public:
    enum GameType
    {
        NoRedeals=0,
        FlipOne=1,
        FlipThree=3
    };

    static const QString  GameTypeKeyStr;

    KlondikeBoard(QWidget * pParent=NULL);
    ~KlondikeBoard();

    virtual void undoMove();
    virtual void redoMove();

    void restartGame();

    bool getHint(CardStack * & pSrcWidget,
		 unsigned int & srcStackIndex,
		 CardStack * & pDstWidget);

    inline bool hasDemo() const {return true;}
	
    void newGame();

    void addGameMenuItems(QMenu & menu);

    void loadSettings(const QSettings & settings);
    void saveSettings(QSettings & settings);

    inline bool isCheating() const {return m_cheat;}

    void setCheat(bool cheat);

    inline bool supportsScore() const{return true;}

public slots:
    void slotFlipCards(CardStack * pCardStackWidget=NULL,unsigned int index=0);
    void slotRedealCards(CardStack * pCardStackWidget=NULL);

    void slotStackCardsClicked(CardStack * pCardStackWidget,
			       const PlayingCardVector & cardVector,
			       const CardMoveRecord &);
    void slotHomeCardsClicked(CardStack * pCardStackWidget,
			      const PlayingCardVector & cardVector,
			      const CardMoveRecord &);

    void slotSetNoRedeals();
    void slotSetFlipOne();
    void slotSetFlipThree();

protected:
    void calcScore();

    virtual void resizeEvent (QResizeEvent * event);

    bool runDemo(bool stopWhenNoMore=true);

    virtual void createStacks();

    bool isGameWon()const;
    bool isGameWonNotComplete()const;

private:

    CardStack  *    m_pDeck;
    KlondikeFlipStack  *    m_pFlipDeck;

    std::vector<KlondikeHomeStack *>  m_homeVector;  // we will have 4 items in this vector one for each suit
    std::vector<KlondikeStack *> m_stackVector; // we will have 7 items in this vector

    bool m_cheat;

    GameType                 m_gameType;
    unsigned int             m_flipNum;

    StackToStackFlipAni   m_sToSFlipAni;
};

#endif // KLONDIKEBOARD_H
