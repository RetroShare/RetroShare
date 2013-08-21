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

#include <QTreeWidgetItem>

#include "ServicePermissionDialog.h"
#include "ui_ServicePermissionDialog.h"
#include "settings/rsharesettings.h"

ServicePermissionDialog::ServicePermissionDialog() :
	QDialog(NULL, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint),
	ui(new Ui::ServicePermissionDialog)
{
	ui->setupUi(this);

	Settings->loadWidgetInformation(this);
	
	ui->headerFrame->setHeaderImage(QPixmap(":/images/user/servicepermissions64.png"));
  ui->headerFrame->setHeaderText(tr("Service Permissions"));

	connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(setPermissions()));
	connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	connect(ui->servicePermissionList, SIGNAL(itemAdded(int,QString,QTreeWidgetItem*)), this, SLOT(itemAdded(int,QString,QTreeWidgetItem*)));
	connect(ui->servicePermissionList, SIGNAL(itemChanged(int,QString,QTreeWidgetItem*,int)), this, SLOT(itemChanged(int,QString,QTreeWidgetItem*,int)));

	ui->servicePermissionList->setModus(FriendSelectionWidget::MODUS_SINGLE);
	ui->servicePermissionList->setShowType(FriendSelectionWidget::SHOW_GROUP | FriendSelectionWidget::SHOW_GPG);

	/* add columns */
	int column = ui->servicePermissionList->addColumn(tr("Anonymous routing"));
	mColumns[column] = RS_SERVICE_PERM_TURTLE;
	column = ui->servicePermissionList->addColumn(tr("Discovery"));
	mColumns[column] = RS_SERVICE_PERM_DISCOVERY;
	column = ui->servicePermissionList->addColumn(tr("Forums/Channels"));
	mColumns[column] = RS_SERVICE_PERM_DISTRIB;

	ui->servicePermissionList->start();
}

ServicePermissionDialog::~ServicePermissionDialog()
{
	Settings->saveWidgetInformation(this);

	delete ui;
}

void ServicePermissionDialog::itemAdded(int idType, const QString &id, QTreeWidgetItem *item)
{
	if (idType != FriendSelectionWidget::IDTYPE_GPG) {
		return;
	}

	RsPeerDetails detail;
	if (!rsPeers->getPeerDetails(id.toStdString(), detail)) {
		return;
	}

	QMap<int, ServicePermissionFlags>::iterator it;
	for (it = mColumns.begin(); it != mColumns.end(); ++it) {
		item->setCheckState(it.key(), (detail.service_perm_flags & it.value()) ? Qt::Checked : Qt::Unchecked);
	}
}

void ServicePermissionDialog::itemChanged(int idType, const QString &id, QTreeWidgetItem *item, int column)
{
	if (idType != FriendSelectionWidget::IDTYPE_GPG) {
		return;
	}

	if (!mColumns.contains(column)) {
		return;
	}

	QList<QTreeWidgetItem*> items;
	ui->servicePermissionList->itemsFromId((FriendSelectionWidget::IdType) idType, id.toStdString(), items);

	/* set checkboxes for the same id in other groups */
	QList<QTreeWidgetItem*>::iterator it;
	for (it = items.begin(); it != items.end(); ++it) {
		if (*it == item) {
			continue;
		}

		(*it)->setCheckState(column, item->checkState(column));
	}
}

void ServicePermissionDialog::setPermissions()
{
	QList<QTreeWidgetItem*> items;
	ui->servicePermissionList->items(items, FriendSelectionWidget::IDTYPE_GPG);

	/* no problem when gpg id is assigned twice */
	QList<QTreeWidgetItem*>::iterator itemIt;
	for (itemIt = items.begin(); itemIt != items.end(); ++itemIt) {
		QTreeWidgetItem *item = *itemIt;

		ServicePermissionFlags flags(0);
		QMap<int, ServicePermissionFlags>::iterator it;
		for (it = mColumns.begin(); it != mColumns.end(); ++it) {
			if (item->checkState(it.key()) == Qt::Checked) {
				flags |= it.value();
			}
		}

		rsPeers->setServicePermissionFlags(ui->servicePermissionList->idFromItem(item), flags);
	}

	done(Accepted);
}
