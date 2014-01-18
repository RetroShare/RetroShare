/****************************************************************
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

#include <rshare.h>
#include <util/rsrandom.h>
#include <retroshare/rsinit.h>
#include <retroshare/rspeers.h>
#include "ProfileManager.h"
#include "gui/GenCertDialog.h"
#include "gui/common/RSTreeWidgetItem.h"

#include <QAbstractEventDispatcher>
#include <QFileDialog>
#include <QMessageBox>
#include <QTreeWidget>
#include <QMenu>

#include <time.h>

#define IMAGE_EXPORT         ":/images/exportpeers_16x16.png"

#define COLUMN_NAME			0
#define COLUMN_EMAIL		1
#define COLUMN_GID			2

/** Default constructor */
ProfileManager::ProfileManager(QWidget *parent)
	: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
	/* Invoke Qt Designer generated QObject setup routine */
	ui.setupUi(this);

//	setAttribute ( Qt::WA_DeleteOnClose, true );

	ui.headerFrame->setHeaderImage(QPixmap(":/images/contact_new128.png"));
	ui.headerFrame->setHeaderText(tr("Profile Manager"));

	connect(ui.identityTreeWidget, SIGNAL( customContextMenuRequested(QPoint)), this, SLOT( identityTreeWidgetCostumPopupMenu(QPoint)));
	connect(ui.identityTreeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(identityItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
//	connect(ui.newIdentity_PB, SIGNAL(clicked()), this, SLOT(newIdentity()));
//	connect(ui.importIdentity_PB, SIGNAL(clicked()), this, SLOT(importIdentity()));
	connect(ui.exportIdentity_PB, SIGNAL(clicked()), this, SLOT(exportIdentity()));

//	ui.newIdentity_PB->hide() ;
//	ui.importIdentity_PB->hide() ;

	fillIdentities();
	ui.exportIdentity_PB->setEnabled(false);
}

void ProfileManager::identityTreeWidgetCostumPopupMenu(QPoint)
{
	QTreeWidgetItem *item = getCurrentIdentity();

	QMenu contextMnu(this);

	QAction *action = contextMnu.addAction(QIcon(IMAGE_EXPORT), tr("Export Identity"), this, SLOT(exportIdentity()));
	action->setEnabled(item != NULL);

	contextMnu.exec(QCursor::pos());
}

void ProfileManager::identityItemChanged(QTreeWidgetItem *current, QTreeWidgetItem */*previous*/)
{
	ui.exportIdentity_PB->setEnabled(current != NULL);
}

void ProfileManager::fillIdentities()
{
	ui.identityTreeWidget->clear();

	ui.identityTreeWidget->setColumnWidth(COLUMN_NAME, 200);
	ui.identityTreeWidget->setColumnWidth(COLUMN_EMAIL, 200);

	std::cerr << "Finding PGPUsers" << std::endl;

	QTreeWidget *identityTreeWidget = ui.identityTreeWidget;

	QTreeWidgetItem *item;

	std::list<std::string> pgpIds;
	std::list<std::string>::iterator it;

	if (RsAccounts::GetPGPLogins(pgpIds)) {
		for (it = pgpIds.begin(); it != pgpIds.end(); it++) {
			std::string name, email;
			RsAccounts::GetPGPLoginDetails(*it, name, email);
			std::cerr << "Adding PGPUser: " << name << " id: " << *it << std::endl;
			QString gid = QString::fromStdString(*it);

			item = new RSTreeWidgetItem(NULL, 0);
			item -> setText(COLUMN_NAME, QString::fromUtf8(name.c_str()));
			item -> setText(COLUMN_EMAIL, QString::fromUtf8(email.c_str()));
			item -> setText(COLUMN_GID, gid);
			identityTreeWidget->addTopLevelItem(item);
		}
	}

//	for (int i = 0; i < ui.identityTreeWidget->columnCount(); ++i) {
//		ui.identityTreeWidget->resizeColumnToContents(i);
//	}
}

void ProfileManager::exportIdentity()
{
	QTreeWidgetItem *item = getCurrentIdentity();
	if (!item)
		return;

	std::string gpgId = item->text(COLUMN_GID).toStdString();
	if (gpgId.empty())
		return;

	QString fname = QFileDialog::getSaveFileName(this, tr("Export Identity"), "", tr("RetroShare Identity files (*.asc)"));

	if (fname.isNull())
		return;

	if (QFileInfo(fname).suffix().isEmpty()) {
		fname += ".asc";
	}

	if (RsAccounts::ExportIdentity(fname.toUtf8().constData(), gpgId))
		QMessageBox::information(this, tr("Identity saved"), tr("Your identity was successfully saved\nIt is encrypted\n\nYou can now copy it to another computer\nand use the import button to load it"));
	else
		QMessageBox::information(this, tr("Identity not saved"), tr("Your identity was not saved. An error occurred."));
}

void ProfileManager::importIdentity()
{
	QString fname = QFileDialog::getOpenFileName(this,tr("Import Identity"), "",tr("RetroShare Identity files (*.asc)")) ;

	if(fname.isNull())
		return ;

	std::string gpg_id ;
	std::string err_string ;

	if(!RsAccounts::ImportIdentity(fname.toUtf8().constData(),gpg_id,err_string))
	{
		QMessageBox::information(this,tr("Identity not loaded"),tr("Your identity was not loaded properly:")+" \n    "+QString::fromStdString(err_string)) ;
		return ;
	}
	else
	{
		std::string name,email ;

		RsAccounts::GetPGPLoginDetails(gpg_id, name, email);
		std::cerr << "Adding PGPUser: " << name << " id: " << gpg_id << std::endl;

		QMessageBox::information(this,tr("New identity imported"),tr("Your identity was imported successfully:")+" \n"+"\nName :"+QString::fromStdString(name)+"\nemail: " + QString::fromStdString(email)+"\nKey ID: "+QString::fromStdString(gpg_id)+"\n\n"+tr("You can use it now to create a new location.")) ;
	}

	fillIdentities();
}

void ProfileManager::selectFriend()
{
#if 0
	/* still need to find home (first) */

	QString fileName = QFileDialog::getOpenFileName(this, tr("Select Trusted Friend"), "",
													tr("Certificates (*.pqi *.pem)"));

	std::string fname, userName;
	fname = fileName.toStdString();
	if (RsInit::ValidateTrustedUser(fname, userName))
	{
		ui.genFriend -> setText(QString::fromStdString(userName));
	}
	else
	{
		ui.genFriend -> setText("<Invalid Selected>");
	}
#endif
}

void ProfileManager::checkChanged(int /*i*/)
{
#if 0
	if (i)
	{
		selectFriend();
	}
	else
	{
		/* invalidate selection */
		std::string fname = "";
		std::string userName = "";
		RsInit::ValidateTrustedUser(fname, userName);
		ui.genFriend -> setText("<None Selected>");
	}
#endif
}

void ProfileManager::newIdentity()
{
	GenCertDialog gd(true);
	gd.exec();
	fillIdentities();
}

QTreeWidgetItem *ProfileManager::getCurrentIdentity()
{ 
	if (ui.identityTreeWidget->selectedItems().size() != 0) {
		return ui.identityTreeWidget->currentItem();
	}

	return NULL;
} 
