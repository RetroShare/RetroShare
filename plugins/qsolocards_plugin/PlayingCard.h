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

#ifndef PLAYINGCARD_H
#define PLAYINGCARD_H

class PlayingCard
{
    public:
    enum Suit
    {
        Clubs=0,
        Diamonds=1,
        Hearts=2,
        Spades=3,
        MaxSuit=4
    };


    enum CardIndex
    {
        Ace=0,
        Two=1,
        Three=2,
        Four=3,
        Five=4,
        Six=5,
        Seven=6,
        Eight=7,
        Nine=8,
        Ten=9,
        Jack=10,
        Queen=11,
        King=12,
        MaxCardIndex=13
    };

    PlayingCard(Suit suit=PlayingCard::MaxSuit,
		CardIndex index=PlayingCard::MaxCardIndex,
		bool isFaceUp=false);
    PlayingCard(const PlayingCard & playingCard);

    virtual ~PlayingCard();

    // a value to uniquely id the card
    unsigned short hashValue() const;
    static PlayingCard cardFromHashValue(unsigned short value);

    Suit getSuit() const {return m_suit;}
    CardIndex getIndex() const {return m_index;}

    inline bool isValid() const{return (MaxSuit!=m_suit && MaxCardIndex!=m_index);}

    inline bool isSameSuit(const PlayingCard & cmp) const{return cmp.getSuit()==m_suit;}
    inline bool isSameIndex(const PlayingCard & cmp) const{return cmp.getIndex()==m_index;}

    inline bool isRed() const{return ((PlayingCard::Diamonds==m_suit || PlayingCard::Hearts==m_suit)?true:false);}
    inline bool isBlack() const{return ((PlayingCard::Spades==m_suit || PlayingCard::Clubs==m_suit)?true:false);}

    inline bool isFaceUp() const {return m_isFaceUp;}
    inline void setFaceUp(bool state=true) {m_isFaceUp=state;}


    inline bool operator==(const PlayingCard & rh) const {return (isSameSuit(rh) && isSameIndex(rh));}

    PlayingCard & operator=(const PlayingCard & rh);

    virtual bool operator<(const PlayingCard & rh) const;
    virtual bool operator>(const PlayingCard & rh) const;

    virtual bool isNextCardIndex(const PlayingCard & rh) const;
    virtual bool isPrevCardIndex(const PlayingCard & rh) const;

    inline const char * asString() const{return m_textStr;}

    private:

    void setTextStr();

    Suit m_suit;
    CardIndex m_index;
    bool m_isFaceUp;
    char * m_textStr;
};
#endif // PLAYINGCARD_H
