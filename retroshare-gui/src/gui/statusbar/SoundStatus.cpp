/*******************************************************************************
 * gui/statusbar/soundstatus.cpp                                               *
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

#include "gui/common/FilesDefs.h"
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

	bool isMute = SoundManager::isMute();
	imageButton->setChecked(isMute);

	connect(soundManager, SIGNAL(mute(bool)), this, SLOT(mute(bool)));
	connect(imageButton, SIGNAL(toggled(bool)), soundManager, SLOT(setMute(bool)));

	mute(isMute);
}

void SoundStatus::mute(bool isMute)
{
    imageButton->setIcon(FilesDefs::getIconFromQtResourcePath(isMute ? IMAGE_MUTE_ON : IMAGE_MUTE_OFF));
    imageButton->setToolTip(isMute ? tr("Sound is off, click to turn it on") : tr("Sound is on, click to turn it off"));
}
