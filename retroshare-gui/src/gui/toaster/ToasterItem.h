/*******************************************************************************
 * gui/toaster/ToasterItem.h                                                   *
 *                                                                             *
 * Copyright (C) 2015 Retroshare Team <retroshare.project@gmail.com>           *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifndef _TOASTER_ITEM_H
#define _TOASTER_ITEM_H

#include "gui/settings/rsharesettings.h"

#include <QWidget>

class ToasterItem : public QObject
{
	Q_OBJECT

public:
	/** Default Constructor */
	explicit ToasterItem(QWidget *child = 0);
	/** Default Destructor */
	virtual ~ToasterItem();

	QWidget *widget;

	/* Values from settings */
	RshareSettings::enumToasterPosition position;
	QPoint margin;

	/* Standard values */
	int timeToShow;
	int timeToLive;
	int timeToHide;

	/* Calculated values */
	QPoint startPos;
	QPoint endPos;
	int elapsedTimeToShow;
	int elapsedTimeToLive;
	int elapsedTimeToHide;

signals:
	void toasterItemDestroyed(ToasterItem *toasterItem);//Can't use QObject::detroyed() signal as it's emitted after this class was destroyed.
};

#endif //_TOASTER_ITEM_H
