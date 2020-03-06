/*******************************************************************************
 * retroshare-gui/src/gui/PhotoShare/AlbumExtra.h                              *
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

#ifndef ALBUMEXTRA_H
#define ALBUMEXTRA_H

#include <QWidget>

namespace Ui {
	class AlbumExtra;
}

class AlbumExtra : public QWidget
{
	Q_OBJECT

public:
	explicit AlbumExtra(QWidget *parent = 0);
	virtual ~AlbumExtra();

	void setShareMode(uint32_t mode);
	void setCaption(const std::string &str);
	void setPhotographer(const std::string &str);
	void setWhere(const std::string &str);
	void setWhen(const std::string &str);

	uint32_t	getShareMode();
	std::string getCaption();
	std::string getPhotographer();
	std::string getWhere();
	std::string getWhen();

private:
	void setUp();
private:
	Ui::AlbumExtra *ui;
};

#endif // ALBUMEXTRA_H
