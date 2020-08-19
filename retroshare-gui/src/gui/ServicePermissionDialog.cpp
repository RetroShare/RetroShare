/*******************************************************************************
 * gui/ServicePermissionDialog.cpp                                             *
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

#include <QTreeWidgetItem>

#include "ServicePermissionDialog.h"
#include "ui_ServicePermissionDialog.h"
#include "settings/rsharesettings.h"
#include "gui/common/FilesDefs.h"

static ServicePermissionDialog *servicePermissionDialog = NULL;

ServicePermissionDialog::ServicePermissionDialog() :
	QDialog(NULL, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint),
	ui(new Ui::ServicePermissionDialog)
{
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);

	Settings->loadWidgetInformation(this);
	
    ui->headerFrame->setHeaderImage(FilesDefs::getPixmapFromQtResourcePath(":/images/user/servicepermissions64.png"));
    ui->headerFrame->setHeaderText(tr("Service Permissions"));

	connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(setPermissions()));
	connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	connect(ui->servicePermissionList, SIGNAL(itemAdded(int,QString,QTreeWidgetItem*)), this, SLOT(itemAdded(int,QString,QTreeWidgetItem*)));
	connect(ui->servicePermissionList, SIGNAL(itemChanged(int,QString,QTreeWidgetItem*,int)), this, SLOT(itemChanged(int,QString,QTreeWidgetItem*,int)));

	ui->servicePermissionList->setModus(FriendSelectionWidget::MODUS_SINGLE);
	ui->servicePermissionList->setShowType(FriendSelectionWidget::SHOW_GROUP | FriendSelectionWidget::SHOW_GPG);

    /* add columns */
    int column ;
    column = ui->servicePermissionList->addColumn(tr("Use as direct source, when available"));
    mColumns[column] = RS_NODE_PERM_DIRECT_DL;
    column = ui->servicePermissionList->addColumn(tr("Auto-download recommended files"));
    mColumns[column] = RS_NODE_PERM_ALLOW_PUSH;
    column = ui->servicePermissionList->addColumn(tr("Require whitelist"));
    mColumns[column] = RS_NODE_PERM_REQUIRE_WL;

	ui->servicePermissionList->start();
}

ServicePermissionDialog::~ServicePermissionDialog()
{
	Settings->saveWidgetInformation(this);

	delete ui;

	servicePermissionDialog = NULL;
}

void ServicePermissionDialog::showYourself()
{
	if (!servicePermissionDialog) {
		servicePermissionDialog = new ServicePermissionDialog();
	}

	servicePermissionDialog->show();
	servicePermissionDialog->activateWindow();
}

void ServicePermissionDialog::itemAdded(int idType, const QString &id, QTreeWidgetItem *item)
{
	if (idType != FriendSelectionWidget::IDTYPE_GPG) {
		return;
	}

	RsPeerDetails detail;
    if (!rsPeers->getGPGDetails(RsPgpId(id.toStdString()), detail)) {
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

        rsPeers->setServicePermissionFlags(RsPgpId(ui->servicePermissionList->idFromItem(item)), flags);
	}

	done(Accepted);
}
