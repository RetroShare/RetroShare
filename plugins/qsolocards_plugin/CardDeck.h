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

#ifndef CARDDECK_H
#define CARDDECK_H

#include <vector>

#include "PlayingCard.h"

typedef std::vector<PlayingCard> PlayingCardVector;

class CardDeck
{
    public:
    // by default we just want to include one complete deck.
    CardDeck(unsigned short numDecks=1);

    // this constructor adds a little flexibility.  So, that a deck of only certain cards
    // can be built easily.
    CardDeck(const PlayingCardVector & cardsForDeck, unsigned short numDecks=1);
    ~CardDeck();

    // will clear any remaining cards in the shuffled deck and create a new shuffled deck
    // this function is automatically called by the constructor.
    void shuffle();

    inline bool isEmpty() const {return m_shuffledCardVector.empty();}

    // get the next card in the shuffled deck.
    PlayingCard next();

    private:
    PlayingCardVector  m_deckOfCards;
    PlayingCardVector  m_shuffledCardVector;

    unsigned short     m_numDecks;
};


#endif // CARDDECK_H
