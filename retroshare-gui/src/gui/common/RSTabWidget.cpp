/*******************************************************************************
 * gui/common/RSTabWidget.cpp                                                  *
 *                                                                             *
 * Copyright (C) 2010 RetroShare Team <retroshare.project@gmail.com>           *
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

#include <QTabBar>
#include <QStyle>

#include "RSTabWidget.h"

RSTabWidget::RSTabWidget(QWidget *parent) : QTabWidget(parent)
{
	mHideTabBarWithOneTab = false;
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

void RSTabWidget::setHideTabBarWithOneTab(bool hideTabBar)
{
	if (mHideTabBarWithOneTab == hideTabBar) {
		return;
	}

	mHideTabBarWithOneTab = hideTabBar;
	hideTabBarWithOneTab();
}

void RSTabWidget::tabInserted(int index)
{
	QTabWidget::tabInserted(index);

	if (mHideTabBarWithOneTab) {
		hideTabBarWithOneTab();
	}
}

void RSTabWidget::tabRemoved(int index)
{
	QTabWidget::tabRemoved(index);

	if (mHideTabBarWithOneTab) {
		hideTabBarWithOneTab();
	}
}

void RSTabWidget::hideTabBarWithOneTab()
{
	if (mHideTabBarWithOneTab) {
		tabBar()->setVisible(count() > 1);
	} else {
		tabBar()->show();
	}
}
