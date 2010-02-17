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

#include "CardDeck.h"

#include <QtCore/QDateTime>


///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
CardDeck::CardDeck(unsigned short numDecks)
        :m_deckOfCards(),
         m_shuffledCardVector(),
         m_numDecks(numDecks)
{
    // This creates the default deck of cards that will be used
    for (unsigned short i=0;i<m_numDecks;i++)
    {
        for(unsigned short currSuit=0;currSuit<PlayingCard::MaxSuit;currSuit++)
        {
            for(unsigned short currCard=0;currCard<PlayingCard::MaxCardIndex;currCard++)
            {
                PlayingCard newCard((PlayingCard::Suit)currSuit,(PlayingCard::CardIndex)currCard);

                m_deckOfCards.push_back(newCard);
            }
        }
    }


    this->shuffle();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
CardDeck::CardDeck(const PlayingCardVector & cardsForDeck, unsigned short numDecks)
        :m_deckOfCards(),
         m_shuffledCardVector(),
         m_numDecks(numDecks)
{
    for (unsigned short i=0;i<m_numDecks;i++)
    {
        for (unsigned short j=0;j<cardsForDeck.size();j++)
        {
            this->m_deckOfCards.push_back(cardsForDeck[j]);
        }
    }

    this->shuffle();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
CardDeck::~CardDeck()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Create a shuffled deck by generating a vector of playing cards.  And then randomly generating an index
// into that vector.  And then pull the card out of that vector and adding it to the end of another vector.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void CardDeck::shuffle()
{
    PlayingCardVector cardVector;

    cardVector=this->m_deckOfCards;

    if (this->m_shuffledCardVector.size()>0)
    {
        this->m_shuffledCardVector.clear();
    }

    qsrand(QDateTime::currentDateTime().toTime_t());


    while(cardVector.size()>0)
    {
        unsigned int index=qrand()%cardVector.size();

        // make sure our random number is not off the end of the vector
        if (index>=cardVector.size())
        {
            index=cardVector.size()-1;
        }

        this->m_shuffledCardVector.push_back(cardVector[index]);
        cardVector.erase(cardVector.begin()+index);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
PlayingCard CardDeck::next()
{
    // create an invalid card.
    PlayingCard playingCard(PlayingCard::MaxSuit,PlayingCard::MaxCardIndex);

    // if we have any cards left get the first one and return it.
    // then remove that card from the vector.
    if (this->m_shuffledCardVector.size()>0)
    {
        playingCard=this->m_shuffledCardVector[0];
        this->m_shuffledCardVector.erase(this->m_shuffledCardVector.begin());
    }

    return playingCard;
}
