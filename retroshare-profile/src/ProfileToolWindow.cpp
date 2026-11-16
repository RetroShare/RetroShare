/*******************************************************************************
 * retroshare-profile/src/ProfileToolWindow.cpp                                *
 *                                                                             *
 * Copyright (c) 2026 RetroShare Team <retroshare.project@gmail.com>           *
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

#include "ProfileToolWindow.h"

#include "ProfileArchive.h"
#include "ProfileUpgradeDialog.h"
#include "NodeInfo.h"

#include "retroshare/rsinit.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QLocale>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>

#include <list>

#define COLUMN_PROFILE  0
#define COLUMN_NODE     1
#define COLUMN_TYPE     2
#define COLUMN_KEYS     3
#define COLUMN_LASTUSED 4
#define COLUMN_ID       5

#define ROLE_NODE_ID   Qt::UserRole
#define ROLE_PGP_ID    (Qt::UserRole + 1)
#define ROLE_PGP_NAME  (Qt::UserRole + 2)
#define ROLE_KEYS_WEAK (Qt::UserRole + 3)

ProfileToolWindow::ProfileToolWindow(bool rnpExistedAtStart, QWidget *parent)
    : QDialog(parent, Qt::Window), mLocked(false)
{
	ui.setupUi(this);

	const QString dataDir = QDir::toNativeSeparators(QDir(QString::fromUtf8(RsAccounts::ConfigDirectory().c_str())).absolutePath());
	ui.dataDirLabel->setText(dataDir);
	ui.dataDirLabel->setToolTip(dataDir);

	QHeaderView *header = ui.nodeTree->header();
	header->setSectionResizeMode(COLUMN_PROFILE, QHeaderView::ResizeToContents);
	header->setSectionResizeMode(COLUMN_NODE, QHeaderView::ResizeToContents);
	header->setSectionResizeMode(COLUMN_TYPE, QHeaderView::ResizeToContents);
	header->setSectionResizeMode(COLUMN_KEYS, QHeaderView::ResizeToContents);
	header->setSectionResizeMode(COLUMN_LASTUSED, QHeaderView::ResizeToContents);
	header->setStretchLastSection(true);
	ui.nodeTree->sortByColumn(COLUMN_PROFILE, Qt::AscendingOrder);

	connect(ui.backupButton, SIGNAL(clicked()), this, SLOT(backupNode()));
	connect(ui.restoreButton, SIGNAL(clicked()), this, SLOT(restoreNode()));
	connect(ui.upgradeButton, SIGNAL(clicked()), this, SLOT(upgradeNode()));
	connect(ui.nodeTree, SIGNAL(itemSelectionChanged()), this, SLOT(updateButtons()));
	connect(ui.openNodeFolderButton, SIGNAL(clicked()), this, SLOT(openNodeFolder()));
	connect(ui.openDataDirButton, SIGNAL(clicked()), this, SLOT(openDataDirectory()));
	connect(ui.changeDataDirButton, SIGNAL(clicked()), this, SLOT(changeDataDirectory()));

	// Data folder of a portable RetroShare: "portable" file next to it
	const bool portable = QFileInfo::exists(QDir(dataDir).absoluteFilePath("../portable"));
	if(portable)
		ui.dataDirTitleLabel->setText(tr("Data directory (portable):"));

	// Portable RetroShare keeps its nodes in its own folder: tell the user
	// how to find them when we are looking at the standard folder.
	ui.portableFrame->setVisible(!portable);
	connect(ui.choosePortableButton, SIGNAL(clicked()), this, SLOT(changeDataDirectory()));

	showKeyring(rnpExistedAtStart);

	RsPeerId preferredId;
	RsAccounts::GetPreferredAccountId(preferredId);
	fillNodeList(preferredId);
}

void ProfileToolWindow::showKeyring(bool rnpExistedAtStart)
{
	const QString dataDir = QDir(QString::fromUtf8(RsAccounts::ConfigDirectory().c_str())).absolutePath();

	switch(NodeInfo::keyring(dataDir, rnpExistedAtStart))
	{
	case NodeInfo::Keyring::Rnp:
		ui.keyringLabel->setText(tr("RNP"));
		ui.keyringLabel->setToolTip(tr("Current PGP keyring (folder \"rnp\")."));
		break;
	case NodeInfo::Keyring::RnpConvertedFromOpenPgpSdk:
		ui.keyringLabel->setText(tr("RNP (converted from OpenPGP-SDK)"));
		ui.keyringLabel->setToolTip(tr("This data folder only had the old OpenPGP-SDK keyring (folder \"pgp\").\n"
		                               "It was copied to a new RNP keyring (folder \"rnp\"), as newer RetroShare versions do.\n"
		                               "The old keyring was not changed."));
		break;
	case NodeInfo::Keyring::OpenPgpSdk:
		ui.keyringLabel->setText(tr("OpenPGP-SDK (old)"));
		ui.keyringLabel->setToolTip(tr("Old PGP keyring (folder \"pgp\")."));
		break;
	default:
		ui.keyringLabel->setText(tr("none"));
		ui.keyringLabel->setToolTip(QString());
		break;
	}
}

void ProfileToolWindow::fillNodeList(const RsPeerId& selectId)
{
	ui.nodeTree->setSortingEnabled(false);
	ui.nodeTree->clear();

	std::list<RsPeerId> accountIds;
	RsAccounts::GetAccountIds(accountIds);

	const QIcon nodeIcon(":/images/node.svg");
	QTreeWidgetItem *select = nullptr;

	for(const RsPeerId& id : accountIds)
	{
		RsPgpId pgpId;
		std::string name, email, node;
		RsAccounts::GetAccountDetails(id, pgpId, name, email, node);

		const QString pgpName = QString::fromUtf8(name.c_str());
		const QString pgpHex = QString::fromStdString(pgpId.toStdString());
		const bool hidden = QFileInfo(ProfileArchive::accountDirectory(id)).fileName().startsWith("HID06_");

		QTreeWidgetItem *item = new QTreeWidgetItem(ui.nodeTree);
		item->setIcon(COLUMN_PROFILE, nodeIcon);
		item->setText(COLUMN_PROFILE, pgpName);
		item->setText(COLUMN_NODE, QString::fromUtf8(node.c_str()));
		item->setText(COLUMN_TYPE, hidden ? tr("Hidden (Tor/I2P)") : tr("Standard"));

		const QString folder = ProfileArchive::accountDirectory(id);
		const NodeInfo::Certificate cert = NodeInfo::readCertificate(folder);
		item->setText(COLUMN_KEYS, cert.summary());
		item->setData(COLUMN_PROFILE, ROLE_KEYS_WEAK, cert.weak);
		if(cert.weak)
		{
			item->setIcon(COLUMN_KEYS, QIcon(":/images/warning.png"));
			item->setToolTip(COLUMN_KEYS, tr("Old keys: newer RetroShare versions may refuse them. Use \"Upgrade to new keys\"."));
		}

		const QDateTime last = NodeInfo::lastUsed(folder);
		item->setData(COLUMN_LASTUSED, Qt::DisplayRole, last.isValid() ? QVariant(last.date()) : QVariant());
		item->setToolTip(COLUMN_LASTUSED, last.isValid() ? QLocale().toString(last, QLocale::ShortFormat) : QString());
		item->setText(COLUMN_ID, QString::fromStdString(id.toStdString()));
		item->setToolTip(COLUMN_PROFILE, pgpName + " (" + pgpHex + ")");
		item->setData(COLUMN_PROFILE, ROLE_NODE_ID, QString::fromStdString(id.toStdString()));
		item->setData(COLUMN_PROFILE, ROLE_PGP_ID, pgpHex);
		item->setData(COLUMN_PROFILE, ROLE_PGP_NAME, pgpName);

		if(id == selectId || !select)
			select = item;
	}

	ui.nodeTree->setSortingEnabled(true);

	if(select)
		ui.nodeTree->setCurrentItem(select);

	if(accountIds.empty())
		ui.statusLabel->setText(tr("No node was found in this data directory. You can restore one from a backup."));

	updateButtons();
}

RsPeerId ProfileToolWindow::selectedNode() const
{
	QTreeWidgetItem *item = ui.nodeTree->currentItem();
	if(!item)
		return RsPeerId();

	return RsPeerId(item->data(COLUMN_PROFILE, ROLE_NODE_ID).toString().toStdString());
}

void ProfileToolWindow::updateButtons()
{
	const RsPeerId id = selectedNode();
	const bool haveNode = !id.isNull();

	ui.backupButton->setEnabled(!mLocked && haveNode);
	ui.upgradeButton->setEnabled(!mLocked && haveNode);
	ui.restoreButton->setEnabled(!mLocked);
	ui.nodeTree->setEnabled(!mLocked);

	/* details of the selected node */
	QTreeWidgetItem *item = ui.nodeTree->currentItem();
	const QString folder = haveNode ? ProfileArchive::accountDirectory(id) : QString();

	ui.detailsGroupBox->setEnabled(haveNode);
	ui.profileValueLabel->setText(item ? item->data(COLUMN_PROFILE, ROLE_PGP_NAME).toString() : QString());
	ui.pgpIdValueLabel->setText(item ? item->data(COLUMN_PROFILE, ROLE_PGP_ID).toString() : QString());
	ui.nodeValueLabel->setText(item ? item->text(COLUMN_NODE) + "  (" + item->text(COLUMN_TYPE) + ")" : QString());
	ui.nodeIdValueLabel->setText(item ? item->text(COLUMN_ID) : QString());
	ui.keysValueLabel->setText(!item ? QString() :
	        item->data(COLUMN_PROFILE, ROLE_KEYS_WEAK).toBool()
	            ? item->text(COLUMN_KEYS) + " - " + tr("upgrade recommended")
	            : item->text(COLUMN_KEYS));
	ui.folderValueLabel->setText(QDir::toNativeSeparators(folder));
	ui.folderValueLabel->setToolTip(QDir::toNativeSeparators(folder));
	ui.openNodeFolderButton->setEnabled(!folder.isEmpty());
}

