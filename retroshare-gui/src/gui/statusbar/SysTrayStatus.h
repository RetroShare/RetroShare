/*******************************************************************************
 * gui/statusbar/systraystatus.h                                               *
 *                                                                             *
 * Copyright (c) 2012 Retroshare Team <retroshare.project@gmail.com>           *
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

#ifndef SYSTRAYSTATUS_H
#define SYSTRAYSTATUS_H

#include <QWidget>
#include <QMenu>

class QPushButton;

class SysTrayStatus : public QWidget
{
	Q_OBJECT

public:
	explicit SysTrayStatus(QWidget *parent = 0);
	void setIcon(const QIcon &icon);

	QMenu *trayMenu;
	QAction *toggleVisibilityAction;

private slots:
	void showMenu();

private:
	QPushButton *imageButton;
};

#endif // SYSTRAYSTATUS_H
