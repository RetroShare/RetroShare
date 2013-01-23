/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2013,  RetroShare Team
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