void ProfileToolWindow::openNodeFolder()
{
	const QString folder = ProfileArchive::accountDirectory(selectedNode());
	if(!folder.isEmpty())
		QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}

void ProfileToolWindow::openDataDirectory()
{
	QDesktopServices::openUrl(QUrl::fromLocalFile(QDir(QString::fromUtf8(RsAccounts::ConfigDirectory().c_str())).absolutePath()));
}

void ProfileToolWindow::changeDataDirectory()
{
	QString dir = QFileDialog::getExistingDirectory(this,
	        tr("Choose a RetroShare folder (portable) or data directory"),
	        QDir(QString::fromUtf8(RsAccounts::ConfigDirectory().c_str())).absolutePath());

	if(dir.isEmpty())
		return;

	// libretroshare reads its keyring only once, so the tool restarts itself
	// on the new directory (main.cpp turns a portable folder into its Data folder).
	if(!QProcess::startDetached(QApplication::applicationFilePath(), QStringList() << "--base-dir" << dir))
	{
		QMessageBox::warning(this, windowTitle(), tr("Cannot restart the tool."));
		return;
	}
	QApplication::quit();
}

void ProfileToolWindow::lockTool(const QString& message)
{
	mLocked = true;
	ui.statusLabel->setText(message);
	updateButtons();
}

