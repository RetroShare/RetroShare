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

#include <QTabBar>
#include <QStyle>

#include "RSTabWidget.h"

RSTabWidget::RSTabWidget(QWidget *parent) : QTabWidget(parent)
{
}

QTabBar *RSTabWidget::tabBar() const
{
	return QTabWidget::tabBar();
}

void RSTabWidget::hideCloseButton(int index)
{
	QTabBar::ButtonPosition buttonPosition = (QTabBar::ButtonPosition) tabBar()->style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, 0, 0);
	QWidget *tabButton = tabBar()->tabButton(index, buttonPosition);
	if (tabButton) {
		tabButton->hide();
	}
}
