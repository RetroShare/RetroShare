/*
 * Retroshare Gxs Support
 *
 * Copyright 2012-2013 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include <math.h>
#include "GxsIdDetails.h"

#include <retroshare/rspeers.h>

#include <iostream>
#include <QPainter>
#include <QIcon>
#include <QObject>


/* Images for tag icons */
#define IMAGE_LOADING     ":/images/folder-draft.png"
#define IMAGE_PGPKNOWN    ":/images/tags/pgp-known.png"
#define IMAGE_PGPUNKNOWN  ":/images/tags/pgp-unknown.png"
#define IMAGE_ANON        ":/images/tags/anon.png"

#define IMAGE_DEV_AMBASSADOR     ":/images/tags/dev-ambassador.png"
#define IMAGE_DEV_CONTRIBUTOR    ":/images/tags/vote_down.png"
#define IMAGE_DEV_TRANSLATOR     ":/images/tags/dev-translator.png"
#define IMAGE_DEV_PATCHER        ":/images/tags/dev-patcher.png"
#define IMAGE_DEV_DEVELOPER      ":/images/tags/developer.png"

static const int IconSize = 20;

const int kRecognTagClass_DEVELOPMENT = 1;

const int kRecognTagType_Dev_Ambassador 	= 1;
const int kRecognTagType_Dev_Contributor 	= 2;
const int kRecognTagType_Dev_Translator 	= 3;
const int kRecognTagType_Dev_Patcher     	= 4;
const int kRecognTagType_Dev_Developer	 	= 5;


static bool findTagIcon(int tag_class, int tag_type, QIcon &icon)
{
	switch(tag_class)
	{
		default:
		case 0:
			icon = QIcon(IMAGE_DEV_AMBASSADOR);
			break;
		case 1:
			icon = QIcon(IMAGE_DEV_CONTRIBUTOR);
			break;
	}
	return true;
}

QImage GxsIdDetails::makeDefaultIcon(const RsGxsId& id)
{
    static std::map<RsGxsId,QImage> image_cache ;

    std::map<RsGxsId,QImage>::const_iterator it = image_cache.find(id) ;

    if(it != image_cache.end())
        return it->second ;

    int S = 128 ;
    QImage pix(S,S,QImage::Format_RGB32) ;

    uint64_t n = reinterpret_cast<const uint64_t*>(id.toByteArray())[0] ;

    uint8_t a[8] ;
    for(int i=0;i<8;++i)
    {
        a[i] = n&0xff ;
        n >>= 8 ;
    }
    QColor val[16] = {
            QColor::fromRgb( 255, 110, 180),
            QColor::fromRgb( 238,  92,  66),
            QColor::fromRgb( 255, 127,  36),
            QColor::fromRgb( 255, 193, 193),
            QColor::fromRgb( 127, 255, 212),
            QColor::fromRgb(   0, 255, 255),
            QColor::fromRgb( 224, 255, 255),
            QColor::fromRgb( 199,  21, 133),
            QColor::fromRgb(  50, 205,  50),
            QColor::fromRgb( 107, 142,  35),
            QColor::fromRgb(  30, 144, 255),
            QColor::fromRgb(  95, 158, 160),
            QColor::fromRgb( 143, 188, 143),
            QColor::fromRgb( 233, 150, 122),
            QColor::fromRgb( 151, 255, 255),
            QColor::fromRgb( 162, 205,  90),
    };

    int c1 = (a[0]^a[1]) & 0xf ;
    int c2 = (a[1]^a[2]) & 0xf ;
    int c3 = (a[2]^a[3]) & 0xf ;
    int c4 = (a[3]^a[4]) & 0xf ;

    for(int i=0;i<S/2;++i)
        for(int j=0;j<S/2;++j)
        {
            float res1 = 0.0f ;
            float res2 = 0.0f ;
            float f = 1.70;

            for(int k1=0;k1<4;++k1)
                for(int k2=0;k2<4;++k2)
                {
                    res1 += cos( (2*M_PI*i/(float)S) * k1 * f) * (a[k1  ] & 0xf) + sin( (2*M_PI*j/(float)S) * k2 * f) * (a[k2  ] >> 4) + sin( (2*M_PI*i/(float)S) * k1 * f) * cos( (2*M_PI*j/(float)S) * k2 * f) * (a[k1+k2] >> 4) ;
                    res2 += cos( (2*M_PI*i/(float)S) * k2 * f) * (a[k1+2] & 0xf) + sin( (2*M_PI*j/(float)S) * k1 * f) * (a[k2+1] >> 4) + sin( (2*M_PI*i/(float)S) * k2 * f) * cos( (2*M_PI*j/(float)S) * k1 * f) * (a[k1^k2] >> 4) ;
                }

            uint32_t q = 0 ;
            if(res1 >= 0.0f) q += val[c1].rgb() ; else q += val[c2].rgb() ;
            if(res2 >= 0.0f) q += val[c3].rgb() ; else q += val[c4].rgb() ;

            pix.setPixel( i, j, q) ;
            pix.setPixel( S-1-i, j, q) ;
            pix.setPixel( S-1-i, S-1-j, q) ;
            pix.setPixel(     i, S-1-j, q) ;
        }

    image_cache[id] = pix.scaled(64,64,Qt::KeepAspectRatio,Qt::SmoothTransformation) ;

    return image_cache[id] ;
}