void ProfileToolWindow::backupNode()
{
	RsPeerId id = selectedNode();
	if(id.isNull())
		return;

	RsPgpId pgpId;
	std::string name, email, node;
	RsAccounts::GetAccountDetails(id, pgpId, name, email, node);

	QString docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
	if(docs.isEmpty())
		docs = QDir::homePath();

	QString suggested = QString("%1-%2.%3").arg(QString::fromUtf8(name.c_str()), QString::fromUtf8(node.c_str()), ProfileArchive::fileExtension());
	suggested.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");

	QString file = QFileDialog::getSaveFileName(this, tr("Back up node"), QDir(docs).absoluteFilePath(suggested),
	                                            tr("RetroShare node backup (*.%1)").arg(ProfileArchive::fileExtension()));
	if(file.isEmpty())
		return;

	if(!file.endsWith("." + ProfileArchive::fileExtension(), Qt::CaseInsensitive))
		file += "." + ProfileArchive::fileExtension();

	QProgressDialog progress(tr("Saving the backup..."), tr("Cancel"), 0, 1000, this);
	progress.setWindowModality(Qt::WindowModal);
	progress.setMinimumDuration(500);

	QString error;
	bool ok = ProfileArchive::exportProfile(id, file, error,
	        [&progress](qint64 done, qint64 total, const QString&)
	{
		progress.setValue(total > 0 ? int(done * 1000 / total) : 0);
		QApplication::processEvents();
		return !progress.wasCanceled();
	});
	progress.reset();

	if(ok)
	{
		ui.statusLabel->setText(tr("Backup saved to %1").arg(QDir::toNativeSeparators(file)));
		QMessageBox::information(this, tr("Back up node"),
		                         tr("The node was saved to:") + "\n" + QDir::toNativeSeparators(file) + "\n\n" +
		                         tr("Keep this file safe: anyone who has it and knows your password can use your node."));
	}
	else
		QMessageBox::warning(this, tr("Back up node"), tr("The backup failed:") + "\n" + error);
}

