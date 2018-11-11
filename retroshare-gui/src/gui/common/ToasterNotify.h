/*******************************************************************************
 * gui/common/ToasterNotify.h                                                  *
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

#ifndef TOASTERNOTIFY_H
#define TOASTERNOTIFY_H

#include <QMap>
#include <QObject>

class ToasterItem;

class ToasterNotify : public QObject
{
	Q_OBJECT

public:
	ToasterNotify(QObject *parent = 0);
	~ToasterNotify();

	virtual bool hasSetting(QString &/*name*/);
	virtual bool notifyEnabled();
	virtual void setNotifyEnabled(bool /*enabled*/);
	virtual ToasterItem *toasterItem();
	virtual ToasterItem *testToasterItem();

	//For Plugin with differents Toasters
	virtual bool hasSettings(QString &/*mainName*/, QMap<QString,QString> &/*tagAndTexts*/);
	virtual bool notifyEnabled(QString /*tag*/);
	virtual void setNotifyEnabled(QString /*tag*/, bool /*enabled*/);
	virtual ToasterItem *testToasterItem(QString /*tag*/);
};

#endif // TOASTERNOTIFY_H
