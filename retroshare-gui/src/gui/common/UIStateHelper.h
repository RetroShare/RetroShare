/*******************************************************************************
 * gui/common/UIStateHelper.h                                                  *
 *                                                                             *
 * Copyright (c) 2013, RetroShare Team <retroshare.project@gmail.com>          *
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

#ifndef UISTATEHELPER_H
#define UISTATEHELPER_H

#include <QObject>
#include <QMap>

class QWidget;
class QLabel;
class ElidedLabel;
class QLineEdit;
class RSTreeWidget;
class UIStateHelperData;
class RSTextBrowser;

enum UIState // State is untouched when bit is not set
{
	/* State for ::setLoading */
	UISTATE_LOADING_VISIBLE     = 0x00000001, // visible when loading
	UISTATE_LOADING_INVISIBLE   = 0x00000002, // invisible when loading
	UISTATE_LOADING_ENABLED     = 0x00000004, // enabled when loading
	UISTATE_LOADING_DISABLED    = 0x00000008, // disabled when loading
	/* State for ::setActive */
	UISTATE_ACTIVE_VISIBLE      = 0x00000010, // visible when active
	UISTATE_ACTIVE_INVISIBLE    = 0x00000020, // invisible when active
	UISTATE_ACTIVE_ENABLED      = 0x00000040, // enabled when active
	UISTATE_ACTIVE_DISABLED     = 0x00000080  // disabled when active
};
Q_DECLARE_FLAGS(UIStates, UIState)
Q_DECLARE_OPERATORS_FOR_FLAGS(UIStates)

class UIStateHelper : public QObject
{
	Q_OBJECT

public:
	UIStateHelper(QObject *parent = 0);
	~UIStateHelper();

	/* Add widgets */
	void addWidget(int index, QWidget *widget, UIStates states = UISTATE_LOADING_DISABLED | UISTATE_ACTIVE_ENABLED);
	void addLoadPlaceholder(int index, QLabel *widget, bool clear = true, const QString &text = "" /* ="Loading" */);
	void addLoadPlaceholder(int index, ElidedLabel *widget, bool clear = true, const QString &text = "" /* ="Loading" */);
	void addLoadPlaceholder(int index, QLineEdit *widget, bool clear = true, const QString &text = "" /* ="Loading" */);
	void addLoadPlaceholder(int index, RSTreeWidget *widget, bool clear = true, const QString &text = "" /* ="Loading" */);
	void addLoadPlaceholder(int index, RSTextBrowser *widget, bool clear = true, const QString &text = "" /* ="Loading" */);
	void addClear(int index, QLabel *widget);
	void addClear(int index, QLineEdit *widget);
	void addClear(int index, RSTreeWidget *widget);
	void addClear(int index, RSTextBrowser *widget);

	/* Set state */
	void setLoading(int index, bool loading);
	void setActive(int index, bool active);
	void clear(int index);

	/* State */
	bool isLoading(int index);
	bool isActive(int index);

	/* Set state of widget */
	void setWidgetVisible(QWidget *widget, bool visible);
	void setWidgetEnabled(QWidget *widget, bool enabled);

private:
	UIStateHelperData *findData(int index, bool create);
	void updateData(UIStateHelperData *data);
	bool isWidgetVisible(QWidget *widget);
	bool isWidgetEnabled(QWidget *widget);
	bool isWidgetLoading(QWidget *widget, QString &text);

private:
	QMap<long, UIStateHelperData*> mData;
	QMap<QWidget*, bool> mWidgetVisible;
	QMap<QWidget*, bool> mWidgetEnabled;
};

#endif // UISTATEHELPER_H
