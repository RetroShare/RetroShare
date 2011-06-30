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

#ifndef VCARDSTACK_H
#define VCARDSTACK_H

#include "CardStack.h"
#include <vector>
#include <QtCore/QRectF>

typedef std::vector<QRectF>  CardBRectVect;


// this class will show cards stacked but all cards 
// will be partly visible with a card on top of a card revealing
// a portion of the top of the card beneath it.
class VCardStack: public CardStack
{
    Q_OBJECT
public:
    static const qreal ExposedPrecentFaceUp;   // the precentage show for a face up card will be more than
                                               // that for a card that is face down.  Since, we want to be
                                               // able to see the value of the card if it is face up.
    static const qreal ExposedPrecentFaceDown; 

    enum 
    {
	CompressNormal=1,
	CompressMax=4
    };

    VCardStack();
    virtual ~VCardStack();


    // this function allows the caller to set the preportion of the screen that the 
    // stack can decend too.  For example, 1 is the default which means to the bottom
    // of the scene. To make the bottom 3/4 of the scene use .75 etc... 1 is the maximum
    // value.
    void setStackBottom(qreal percentScene);
    qreal stackBottom() const{return m_percentScene;}

    // this function gets the point in the scene that a card would be added to this stack.
    virtual QPointF getGlobalCardAddPt() const;
    virtual QPointF getGlobalLastCardPt() const;
    virtual QPointF getGlobalCardPt(int index) const;

    virtual void updateStack();

    virtual QPixmap * getStackPixmap(const PlayingCardVector & cardVector,
				     bool highlighted=false,
				     int hintHighlightIndex=-1);
protected:
    virtual bool getCardIndex(const QPointF & pos,unsigned int & index);

    void calcPixmapSize(const PlayingCardVector & cardVector,
			QSize & size,qreal compressValue);

    virtual unsigned int getOverlapIncrement(const PlayingCard & card,qreal compressValue) const;

    virtual QPixmap * getStackPixmap(const PlayingCardVector & cardVector,
				     bool highlighted,
				     int hintHighlightIndex,
				     qreal compressValue,
				     CardBRectVect * pBRectVector);

private:
    CardBRectVect  m_bRectVector;

    qreal  m_compressValue;
    qreal  m_percentScene;
};
#endif
