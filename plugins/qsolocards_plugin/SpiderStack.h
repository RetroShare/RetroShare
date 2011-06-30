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

#ifndef SPIDERSTACK_H
#define SPIDERSTACK_H

#include "VCardStack.h"

// This is the stack of cards that are used for the main game play
// There are ten of these stacks.
class SpiderStack: public VCardStack
{
    Q_OBJECT

    public:
        SpiderStack();
        ~SpiderStack();

        inline bool isCheating() const{return m_cheat;}

        inline void setCheat(bool cheat=true){m_cheat=cheat;}

        int score() const;

        bool canAddCards(const PlayingCardVector &);

        // similar to can Add cards.  But in this case the
        // suit of the last card in this stack is the same as
        // the first suit of the cardVector
        bool canAddCardsSameSuit(const PlayingCardVector &);

    signals:
        // signals that are emitted when a card that can be moved is clicked on
        // the move record created will have the Remove and flip actions (if the flip of the
        // card before the cards is necessary.)
        void sendSuitHome(SpiderStack *,const PlayingCardVector &,const CardMoveRecord &);
        void moveCardsToDiffStack(SpiderStack *,const PlayingCardVector &,const CardMoveRecord &);

    public slots:
        void slotMovableCardsClicked(CardStack * pCardStack,
				     const PlayingCardVector &,
				     const CardMoveRecord &);


    protected:
        bool canMoveCard(unsigned int index) const;

    private:
        bool m_cheat;
};

#endif // SPIDERSTACK_H
