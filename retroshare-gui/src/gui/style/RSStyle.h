/*******************************************************************************
 * gui/style/RSStyle.h                                                         *
 *                                                                             *
 * Copyright (c) 2010 Retroshare Team <retroshare.project@gmail.com>           *
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

