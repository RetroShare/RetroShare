/*******************************************************************************
 * gui/common/RSImageBlockWidget.cpp                                           *
 *                                                                             *
 * Copyright (C) 2013 RetroShare Team <retroshare.project@gmail.com>           *
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

#include <QMenu>

#include "RSImageBlockWidget.h"
#include "ui_RSImageBlockWidget.h"

RSImageBlockWidget::RSImageBlockWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::RSImageBlockWidget),
	mAutoHide(false), mAutoHideHeight(4), mAutoHideTimeToStart(3000), mAutoHideDuration(3000)
{
	ui->setupUi(this);
	mDefaultRect = this->geometry();

	ui->info_Frame->installEventFilter(this);

	mTimer = new RsProtectedTimer(this);
	mTimer->setSingleShot(true);
	connect(mTimer, SIGNAL(timeout()), this, SLOT(startAutoHide()));

	mAnimation = new QPropertyAnimation(this, "geometry");

	connect(ui->loadImagesButton, SIGNAL(clicked()), this, SIGNAL(showImages()));
}

RSImageBlockWidget::~RSImageBlockWidget()
{
	delete mAnimation;
	mTimer->stop();
	delete mTimer;
	delete ui->loadImagesButton->menu();
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

bool RSImageBlockWidget::eventFilter(QObject *obj, QEvent *event)
{
	if (mAutoHide) {
		if (event->type() == QEvent::Show) {
			mTimer->start(mAutoHideTimeToStart);
		}
		if (event->type() == QEvent::Hide) {
			mTimer->stop();
		}
		if (event->type() == QEvent::Enter) {
			mAnimation->stop();
			this->setGeometry(mDefaultRect);
			this->updateGeometry();
			mTimer->start(mAutoHideTimeToStart);
			mAnimation->setCurrentTime(0);
		}
	}
	if (mAnimation->currentTime() == 0) {
		mDefaultRect = this->geometry();
	} else if (mAnimation->state() == QAbstractAnimation::Running) {
		this->updateGeometry();
	}

	// pass the event on to the parent class
	return QObject::eventFilter(obj, event);
}

void RSImageBlockWidget::setAutoHide(const bool value)
{
	if(value && !mAutoHide) {
		if (this->isVisible()) mTimer->start(mAutoHideTimeToStart);
	} else if (!value && mAutoHide) {
		mTimer->stop();
	}
	mAutoHide = value;
}

void RSImageBlockWidget::startAutoHide()
{
	QRect r = mDefaultRect;
	r.setHeight(mAutoHideHeight);
	this->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
	mAnimation->setDuration(mAutoHideDuration);
	mAnimation->setStartValue(mDefaultRect);
	mAnimation->setEndValue(r);

	mAnimation->start();
}

QSize RSImageBlockWidget::sizeHint() const
{
	if (mAnimation->currentTime() == 0) {
		return mDefaultRect.size();
	} else {
		return mAnimation->currentValue().toRect().size();
	}
}
