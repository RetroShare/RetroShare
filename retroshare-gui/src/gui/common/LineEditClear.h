/*******************************************************************************
 * gui/common/LineEditClear.h                                                  *
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

#ifndef LINEEDITCLEAR_H
#define LINEEDITCLEAR_H

#include <QLineEdit>
#include <QMap>

class QToolButton;
class QActionGroup;
//#if QT_VERSION < 0x040700
#if QT_VERSION < 0x050000//PlaceHolder text only shown when not have focus in Qt4
class QLabel;
#endif

class LineEditClear : public QLineEdit
{
	Q_OBJECT

public:
	LineEditClear(QWidget *parent = 0);
	~LineEditClear();

	void addFilter(const QIcon &icon, const QString &text, int id, const QString &description = "");
	void setCurrentFilter(int id);
	int currentFilter();

	void showFilterIcon();

//#if QT_VERSION < 0x040700
	// for Qt version with setPlaceholderText too to set the tooltip of the lineedit
	void setPlaceholderText(const QString &text);
//#endif

signals:
	void filterChanged(int id);

protected:
	void resizeEvent(QResizeEvent *);
//#if QT_VERSION < 0x040700
#if 0//PlaceHolder text only shown when not have focus in Qt4
	void focusInEvent(QFocusEvent *event);
	void focusOutEvent(QFocusEvent *event);
#endif
	void reposButtons();
	void activateAction(QAction *action);
	void setFilterButtonIcon(const QIcon &icon);

private slots:
	void updateClearButton(const QString &text);
	void filterTriggered(QAction *action);

private:
	QToolButton *mClearButton;
	QToolButton *mFilterButton;
	QActionGroup *mActionGroup;
	QMap<int, QString> mDescription;

//#if QT_VERSION < 0x040700
#if QT_VERSION < 0x050000//PlaceHolder text only shown when not have focus in Qt4
	QLabel *mFilterLabel;
#endif
};

#endif // LINEEDITCLEAR_H
