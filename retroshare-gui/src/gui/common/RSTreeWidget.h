/*******************************************************************************
 * gui/common/RSTreeWidget.h                                                   *
 *                                                                             *
 * Copyright (C) 2012 RetroShare Team <retroshare.project@gmail.com>           *
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

#ifndef _RSTREEWIDGET_H
#define _RSTREEWIDGET_H

#include <QTreeWidget>

#define FILTER_REASON_TEXT           0x0001
#define FILTER_REASON_MINVAL         0x0002

/* Subclassing QTreeWidget */
class RSTreeWidget : public QTreeWidget
{
	Q_OBJECT

public:
	RSTreeWidget(QWidget *parent = 0);

	QString placeholderText() { return mPlaceholderText; }
	void setPlaceholderText(const QString &text);

	void setFilterReasonRole(int role = -1);
	void filterItems(int filterColumn, const QString &text, int role = Qt::DisplayRole);
	void filterMinValItems(int filterColumn, const double &value, int role = Qt::DisplayRole);

	void setSettingsVersion(qint32 version);
	void processSettings(bool load);

	void enableColumnCustomize(bool customizable);
	void setColumnCustomizable(int column, bool customizable);

	void resort();

	// Add QAction to context menu (action won't be deleted)
	void addContextMenuAction(QAction *action);
	// Add QMenu to context menu (menu won't be deleted)
	void addContextMenuMenu(QMenu *menu);
	// Get Default context menu (Columns choice and menus added)
	QMenu *createStandardContextMenu(QMenu *menu);

signals:
	void signalMouseMiddleButtonClicked(QTreeWidgetItem *item);
	void headerVisibleChanged(bool visible);
	void columnVisibleChanged(int column, bool visible);

private:
	bool filterItem(QTreeWidgetItem *item, int filterColumn, const QString &text, int role);
	bool filterMinValItem(QTreeWidgetItem *item, int filterColumn, const double &value, int role);

private slots:
	void headerContextMenuRequested(const QPoint &pos);
	void headerVisible();
	void columnVisible();

protected:
	void paintEvent(QPaintEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);

private:
	QString mPlaceholderText;
	bool mEnableColumnCustomize;
	quint32 mSettingsVersion;
	QMap<int, bool> mColumnCustomizable;
	QList<QAction*> mContextMenuActions;
	QList<QMenu*> mContextMenuMenus;
	int mFilterReasonRole;
};

#endif
