/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2015 RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include "ToasterItem.h"

/** Constructor */
ToasterItem::ToasterItem(QWidget *child) : QObject(NULL)
{
	/* Set widget */
	widget = child;

	/* Values from settings */
	position = Settings->getToasterPosition();
	margin = Settings->getToasterMargin();

	/* Standard values */
	timeToShow = 500;
	timeToLive = 3000;
	timeToHide = 500;

	/* Calculated values */
	elapsedTimeToShow = 0;
	elapsedTimeToLive = 0;
	elapsedTimeToHide = 0;
}

ToasterItem::~ToasterItem()
{
	emit toasterItemDestroyed(this);
	delete widget;
}