void ProfileToolWindow::restoreNode()
{
	QString docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

	QString file = QFileDialog::getOpenFileName(this, tr("Restore node"), docs,
	                                            tr("RetroShare node backup (*.%1);;All files (*)").arg(ProfileArchive::fileExtension()));
	if(file.isEmpty())
		return;

	ProfileArchive::Info info;
	QString error;
	if(!ProfileArchive::readInfo(file, info, error))
	{
		QMessageBox::warning(this, tr("Restore node"), error);
		return;
	}

	QString question = tr("Restore this node?") + "\n\n"
	        + tr("Profile:") + " " + info.pgpName + " (" + QString::fromStdString(info.pgpId.toStdString()) + ")\n"
	        + tr("Node:") + " " + info.locationName + "\n"
	        + tr("Node ID:") + " " + QString::fromStdString(info.sslId.toStdString()) + "\n"
	        + tr("Saved:") + " " + info.createdAt + "\n\n"
	        + tr("Do not run the same node on two computers at the same time.");

	if(QMessageBox::question(this, tr("Restore node"), question, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
		return;

	QProgressDialog progress(tr("Restoring the node..."), tr("Cancel"), 0, 1000, this);
	progress.setWindowModality(Qt::WindowModal);
	progress.setMinimumDuration(500);

	bool ok = ProfileArchive::importProfile(file, info, error,
	        [&progress](qint64 done, qint64 total, const QString&)
	{
		progress.setValue(total > 0 ? int(done * 1000 / total) : 0);
		QApplication::processEvents();
		return !progress.wasCanceled();
	});
	progress.reset();

	if(!ok)
	{
		QMessageBox::warning(this, tr("Restore node"), tr("The node could not be restored:") + "\n" + error);
		return;
	}

	fillNodeList(info.sslId);
	ui.statusLabel->setText(tr("Node \"%1\" restored. Start RetroShare to log in.").arg(info.locationName));
}

void ProfileToolWindow::upgradeNode()
{
	RsPeerId id = selectedNode();
	if(id.isNull())
		return;

	ProfileUpgradeDialog dlg(id, this);
	dlg.setFinishButtonText(tr("Done"));

	if(dlg.exec() == QDialog::Accepted)
	{
		// The new node was created and logged in by this tool, so its folder
		// (and the old one) stay locked until the tool is closed.
		fillNodeList(dlg.newNodeId());
		lockTool(tr("Upgrade done. Close this tool, then start RetroShare and log in with the new node "
		            "(ID %1).").arg(QString::fromStdString(dlg.newNodeId().toStdString())));
		return;
	}

	if(dlg.needsRestart())
		lockTool(tr("The upgrade failed. Please close and restart this tool before trying again."));
}
