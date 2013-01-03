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

#include <QHBoxLayout>
#include <QPushButton>

#include "SoundStatus.h"
#include "gui/SoundManager.h"

#define IMAGE_MUTE_ON   ":/images/mute-on-16.png"
#define IMAGE_MUTE_OFF  ":/images/mute-off-16.png"

SoundStatus::SoundStatus(QWidget *parent)
 : QWidget(parent)
{
	QHBoxLayout *hbox = new QHBoxLayout(this);
	hbox->setMargin(0);
	hbox->setSpacing(0);

	imageButton = new QPushButton(this);
	imageButton->setFlat(true);
	imageButton->setCheckable(true);
	imageButton->setMaximumSize(24, 24);
	imageButton->setFocusPolicy(Qt::ClickFocus);
	hbox->addWidget(imageButton);

	setLayout(hbox);

	bool isMute = soundManager->isMute();
	imageButton->setChecked(isMute);

	connect(soundManager, SIGNAL(mute(bool)), this, SLOT(mute(bool)));
	connect(imageButton, SIGNAL(toggled(bool)), soundManager, SLOT(setMute(bool)));

	mute(isMute);
}

void SoundStatus::mute(bool isMute)
{
	imageButton->setIcon(QIcon(isMute ? IMAGE_MUTE_ON : IMAGE_MUTE_OFF));
	imageButton->setToolTip(isMute ? tr("Sound on") : tr("Sound off"));
}
