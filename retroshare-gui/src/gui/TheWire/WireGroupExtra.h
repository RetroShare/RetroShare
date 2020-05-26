/*******************************************************************************
 * retroshare-gui/src/gui/TheWire/WireGroupExtra.h                             *
 *                                                                             *
 * Copyright (C) 2020 by Robert Fernie <retroshare.project@gmail.com>          *
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

#ifndef WIRE_GROUP_EXTRA_H
#define WIRE_GROUP_EXTRA_H

#include <QWidget>
#include "ui_WireGroupExtra.h"

class WireGroupExtra : public QWidget
{
	Q_OBJECT

public:
	explicit WireGroupExtra(QWidget *parent = 0);
	virtual ~WireGroupExtra();

	void setMasthead(const QPixmap &pixmap);
	QPixmap getMasthead();

	void setTagline(const std::string &str);
	void setLocation(const std::string &str);

	std::string getTagline();
	std::string getLocation();

private slots:
	void addMasthead();

private:
	void setUp();
private:
	QPixmap mMasthead;
	Ui::WireGroupExtra ui;
};

#endif // WIRE_GROUP_EXTRA_H
