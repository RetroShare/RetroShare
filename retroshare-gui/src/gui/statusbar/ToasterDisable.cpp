/*******************************************************************************
 * gui/statusbar/ToasterDisable.cpp                                            *
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

#include <QHBoxLayout>
#include <QPushButton>

#include "ToasterDisable.h"
#include "gui/notifyqt.h"
#include "gui/common/FilesDefs.h"

#define IMAGE_TOASTERDISABLE   ":/images/toasterDisable.png"
#define IMAGE_TOASTERENABLE    ":/images/toasterEnable.png"

ToasterDisable::ToasterDisable(QWidget *parent)
 : QWidget(parent)
{
	QHBoxLayout *hbox = new QHBoxLayout(this);
	hbox->setMargin(0);
	hbox->setSpacing(0);

	imageButton = new QPushButton(this);

    int S = QFontMetricsF(imageButton->font()).height();

	imageButton->setFlat(true);
	imageButton->setCheckable(true);
	imageButton->setMaximumSize(S,S);
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
    imageButton->setIcon(FilesDefs::getPixmapFromQtResourcePath(isDisable ? IMAGE_TOASTERDISABLE : IMAGE_TOASTERENABLE));
	imageButton->setToolTip(isDisable ? tr("All Toasters are disabled") : tr("Toasters are enabled"));
	imageButton->setChecked(isDisable);
}
