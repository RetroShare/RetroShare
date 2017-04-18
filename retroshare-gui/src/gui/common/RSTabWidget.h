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

#ifndef _RSTABWIDGET_H
#define _RSTABWIDGET_H

#include <QTabWidget>

/* Subclassing QTabWidget to get the instance of QTabBar. The base method of QTabWidget is protected. */
class RSTabWidget : public QTabWidget
{
public:
	explicit RSTabWidget(QWidget *parent = 0);

	void hideCloseButton(int index);
	void setHideTabBarWithOneTab(bool hideTabBar);

public:
	QTabBar *tabBar() const;

protected:
	virtual void tabInserted(int index);
	virtual void tabRemoved(int index);

private:
	void hideTabBarWithOneTab();

private:
	bool mHideTabBarWithOneTab;
};

#endif
