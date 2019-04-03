/*******************************************************************************
 * gui/common/ToasterNotify.cpp                                                *
 *                                                                             *
 * Copyright (c) 2015, RetroShare Team <retroshare.project@gmail.com>          *
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

#include "ToasterNotify.h"

ToasterNotify::ToasterNotify(QObject *parent) :
    QObject(parent)
{
}

ToasterNotify::~ToasterNotify()
{
}

bool ToasterNotify::hasSetting(QString &/*name*/)
{
	return false;
}

bool ToasterNotify::notifyEnabled()
{
	return false;
}

void ToasterNotify::setNotifyEnabled(bool /*enabled*/)
{
}

ToasterItem *ToasterNotify::toasterItem()
{
	return NULL;
}

ToasterItem *ToasterNotify::testToasterItem()
{
	return NULL;
}

bool ToasterNotify::hasSettings(QString &/*name*/, QMap<QString,QString> &/*tagAndTexts*/)
{
	return false;
}

bool ToasterNotify::notifyEnabled(QString /*tag*/)
{
	return false;
}

void ToasterNotify::setNotifyEnabled(QString /*tag*/, bool /*enabled*/)
{
}

ToasterItem *ToasterNotify::testToasterItem(QString /*tag*/)
{
	return NULL;
}
