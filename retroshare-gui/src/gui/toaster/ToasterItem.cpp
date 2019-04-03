/*******************************************************************************
 * gui/toaster/ToasterItem.cpp                                                 *
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
