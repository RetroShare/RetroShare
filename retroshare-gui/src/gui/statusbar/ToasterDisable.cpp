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

#include "ToasterDisable.h"
#include "gui/notifyqt.h"

#define IMAGE_TOASTERDISABLE   ":/images/toasterDisable.png"
#define IMAGE_TOASTERENABLE    ":/images/toasterEnable.png"

ToasterDisable::ToasterDisable(QWidget *parent)
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

	bool isDisable = NotifyQt::isAllDisable();
	imageButton->setChecked(isDisable);

	connect(NotifyQt::getInstance(), SIGNAL(disableAllChanged(bool)), this, SLOT(disable(bool)));
	connect(imageButton, SIGNAL(toggled(bool)), NotifyQt::getInstance(), SLOT(SetDisableAll(bool)));

	disable(isDisable);
}

void ToasterDisable::disable(bool isDisable)
{
	imageButton->setIcon(QIcon(isDisable ? IMAGE_TOASTERDISABLE : IMAGE_TOASTERENABLE));
	imageButton->setToolTip(isDisable ? tr("All Toasters are disable") : tr("Toasters are enable"));
	imageButton->setChecked(isDisable);
}
