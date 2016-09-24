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

#include <QAction>
#include <QHBoxLayout>
#include <QPushButton>

#include "SysTrayStatus.h"

#define IMAGE_NOONLINE          ":/icons/logo_0_connected_128.png"
#define IMAGE_ONEONLINE         ":/icons/logo_1_connected_128.png"
#define IMAGE_TWOONLINE         ":/icons/logo_2_connected_128.png"

SysTrayStatus::SysTrayStatus(QWidget *parent) :
  QWidget(parent)
{
	QHBoxLayout *hbox = new QHBoxLayout(this);
	hbox->setMargin(0);
	hbox->setSpacing(0);

	imageButton = new QPushButton(this);
	imageButton->setIcon(QIcon(IMAGE_NOONLINE));
	imageButton->setFlat(true);
	imageButton->setCheckable(false);
	imageButton->setFocusPolicy(Qt::ClickFocus);
	hbox->addWidget(imageButton);

	setLayout(hbox);

	trayMenu = NULL;
	toggleVisibilityAction = NULL;

	connect(imageButton, SIGNAL(clicked()), this, SLOT(showMenu()));
}
void SysTrayStatus::setIcon(const QIcon &icon)
{
	imageButton->setIcon(icon);
}

void SysTrayStatus::showMenu()
{
	if(toggleVisibilityAction) toggleVisibilityAction->setVisible(false);
	if(trayMenu) trayMenu->exec(QCursor::pos());
	if(toggleVisibilityAction) toggleVisibilityAction->setVisible(true);
}
