/*******************************************************************************
 * gui/StartDialog.cpp                                                         *
 *                                                                             *
 * Copyright (c) 2006 Crypton          <retroshare.project@gmail.com>          *
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

#include "StartDialog.h"

#include "LogoBar.h"
#include "rshare.h"
#include "settings/rsharesettings.h"
#include "gui/common/FilesDefs.h"

#include "retroshare/rsinit.h"
#include "gui/profile/ProfileArchive.h"
#include "gui/profile/ProfileUpgradeDialog.h"
#include "util/misc.h"

#include <QApplication>
#include <QDir>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressDialog>

#include <iostream>

/** Default constructor */
StartDialog::StartDialog(QWidget *parent)
  : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint), reqNewCert(false)
{
	/* Invoke Qt Designer generated QObject setup routine */
	ui.setupUi(this);

#ifdef RS_AUTOLOGIN
	connect(ui.autologin_checkbox, SIGNAL(clicked()), this, SLOT(notSecureWarning()));
#else
	ui.autologin_checkbox->setHidden(true);
#endif

	Settings->loadWidgetInformation(this);
	
	/* Put the Login dialog in the screen center */
	const QRect screen = RsApplication::primaryScreenGeometry();
	this->move( screen.center() - this->rect().center() );

	RsPeerId preferedId;
	RsAccounts::GetPreferredAccountId(preferedId);
	fillNodeList(preferedId);

	QObject::connect(ui.loadName,SIGNAL(currentIndexChanged(int)),this,SLOT(updateSelectedProfile(int))) ;
	//QObject::connect(ui.password_input,SIGNAL(returnPressed()),this,SLOT(loadPerson())) ;//Already called by StartDialog.returnPressed->loadButton.clicked
	QObject::connect(ui.loadButton, SIGNAL(clicked()), this, SLOT(loadPerson()));

	/* node backup / restore / upgrade */
	ui.nodeToolsButton->addAction(ui.actionBackupNode);
	ui.nodeToolsButton->addAction(ui.actionRestoreNode);
	ui.nodeToolsButton->addAction(ui.actionUpgradeNode);
	connect(ui.actionBackupNode, SIGNAL(triggered()), this, SLOT(backupNode()));
	connect(ui.actionRestoreNode, SIGNAL(triggered()), this, SLOT(restoreNode()));
	connect(ui.actionUpgradeNode, SIGNAL(triggered()), this, SLOT(upgradeNode()));

	ui.password_input->setFocus();

	if (Settings->getSheetName() == ":Standard_Dark")
		ui.rsTopLogo->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/logo/logo_web_nobackground_black.png"));
}

void StartDialog::fillNodeList(const RsPeerId& selectId)
{
	/* get all available pgp private certificates....
	* mark the given one as default.
	*/
	ui.loadName->blockSignals(true);
	ui.loadName->clear();

	std::list<RsPeerId> accountIds;
	int pidx = -1;
	int i = 0;

	if (RsAccounts::GetAccountIds(accountIds))
	{
		for(std::list<RsPeerId>::const_iterator it = accountIds.begin(); it != accountIds.end(); ++it, ++i)
		{
			const QVariant & userData = QVariant(QString::fromStdString((*it).toStdString()));
			RsPgpId gpgid ;
			std::string name, email, node;
			RsAccounts::GetAccountDetails(*it, gpgid, name, email, node);
			QString accountName = QString::fromUtf8(name.c_str()) + " (" + QString::fromStdString(gpgid.toStdString()).right(8) + ") - " + QString::fromUtf8(node.c_str());
			ui.loadName->addItem(accountName, userData);

			if (selectId == *it)
			{
				pidx = i;
			}
		}
	}

	if (pidx >= 0)
	{
		ui.loadName->setCurrentIndex(pidx);
	}
	ui.loadName->blockSignals(false);
}

RsPeerId StartDialog::selectedNode() const
{
	int idx = ui.loadName->currentIndex();
	if (idx < 0)
		return RsPeerId();

	return RsPeerId(ui.loadName->itemData(idx).toString().toStdString());
}

void StartDialog::updateSelectedProfile(int)
{
	ui.password_input->clear();
	ui.password_input->setPlaceholderText(tr("Password"));
	ui.password_input->setFocus();
}

void StartDialog::closeEvent (QCloseEvent * event)
{
	Settings->saveWidgetInformation(this);

	QWidget::closeEvent(event);
}

