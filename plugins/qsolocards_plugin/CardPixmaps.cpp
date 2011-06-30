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

#include "CardPixmaps.h"
#include <QtGui/QPainter>
#include <QtGui/QImage>

const qreal CardPixmaps::CardWidthToHeight=.66;
const QString CardPixmaps::EmptyStackName("empty");
const QString CardPixmaps::EmptyStackRedealName("emtpy_redeal");
const QString CardPixmaps::CardBackName("back");
const QString CardPixmaps::TransparentNoCard("transparent");

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
CardPixmaps::~CardPixmaps()
{
    delete m_pSvgRendCard;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
CardPixmaps & CardPixmaps::getInst()
{
    static CardPixmaps cardPixmaps;

    return cardPixmaps;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void CardPixmaps::setCardWidth(unsigned int width)
{
    m_cardSize.setWidth(width);
    m_cardSize.setHeight(width/CardWidthToHeight);
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void CardPixmaps::setCardHeight(unsigned int height)
{
    m_cardSize.setHeight(height);
    m_cardSize.setWidth(height*CardWidthToHeight);
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
CardPixmaps::CardPixmaps()
    :m_pSvgRendCard(new QSvgRenderer(QString(":/images/anglo_bitmap.svg"))), 
     m_pixmapMap(),
     m_cardSize(0,0)
{
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
const QPixmap & CardPixmaps::getCardPixmap(const QString & imageName,bool highlighted)
{
    QString fullImageName("");

    if (highlighted)
    {
	fullImageName+="hl_";
    }
    fullImageName+=imageName;

    CardPixmapMap::iterator it=m_pixmapMap.find(fullImageName.toStdString());
    
    if (m_pixmapMap.end()!=it)
    {
        return it->second;
    }

    QPainter painter;

    QPixmap dummyPix(m_cardSize);

    dummyPix.fill(Qt::transparent);

    m_pixmapMap[fullImageName.toStdString()]=dummyPix;
    painter.begin(&m_pixmapMap[fullImageName.toStdString()]);
    if (imageName==EmptyStackName || imageName==EmptyStackRedealName)
    {
	drawEmptyStack(painter,highlighted,imageName==EmptyStackRedealName);
    }
    else if (imageName==TransparentNoCard)
    {
    }
    else if (highlighted)
    {
	drawHighlightedCard(painter,imageName);
    }
    else
    {    
	QRect pixRect(QPoint(0,0),m_cardSize);

        m_pSvgRendCard->render(&painter,imageName,pixRect);
    }

    painter.end();

    return m_pixmapMap[fullImageName.toStdString()];
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void CardPixmaps::drawEmptyStack(QPainter & painter,bool highlighted,bool redealCircle)
{
    QPoint topLeft(0,0);
    QPen pen;
    
    
    pen.setColor(QColor("#006520"));
    
    pen.setWidth(3);
    painter.setPen(pen);
    
    
    QRect rect(topLeft,m_cardSize);
    
    painter.setRenderHint(QPainter::Antialiasing,true);


    if (redealCircle)
    {
	QPoint ellipseCenter(rect.left()+rect.width()/2,
			     rect.top()+rect.height()/2);
	
	pen.setWidth(10);
	painter.setBrush(QColor("#006520"));
	painter.drawEllipse(ellipseCenter,
			    rect.width()/3,rect.width()/3);
    }

    painter.setBrush(Qt::transparent);

    // draw the highlight rect if necessary.
    if (highlighted)
    {
	painter.setBrush(QBrush(QColor("#806000"),Qt::Dense4Pattern));
    }

    painter.drawRoundedRect(rect.adjusted(1,1,-1,-1),6.0,6.0);    
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
void CardPixmaps::drawHighlightedCard(QPainter & painter,const QString & imageName)
{
    // for the highlighted case we are going to manipulate the original
    // image and change the background of the card to a yellowish color
    // to highlight it.

    // A QImage is necessary in Format_ARGB32_Premultiplied or Format_ARGB32
    // is required to use the composite modes.  So, draw the pixmap on the
    // QImage.  And then render the result in our pixmap.

    QRect pixRect(QPoint(0,0),m_cardSize);

    QImage  image(m_cardSize,QImage::Format_ARGB32_Premultiplied);

    image.fill(Qt::transparent);

    QPainter imagePainter;

    imagePainter.begin(&image);

    imagePainter.setPen(Qt::NoPen);

    imagePainter.setBrush(QColor("#ffff90"));
    imagePainter.drawPixmap(QPoint(0,0),getCardPixmap(imageName));

    imagePainter.setCompositionMode(QPainter::CompositionMode_Darken);
    imagePainter.drawRoundedRect(pixRect.adjusted(0,0,-1,-1),4.0,4.0);

    imagePainter.end();
    painter.drawImage(QPoint(0,0),image);
}
