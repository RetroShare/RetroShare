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