void StartDialog::loadPerson()
{
	int pgpidx = ui.loadName->currentIndex();
	if (pgpidx < 0)
	{
		/* Message Dialog */
		QMessageBox::warning(this, tr("Load Person Failure"), tr("Missing PGP Certificate"), QMessageBox::Ok);
		return;
	}

	QVariant data = ui.loadName->itemData(pgpidx);
	RsPeerId accountId = RsPeerId((data.toString()).toStdString());

	// Cache the passphrase, so that it is not asked again.
    RsLoginHelper::cachePgpPassphrase(ui.password_input->text().toUtf8().constData()) ;

    bool res = RsApplication::loadCertificate(accountId, ui.autologin_checkbox->isChecked()) ;

    RsLoginHelper::clearPgpPassphrase();

	if(res)
		accept();
	else
	{
		ui.password_input->setPlaceholderText(tr("Wrong password"));
		ui.password_input->setText("");
		ui.password_input->setFocus();
	}
}

void StartDialog::on_labelProfile_linkActivated(QString /*link*/)
{
		reqNewCert = true;
		accept();
}

bool StartDialog::requestedNewCert()
{
	return reqNewCert;
}

void StartDialog::backupNode()
{
	RsPeerId id = selectedNode();
	if (id.isNull())
		return;

	RsPgpId gpgid;
	std::string name, email, node;
	RsAccounts::GetAccountDetails(id, gpgid, name, email, node);

	QString file = QString("%1-%2.%3").arg(QString::fromUtf8(name.c_str()), QString::fromUtf8(node.c_str()), ProfileArchive::fileExtension());
	if (!misc::getSaveFileName(this, RshareSettings::LASTDIR_CERT, tr("Back up node"),
	                           tr("RetroShare node backup (*.%1)").arg(ProfileArchive::fileExtension()), file))
		return;

	if (!file.endsWith(QString(".") + ProfileArchive::fileExtension(), Qt::CaseInsensitive))
		file += QString(".") + ProfileArchive::fileExtension();

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

	if (ok)
		QMessageBox::information(this, tr("Back up node"),
		                         tr("The node was saved to:") + "\n" + QDir::toNativeSeparators(file) + "\n\n" +
		                         tr("Keep this file safe: anyone who has it and knows your password can use your node."));
	else
		QMessageBox::warning(this, tr("Back up node"), tr("The backup failed:") + "\n" + error);
}

void StartDialog::restoreNode()
{
	QString file;
	if (!misc::getOpenFileName(this, RshareSettings::LASTDIR_CERT, tr("Restore node"),
	                           tr("RetroShare node backup (*.%1);;All files (*)").arg(ProfileArchive::fileExtension()), file))
		return;

	ProfileArchive::Info info;
	QString error;
	if (!ProfileArchive::readInfo(file, info, error))
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

	if (QMessageBox::question(this, tr("Restore node"), question, QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
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

	if (!ok)
	{
		QMessageBox::warning(this, tr("Restore node"), tr("The node could not be restored:") + "\n" + error);
		return;
	}

	fillNodeList(info.sslId);
	ui.password_input->clear();
	ui.password_input->setFocus();

	QMessageBox::information(this, tr("Restore node"), tr("The node was restored. Enter your password to log in."));
}

void StartDialog::upgradeNode()
{
	RsPeerId id = selectedNode();
	if (id.isNull())
		return;

	ProfileUpgradeDialog dlg(id, this);

	if (dlg.exec() == QDialog::Accepted)
	{
		/* the new node is already logged in: continue the start-up with it */
		accept();
		return;
	}

	if (dlg.needsRestart())
	{
		ui.loadButton->setEnabled(false);
		ui.password_input->setEnabled(false);
		ui.nodeToolsButton->setEnabled(false);
		ui.password_input->setPlaceholderText(tr("Please restart RetroShare"));
	}
}

#ifdef RS_AUTOLOGIN
void StartDialog::notSecureWarning()
{
	/* some error msg */
	if (ui.autologin_checkbox->isChecked())
#ifdef WINDOWS_SYS
        QMessageBox::warning ( this, tr("Warning"), tr("The password to your SSL certificate (your node) will be stored encrypted in the keys/help.dta file. This is not secure. \n\n Your PGP password will not be stored.\n\nThis choice can be reverted in settings."), QMessageBox::Ok);
#else
#ifdef __APPLE__
		QMessageBox::warning ( this, tr("Warning"), tr("The password to your SSL certificate (your node) will be stored encrypted in your Keychain. \n\n Your PGP passwd will not be stored.\n\nThis choice can be reverted in settings."), QMessageBox::Ok);
#else
        // this handles all linux systems at once.
        QMessageBox::warning ( this, tr("Warning"), tr("The password to your SSL certificate (your node) will be stored encrypted in your desktop's keyring. \n\n Your PGP passwd will not be stored.\n\nThis choice can be reverted in settings."), QMessageBox::Ok);
#endif
#endif
}
#endif // RS_AUTOLOGIN
