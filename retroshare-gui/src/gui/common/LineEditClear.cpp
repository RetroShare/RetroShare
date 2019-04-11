/*******************************************************************************
 * gui/common/LineEditClear.cpp                                                *
 *                                                                             *
 * Copyright (C) 2012, Retroshare Team <retroshare.project@gmail.com>          *
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

#include "LineEditClear.h"

#include <QToolButton>
#include <QStyle>
#include <QMenu>
//#if QT_VERSION < 0x040700
#if QT_VERSION < 0x050000//PlaceHolder text only shown when not have focus in Qt4
#include <QLabel>
#endif

#define IMAGE_FILTER ":/images/find-16.png"

LineEditClear::LineEditClear(QWidget *parent)
	: QLineEdit(parent)
{
	mActionGroup = NULL;
	mFilterButton = NULL;

	mClearButton = new QToolButton(this);
	mClearButton->setFixedSize(16, 16);
	mClearButton->setIconSize(QSize(16, 16));
	mClearButton->setCursor(Qt::ArrowCursor);
	mClearButton->setStyleSheet("QToolButton { border: none; padding: 0px; }"
								"QToolButton { border-image: url(:/images/closenormal.png) }"
								"QToolButton:hover { border-image: url(:/images/closehover.png) }"
								"QToolButton:pressed  { border-image: url(:/images/closepressed.png) }");
	mClearButton->hide();
	mClearButton->setFocusPolicy(Qt::NoFocus);
	mClearButton->setToolTip("Clear Filter");

	connect(mClearButton, SIGNAL(clicked()), this, SLOT(clear()));
	connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(updateClearButton(const QString&)));

//#if QT_VERSION < 0x040700
#if QT_VERSION < 0x050000//PlaceHolder text only shown when not have focus in Qt4
	mFilterLabel = new QLabel("", this);
	mFilterLabel->setStyleSheet("QLabel { color: gray; }");
#endif

	reposButtons();

	int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
	QSize msz = minimumSizeHint();
	setMinimumSize(
		qMax(msz.width(), mClearButton->sizeHint().height() + /*mFilterButton->sizeHint().width() + */frameWidth * 2),
		qMax(msz.height(), mClearButton->sizeHint().height() + frameWidth * 2));
}

void LineEditClear::resizeEvent(QResizeEvent *)
{
	QSize sz = mClearButton->sizeHint();
	int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
	mClearButton->move(rect().right() - frameWidth - sz.width() + 2, (rect().bottom() - sz.height()) / 2 + 2);

//#if QT_VERSION < 0x040700
#if QT_VERSION < 0x050000//PlaceHolder text only shown when not have focus in Qt4
	sz = mFilterLabel->sizeHint();
	mFilterLabel->move(frameWidth + (mFilterButton ? mFilterButton->sizeHint().width() + 5 : 5), (rect().bottom() + 1 - sz.height())/2);
#endif
}

void LineEditClear::setPlaceholderText(const QString &text)
{
//#if QT_VERSION < 0x040700
#if QT_VERSION < 0x050000//PlaceHolder text only shown when not have focus in Qt4
	mFilterLabel->setText(text);
#else
	QLineEdit::setPlaceholderText(text);
#endif

	setToolTip(text);
}

//#if QT_VERSION < 0x040700
#if 0//PlaceHolder text only shown when not have focus in Qt4
void LineEditClear::focusInEvent(QFocusEvent *event)
{
	mFilterLabel->setVisible(false);
	QLineEdit::focusInEvent(event);
}

void LineEditClear::focusOutEvent(QFocusEvent *event)
{
	if (text().isEmpty()) {
		mFilterLabel->setVisible(true);
	}
	QLineEdit::focusOutEvent(event);
}
#endif

void LineEditClear::reposButtons()
{
	int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
	setStyleSheet(QString("QLineEdit { padding-right: %1px; padding-left: %2px; }")
				  .arg(mClearButton->sizeHint().width() + frameWidth + 1)
				  .arg(mFilterButton ? mFilterButton->sizeHint().width() + frameWidth + 1 : 0));
}

void LineEditClear::showFilterIcon()
{
	if (mFilterButton) {
		return;
	}

	mFilterButton = new QToolButton(this);
	mFilterButton->setFixedSize(16, 16);
	QPixmap filterPixmap(IMAGE_FILTER);
	mFilterButton->setIcon(QIcon(filterPixmap));
	mFilterButton->setIconSize(filterPixmap.size());
	mFilterButton->setCursor(Qt::ArrowCursor);
	mFilterButton->setStyleSheet("QToolButton { border: none; padding: 0px; }"
								 "QToolButton[popupMode=\"2\"] { padding-right: 10px; }"
								 "QToolButton::menu-indicator[popupMode=\"2\"] { subcontrol-origin: padding; subcontrol-position: bottom right; top: 5px; left: -3px; width: 7px; }");
	mFilterButton->move(2, 2);
	mFilterButton->setFocusPolicy(Qt::NoFocus);

	reposButtons();
}

void LineEditClear::updateClearButton(const QString& text)
{
	mClearButton->setVisible(!text.isEmpty());
#if QT_VERSION < 0x050000//PlaceHolder text only shown when not have focus in Qt4
	mFilterLabel->setVisible(text.isEmpty());
#endif
}

void LineEditClear::addFilter(const QIcon &icon, const QString &text, int id, const QString &description)
{
	QAction *action = new QAction(icon, text, this);
	action->setData(id);
	action->setCheckable(true);
	mDescription[id] = description;

	showFilterIcon();

	if (mActionGroup == NULL) {
		mFilterButton->setFixedSize(26, 16);
		mFilterButton->setPopupMode(QToolButton::InstantPopup);

		mActionGroup = new QActionGroup(this);
		mActionGroup->setExclusive(true);

		QMenu *menu = new QMenu;
		mFilterButton->setMenu(menu);

		connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(filterTriggered(QAction*)));
		reposButtons();

		/* set first action checked */
		action->setChecked(true);
		activateAction(action);
	}

	mFilterButton->menu()->addAction(action);
	mActionGroup->addAction(action);
}

void LineEditClear::setCurrentFilter(int id)
{
	if (mFilterButton == NULL) {
		return;
	}

	QMenu *menu = mFilterButton->menu();
	if (menu) {
		Q_FOREACH (QAction *action, menu->actions()) {
			if (action->data().toInt() == id) {
				action->setChecked(true);
				activateAction(action);
				emit filterChanged(id);
				break;
			}
		}
	}
}

int LineEditClear::currentFilter()
{
	if (mActionGroup == NULL) {
		return 0;
	}

	QAction *action = mActionGroup->checkedAction();
	if (action) {
		return action->data().toInt();
	}

	return 0;
}

void LineEditClear::filterTriggered(QAction *action)
{
	activateAction(action);
	emit filterChanged(action->data().toInt());
}

void LineEditClear::activateAction(QAction *action)
{
	QMap<int, QString>::iterator description = mDescription.find(action->data().toInt());
	if (description != mDescription.end() && !description->isEmpty()) {
		setPlaceholderText(*description);
	}

	QIcon icon = action->icon();
	if (icon.isNull()) {
		icon = QIcon(IMAGE_FILTER);
	}

	mFilterButton->setIcon(icon);
}
