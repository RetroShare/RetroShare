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

#ifndef SPIDERHOMESTACK_H
#define SPIDERHOMESTACK_H

#include "CardStack.h"

// When a suit is together from King to Ace it can be "sent home" or moved off the
// main board to a separate set stack.  When all 8 sets of suits are "sent home" the
// game is won.
class SpiderHomeStack: public CardStack
{
    public:
        SpiderHomeStack();
        ~SpiderHomeStack();

        // this function just tests if the cardVector is complete
        // King to Ace of the same suit
        static bool canSendHome(const PlayingCardVector &);

        // for this type of stack the score will be 0 or 12
        // or one less than the number of cards in a suit.
        // ie we have a sent home suit or nothing.
        inline int score() const {return ((isEmpty())?0:PlayingCard::MaxCardIndex-1);}

        bool canAddCards(const PlayingCardVector &);

};

#endif // SPIDERHOMESTACK_H
