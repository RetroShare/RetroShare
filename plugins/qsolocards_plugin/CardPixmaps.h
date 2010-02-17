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

#ifndef CARDPIXMAPS_H
#define CARDPIXMAPS_H

#include "PlayingCard.h"
#include <QtSvg/QSvgRenderer>
#include <QtGui/QPixmap>
#include <map>
#include <string>

typedef std::map<std::string,QPixmap>   CardPixmapMap;


class CardPixmaps
{
public:
    static const qreal CardWidthToHeight;  // This is the percentage of the height to the width
                                           // that is used to render a card.

    static const QString EmptyStackName;
    static const QString EmptyStackRedealName;
    static const QString CardBackName;
    static const QString TransparentNoCard;

    ~CardPixmaps();
    
    static CardPixmaps & getInst();


    void setCardWidth(unsigned int width);
    void setCardHeight(unsigned int height);

    inline const QSize & getCardSize() {return m_cardSize;}

    inline void clearPixmapCache(){m_pixmapMap.clear();}

    // functions to make it easier to get card pixmaps.
    const QPixmap & getCardPixmap(const PlayingCard & card,bool highlighted=false)
    {return getCardPixmap(card.asString(),highlighted);}

    const QPixmap & getCardBackPixmap(bool highlighted=false)
    {return getCardPixmap(CardBackName,highlighted);}

    const QPixmap & getTransparentPixmap()
    {return getCardPixmap(TransparentNoCard);}

    // the redeal circle is just a circle in the middle of the empty stack that indicates that
    // you can d-click on the stack to recycle the cards.  Used in games like Klondike where
    // can go through the flip stack multiple times
    const QPixmap & getCardNonePixmap(bool highlighted=false,bool redealCircle=false)
    {return getCardPixmap(redealCircle?EmptyStackRedealName:EmptyStackName,highlighted);}


protected:
    CardPixmaps();

private:
    const QPixmap & getCardPixmap(const QString & imageName, bool highlighted=false);
    void drawEmptyStack(QPainter & painter,bool highlighted,bool redealCircle);
    void drawHighlightedCard(QPainter & painter,const QString & imageName);


    QSvgRenderer   *         m_pSvgRendCard;  // the svg render will be created the first time
                                              // it is needed.  It has to be done after Qt is initialized.
                                              // And we just want to create it once. Loading it is fairly 
                                              // expensive.
    CardPixmapMap            m_pixmapMap;     // this is a map of cards that is built as a card is needed.
                                              // It will also contain the card back.  And highlighted 
                                              // versions of cards.
    QSize                    m_cardSize;      // this is the card size that is used for every card
                                              // that is drawn.  The height of a card is determined
                                              // from the width.  So, the value is set by calling 
                                              // setCardWidth.
};

#endif
