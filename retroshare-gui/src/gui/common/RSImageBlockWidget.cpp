/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2013 RetroShare Team
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

#include <QMenu>

#include "RSImageBlockWidget.h"
#include "ui_RSImageBlockWidget.h"

RSImageBlockWidget::RSImageBlockWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::RSImageBlockWidget)
{
	ui->setupUi(this);

	connect(ui->loadImagesButton, SIGNAL(clicked()), this, SIGNAL(showImages()));
}

RSImageBlockWidget::~RSImageBlockWidget()
{
	delete ui;
}

void RSImageBlockWidget::addButtonAction(const QString &text, const QObject *receiver, const char *member, bool standardAction)
{
	QMenu *menu = ui->loadImagesButton->menu();
	if (!menu) {
		/* Set popup mode */
		ui->loadImagesButton->setPopupMode(QToolButton::MenuButtonPopup);
		ui->loadImagesButton->setIcon(ui->loadImagesButton->icon()); // Sometimes Qt doesn't recalculate sizeHint

		/* Create popup menu */
		menu = new QMenu;
		ui->loadImagesButton->setMenu(menu);

		/* Add 'click' action as action */
		QAction *action = menu->addAction(ui->loadImagesButton->text(), this, SIGNAL(showImages()));
		menu->setDefaultAction(action);
	}

	/* Add new action */
	QAction *action = menu->addAction(text, receiver, member);
	ui->loadImagesButton->addAction(action);

	if (standardAction) {
		/* Connect standard action */
		connect(action, SIGNAL(triggered()), this, SIGNAL(showImages()));
	}
}
