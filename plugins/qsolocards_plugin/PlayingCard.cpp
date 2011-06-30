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

#include "PlayingCard.h"

#include <stdio.h>
#include <string.h>
#include <string>

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
PlayingCard::PlayingCard(PlayingCard::Suit suit,
                         PlayingCard::CardIndex index,
                         bool isFaceUp)
    :m_suit(suit),
     m_index(index),
     m_isFaceUp(isFaceUp),
     m_textStr(NULL)
{
    this->setTextStr();
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
PlayingCard::PlayingCard(const PlayingCard & playingCard)
    :m_textStr(NULL)
{
    *this=playingCard;
}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
PlayingCard::~PlayingCard()
{
    delete []m_textStr;
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
unsigned short PlayingCard::hashValue() const
{
    return m_suit*MaxCardIndex + m_index;
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
PlayingCard PlayingCard::cardFromHashValue(unsigned short value)
{
    PlayingCard card(PlayingCard::MaxSuit,PlayingCard::MaxCardIndex);
    // if the value is valid set the suit and index
    if (value<PlayingCard::MaxSuit*PlayingCard::MaxCardIndex)
    {
        card=PlayingCard((PlayingCard::Suit)(value/MaxCardIndex),
                         (PlayingCard::CardIndex)(value%MaxCardIndex));
    }
    return card;
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
bool PlayingCard::operator<(const PlayingCard & rh) const
{
    return m_index<rh.getIndex();
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
bool PlayingCard::operator>(const PlayingCard & rh) const
{
    return m_index>rh.getIndex();
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
PlayingCard & PlayingCard::operator=(const PlayingCard & rh)
{
    m_suit=rh.getSuit();
    m_index=rh.getIndex();
    this->setFaceUp(rh.isFaceUp());
    setTextStr();
    return *this;
}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
bool PlayingCard::isNextCardIndex(const PlayingCard & rh) const
{
    return m_index+1==rh.getIndex();
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
bool PlayingCard::isPrevCardIndex(const PlayingCard & rh) const
{
    return m_index-1==rh.getIndex();
}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
void PlayingCard::setTextStr()
{
    std::string cardValue;

    switch (m_index)
    {
        case Ace:
            cardValue="1";
            break;
        case Jack:
            cardValue="jack";
            break;
        case Queen:
            cardValue="queen";
            break;
        case King:
            cardValue="king";
            break;
        case MaxCardIndex:
            cardValue="invalid";
            break;
        default:
            {
                char cardNumStr[20];
                sprintf(cardNumStr,"%i",(int)m_index+1);
                cardValue=cardNumStr;
            };
            break;
    };

    cardValue+="_";

    switch (m_suit)
    {
        case Clubs:
            cardValue+="club";
            break;
        case Spades:
            cardValue+="spade";
            break;
        case Hearts:
            cardValue+="heart";
            break;
        case Diamonds:
            cardValue+="diamond";
            break;
        default:
            cardValue+="Invalid Suit";
            break;
    };

    delete []m_textStr;

    this->m_textStr=new char[cardValue.size()+1];

    strcpy(this->m_textStr,cardValue.c_str());
}
