/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2010, RetroShare Team
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


#ifndef _RSSTYLE_H
#define _RSSTYLE_H

#include <QList>
class QSettings;

class RSStyle
{
public:
	enum StyleType
	{
		STYLETYPE_NONE = 0,
		STYLETYPE_SOLID = 1,
		STYLETYPE_GRADIENT = 2,
		STYLETYPE_MAX = 2
	};

public:
	RSStyle();

	QString getStyleSheet() const;

	bool showDialog(QWidget *parent);

	static int neededColors(StyleType styleType);

	void readSetting (QSettings &settings);
	void writeSetting (QSettings &settings);

public:
	StyleType styleType;
	QList<QColor> colors;
};

#endif

