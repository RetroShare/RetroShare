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

#ifndef RSIMAGEBLOCKWIDGET_H
#define RSIMAGEBLOCKWIDGET_H

#include <QPropertyAnimation>
#include <QWidget>

#include "util/RsProtectedTimer.h"

namespace Ui {
class RSImageBlockWidget;
}

#warning: Cppcheck(noConstructor): The class 'RSImageBlockWidget' does not have a constructor.
// cppcheck-suppress noConstructor
class RSImageBlockWidget : public QWidget
{
	Q_OBJECT

	Q_PROPERTY(bool autoHide READ autoHide WRITE setAutoHide)
	Q_PROPERTY(int autoHideHeight READ autoHideHeight WRITE setAutoHideHeight)
	Q_PROPERTY(int autoHideTimeToStart READ autoHideTimeToStart WRITE setAutoHideTimeToStart)
	Q_PROPERTY(int autoHideDuration READ autoHideDuration WRITE setAutoHideDuration)

public:
	explicit RSImageBlockWidget(QWidget *parent = 0);
	~RSImageBlockWidget();

	void addButtonAction(const QString &text, const QObject *receiver, const char *member, bool standardAction);

	virtual QSize sizeHint() const;//To update parent layout.
	virtual QSize minimumSizeHint() const { return sizeHint();}//To update parent layout.

	bool autoHide() { return mAutoHide; }
	int autoHideHeight() { return mAutoHideHeight; }
	int autoHideTimeToStart() { return mAutoHideTimeToStart; }
	int autoHideDuration() { return mAutoHideDuration; }

	void setAutoHide(const bool value);
	void setAutoHideHeight(const int value) { mAutoHideHeight = value; }
	void setAutoHideTimeToStart(const int value) { mAutoHideTimeToStart = value; }
	void setAutoHideDuration(const int value) { mAutoHideDuration = value; }

signals:
	void showImages();

private slots:
	void startAutoHide();

protected:
	bool eventFilter(QObject *obj, QEvent *event);

private:
	Ui::RSImageBlockWidget *ui;
	QPropertyAnimation *mAnimation;
	QRect mDefaultRect;
	RsProtectedTimer *mTimer;
	bool mAutoHide;
	int mAutoHideHeight;
	int mAutoHideTimeToStart;
	int mAutoHideDuration;
};

#endif // RSIMAGEBLOCKWIDGET_H