static bool CreateIdIcon(const RsGxsId &id, QIcon &idIcon)
{
	QPixmap image(IconSize, IconSize);
	QPainter painter(&image);

	painter.fillRect(0, 0, IconSize, IconSize, Qt::black);

    int len = id.SIZE_IN_BYTES;
    for(int i = 0; i<len; ++i)
	{
        unsigned char hex = id.toByteArray()[i];
        int x = hex & 0xf ;
        int y = (hex & 0xf0) >> 4 ;
		painter.fillRect(x, y, x+1, y+1, Qt::green);
	}
	idIcon = QIcon(image);
	return true;
}

bool GxsIdDetails::MakeIdDesc(const RsGxsId &id, bool doIcons, QString &str, std::list<QIcon> &icons,QString& comment)
{
	RsIdentityDetails details;
	
	if (!rsIdentity->getIdDetails(id, details))
	{
       // std::cerr << "GxsIdTreeWidget::MakeIdDesc() FAILED TO GET ID " << id;
		//std::cerr << std::endl;

        str = QObject::tr("Loading... ") + QString::fromStdString(id.toStdString().substr(0,5));

		if (!doIcons)
		{
			QIcon baseIcon = QIcon(IMAGE_LOADING);
			icons.push_back(baseIcon);
		}

		return false;
	}

	str = QString::fromUtf8(details.mNickname.c_str());

	std::list<RsRecognTag>::iterator it;
	for(it = details.mRecognTags.begin(); it != details.mRecognTags.end(); ++it)
	{
		str += " (";
		str += QString::number(it->tag_class);
		str += ":";
		str += QString::number(it->tag_type);
		str += ")";
	}

	comment += "Identity name: " + QString::fromUtf8(details.mNickname.c_str()) + "\n";
    comment += "Identity Id  : " + QString::fromStdString(id.toStdString()) + "\n";

	if (details.mPgpLinked)
	{
		comment += "Authentication: signed by " ;

		if (details.mPgpKnown)
		{
			/* look up real name */
			std::string authorName = rsPeers->getGPGName(details.mPgpId);
			comment += QString::fromUtf8(authorName.c_str());
			comment += " [";
			comment += QString::fromStdString(details.mPgpId.toStdString()) ;
			comment += "]";
		}
		else
			comment += QObject::tr("unknown Key") ;
	}
	else
		comment += "Authentication: anonymous" ;

	if (!doIcons)
		return true;

    QPixmap pix ;
    pix.convertFromImage( makeDefaultIcon(id) );
    QIcon idIcon( pix ) ;
    //CreateIdIcon(id, idIcon);
	icons.push_back(idIcon);

	// ICON Logic.
	QIcon baseIcon;
	if (details.mPgpLinked)
	{
		if (details.mPgpKnown)
			baseIcon = QIcon(IMAGE_PGPKNOWN);
		else
			baseIcon = QIcon(IMAGE_PGPUNKNOWN);
	}
	else
		baseIcon = QIcon(IMAGE_ANON);

	icons.push_back(baseIcon);
	// Add In RecognTags Icons.
	for(it = details.mRecognTags.begin(); it != details.mRecognTags.end(); ++it)
	{
		QIcon tagIcon;
		if (findTagIcon(it->tag_class, it->tag_type, tagIcon))
		{
			icons.push_back(tagIcon);
		}
	}

//	Cyril: I disabled these three which I believe to have been put for testing purposes.
//
//	icons.push_back(QIcon(IMAGE_ANON));
//	icons.push_back(QIcon(IMAGE_ANON));
//	icons.push_back(QIcon(IMAGE_ANON));

//	std::cerr << "GxsIdTreeWidget::MakeIdDesc() ID Ok. Comment: " << comment.toStdString() ;
//	std::cerr << std::endl;

	return true;
}

bool GxsIdDetails::GenerateCombinedIcon(QIcon &outIcon, std::list<QIcon> &icons)
{
	int count = icons.size();
	QPixmap image(IconSize * count, IconSize);
	QPainter painter(&image);

	painter.fillRect(0, 0, IconSize * count, IconSize, Qt::transparent);
	std::list<QIcon>::iterator it;
	int i = 0;
	for(it = icons.begin(); it != icons.end(); ++it, ++i)
	{
        	it->paint(&painter, IconSize * i, 0, IconSize, IconSize);
	}

	outIcon = QIcon(image);
	return true;
}

