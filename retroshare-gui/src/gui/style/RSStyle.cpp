/*******************************************************************************
 * gui/style/RSStyle.cpp                                                       *
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

#include <QColor>
#include <QSettings>

#include "RSStyle.h"
#include "StyleDialog.h"

RSStyle::RSStyle()
{
	styleType = STYLETYPE_NONE;
}

/*static*/ int RSStyle::neededColors(StyleType styleType)
{
	switch (styleType) {
	case STYLETYPE_NONE:
		return 0;
	case STYLETYPE_SOLID:
		return 1;
	case STYLETYPE_GRADIENT:
		return 2;
	}

	return 0;
}

QString RSStyle::getStyleSheet() const
{
	QString sheet;

	switch (styleType) {
	case STYLETYPE_NONE:
		break;
	case STYLETYPE_SOLID:
		sheet = "background-color: <color1>";
		break;
	case STYLETYPE_GRADIENT:
		sheet = "background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,stop:0 <color1>, stop:1 <color2>);";
		break;
	}

	if (sheet.isEmpty() == false) {
		/* Replace colors */
		for (int i = 0; i < colors.size(); ++i) {
			sheet.replace(QString("<color%1>").arg(i + 1), colors[i].name());
		}
	}

	return sheet;
}

bool RSStyle::showDialog(QWidget *parent)
{
	StyleDialog dlg(*this, parent);
	int result = dlg.exec();
	if (result == QDialog::Accepted) {
		dlg.getStyle(*this);
		return true;
	}

	return false;
}

void RSStyle::readSetting (QSettings &settings)
{
	int type = (StyleType) settings.value("styleType").toInt();
	if (type >= 0 && type <= STYLETYPE_MAX) {
		styleType = (StyleType) type;
	} else {
		styleType = STYLETYPE_NONE;
	}

    colors.clear();

	int size = settings.beginReadArray("colors");
	for (int i = 0; i < size; ++i) {
		settings.setArrayIndex(i);
		colors.append(QColor(settings.value("value").toString()));
	}
	settings.endArray();
}

void RSStyle::writeSetting (QSettings &settings)
{
	settings.setValue("styleType", styleType);

	settings.beginWriteArray("colors");
	int size = colors.size();
	for (int i = 0; i < size; ++i) {
		settings.setArrayIndex(i);
		settings.setValue("value", colors[i].name());
	}
	settings.endArray();
}
