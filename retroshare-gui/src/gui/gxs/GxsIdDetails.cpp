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

#include "GxsIdDetails.h"

#include <retroshare/rspeers.h>

#include <iostream>
#include <QPainter>
#include <QObject>


/* Images for tag icons */
#define IMAGE_LOADING     ":/images/folder-draft.png"
#define IMAGE_PGPKNOWN    ":/images/vote_up.png"
#define IMAGE_PGPUNKNOWN  ":/images/vote_up.png"
#define IMAGE_ANON        ":/images/vote_down.png"

#define IMAGE_DEV_AMBASSADOR     ":/images/vote_down.png"
#define IMAGE_DEV_CONTRIBUTOR    ":/images/vote_down.png"
#define IMAGE_DEV_TRANSLATOR     ":/images/vote_down.png"
#define IMAGE_DEV_PATCHER        ":/images/vote_down.png"
#define IMAGE_DEV_DEVELOPER      ":/images/vote_down.png"

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


static bool CreateIdIcon(const std::string &id, QIcon &idIcon)
{
	QPixmap image(IconSize, IconSize);
	QPainter painter(&image);

	painter.fillRect(0, 0, IconSize, IconSize, Qt::black);

	int len = id.length();
	for(int i = 0; i + 1 < len; i += 2)
	{
		char hex1 = id[i];
		char hex2 = id[i+1];
		int x = (hex1 >= 'a') ? (hex1 - 'a' + 10) : (hex1 - '0');
		int y = (hex2 >= 'a') ? (hex2 - 'a' + 10) : (hex2 - '0');
		painter.fillRect(x, y, x+1, y+1, Qt::green);
	}
	idIcon = QIcon(image);
	return true;
}


bool GxsIdDetails::MakeIdDesc(const RsGxsId &id, bool doIcons, QString &str, std::list<QIcon> &icons)
{
	RsIdentityDetails details;
	
	if (!rsIdentity->getIdDetails(id, details))
	{
		std::cerr << "GxsIdTreeWidget::MakeIdDesc() FAILED TO GET ID";
		std::cerr << std::endl;

		str = QObject::tr("Loading... ") + QString::fromStdString(id.substr(0,5));

		if (!doIcons)
		{
			QIcon baseIcon = QIcon(IMAGE_LOADING);
			icons.push_back(baseIcon);
		}

		return false;
	}

	str = QString::fromUtf8(details.mNickname.c_str());

	std::list<RsRecognTag>::iterator it;
	for(it = details.mRecognTags.begin(); it != details.mRecognTags.end(); it++)
	{
		str += " (";
		str += QString::number(it->tag_class);
		str += ":";
		str += QString::number(it->tag_type);
		str += ")";
	}


	bool addCode = true;
	if (details.mPgpLinked)
	{
		str += " (PGP) [";
		if (details.mPgpKnown)
		{
			/* look up real name */
			std::string authorName = rsPeers->getPeerName(details.mPgpId);
			str += QString::fromUtf8(authorName.c_str());
			str += "]";

			addCode = false;
		}
	}
	else
	{
		str += " (Anon) [";
	}

	if (addCode)
	{
		str += QString::fromStdString(id.substr(0,5));
		str += "...]";
	}

	if (!doIcons)
	{
		return true;
	}

	QIcon idIcon;
	CreateIdIcon(id, idIcon);
	icons.push_back(idIcon);

	// ICON Logic.
	QIcon baseIcon;
	if (details.mPgpLinked)
	{
		if (details.mPgpKnown)
		{
			baseIcon = QIcon(IMAGE_PGPKNOWN);
		}
		else
		{
			baseIcon = QIcon(IMAGE_PGPUNKNOWN);
		}
	}
	else
	{
		baseIcon = QIcon(IMAGE_ANON);
	}

	icons.push_back(baseIcon);
	// Add In RecognTags Icons.
	for(it = details.mRecognTags.begin(); it != details.mRecognTags.end(); it++)
	{
		QIcon tagIcon;
		if (findTagIcon(it->tag_class, it->tag_type, tagIcon))
		{
			icons.push_back(tagIcon);
		}
	}

	icons.push_back(QIcon(IMAGE_ANON));
	icons.push_back(QIcon(IMAGE_ANON));
	icons.push_back(QIcon(IMAGE_ANON));

	std::cerr << "GxsIdTreeWidget::MakeIdDesc() ID Ok";
	std::cerr << std::endl;

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
	for(it = icons.begin(); it != icons.end(); it++, i++)
	{
        	it->paint(&painter, IconSize * i, 0, IconSize, IconSize);
	}

	outIcon = QIcon(image);
	return true;
}

