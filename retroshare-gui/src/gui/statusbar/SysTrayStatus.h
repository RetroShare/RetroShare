/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2012 RetroShare Team
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
