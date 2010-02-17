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

#ifndef SPIDERBOARD_H
#define SPIDERBOARD_H

#include "GameBoard.h"
#include "CardStack.h"
#include "SpiderStack.h"
#include "SpiderHomeStack.h"

#include <vector>

class SpiderBoard : public GameBoard
{
    Q_OBJECT

    public:
        enum GameType
        {
            FourSuits=4,
            TwoSuits=2,
            OneSuit=1
        };

        static const QString  GameTypeKeyStr;

        SpiderBoard(QWidget * pParent=NULL);
        virtual ~SpiderBoard();

        bool getHint(CardStack * & pSrcWidget,
		     unsigned int & srcStackIndex,
		     CardStack * & pDstWidget);

        inline bool hasDemo() const {return true;}

        virtual void newGame();

        virtual void addGameMenuItems(QMenu & menu);

        virtual void loadSettings(const QSettings & settings);
        virtual void saveSettings(QSettings & settings);

        inline bool isCheating() const {return m_cheat;}

        void setCheat(bool cheat);

        inline bool supportsScore() const{return true;}

    public slots:
        virtual void slotDealNextCards(CardStack * pCardStackWidget=NULL,unsigned int index=0);

        // these slot catch the signals from the SpiderStacks that are started by
        // a click on a card instead of a drag
        virtual void slotSendSuitHome(SpiderStack *,const PlayingCardVector &,const CardMoveRecord &);
        virtual void slotMoveCardsToDiffStack(SpiderStack *,const PlayingCardVector &,const CardMoveRecord &);


        void slotSetFourSuits();
        void slotSetTwoSuits();
        void slotSetOneSuit();

    protected:
        void calcScore();

	virtual void resizeEvent (QResizeEvent * event);

	bool runDemo(bool stopWhenNoMore=true);

        virtual void createStacks();

	virtual bool isGameWon()const;
	virtual bool isGameWonNotComplete()const;

        CardStack  *    m_pDeck;
        std::vector<SpiderHomeStack *>  m_homeVector;  // we will have 8 items in this 
                                                 //vector one for each suit (2 decks)
        std::vector<SpiderStack *> m_stackVector;  // we will have 10 items in this vector

        bool m_cheat;


        GameType   m_gameType;
};

#endif // SPIDERBOARD_H
