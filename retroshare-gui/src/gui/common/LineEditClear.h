/****************************************************************
 *
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2012, RetroShare Team
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

#ifndef LINEEDITCLEAR_H
#define LINEEDITCLEAR_H

#include <QLineEdit>
#include <QMap>

class QToolButton;
class QActionGroup;
#if QT_VERSION < 0x040700
class QLabel;
#endif

class LineEditClear : public QLineEdit
{
	Q_OBJECT

public:
	LineEditClear(QWidget *parent = 0);

	void addFilter(const QIcon &icon, const QString &text, int id, const QString &description = "");
	void setCurrentFilter(int id);
	int currentFilter();

//#if QT_VERSION < 0x040700
	// for Qt version with setPlaceholderText too to set the tooltip of the lineedit
	void setPlaceholderText(const QString &text);
//#endif

signals:
	void filterChanged(int id);

protected:
	void resizeEvent(QResizeEvent *);
#if QT_VERSION < 0x040700
	void focusInEvent(QFocusEvent *event);
	void focusOutEvent(QFocusEvent *event);
#endif
	void reposButtons();
	void activateAction(QAction *action);

private slots:
	void updateClearButton(const QString &text);
	void filterTriggered(QAction *action);

private:
	QToolButton *mClearButton;
	QToolButton *mFilterButton;
	QActionGroup *mActionGroup;
	QMap<int, QString> mDescription;

#if QT_VERSION < 0x040700
	QLabel *mFilterLabel;
#endif
};

#endif // LINEEDITCLEAR_H
