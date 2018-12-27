/*******************************************************************************
 * gui/ServicePermissionDialog.h                                               *
 *                                                                             *
 * Copyright (c) 2013 Retroshare Team  <retroshare.project@gmail.com>          *
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

#ifndef SERVICEPERMISSIONDIALOG_H
#define SERVICEPERMISSIONDIALOG_H

#include <QDialog>
#include <QMap>

#include <retroshare/rspeers.h>

class QTreeWidgetItem;

namespace Ui {
class ServicePermissionDialog;
}

class ServicePermissionDialog : public QDialog
{
	Q_OBJECT

public:
	static void showYourself();

private:
	ServicePermissionDialog();
	~ServicePermissionDialog();

private slots:
	void itemAdded(int idType, const QString &id, QTreeWidgetItem *item);
	void itemChanged(int idType, const QString &id, QTreeWidgetItem *item, int column);
	void setPermissions();

private:
	QMap<int, ServicePermissionFlags> mColumns;

	Ui::ServicePermissionDialog *ui;
};

#endif // SERVICEPERMISSIONDIALOG_H
