/*******************************************************************************
 * gui/profile/ProfileUpgradeDialog.cpp                                        *
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

#include "ProfileUpgradeDialog.h"

#include "ProfileArchive.h"
#include "ProfileUpgrader.h"

#include "retroshare/rsinit.h"
#include "util/misc.h"

#include <QApplication>
#include <QDate>
#include <QDir>
#include <QEventLoop>
#include <QMessageBox>
#include <QRegularExpression>
#include <QScrollBar>
#include <QStandardPaths>

ProfileUpgradeDialog::ProfileUpgradeDialog(const RsPeerId& sslId, QWidget *parent)
    : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
    , mOldId(sslId), mRunning(false), mDone(false), mNeedsRestart(false)
{
	ui.setupUi(this);

	RsPgpId pgpId;
	std::string pgpName, pgpEmail, location;
	RsAccounts::GetAccountDetails(sslId, pgpId, pgpName, pgpEmail, location);

	const QString name = QString::fromUtf8(pgpName.c_str());
	const QString loc = QString::fromUtf8(location.c_str());

	ui.nodeLabel->setText(QString("%1 (%2) - %3")
	                      .arg(name, QString::fromStdString(pgpId.toStdString()).right(8), loc));
	ui.locationEdit->setText(loc);

	QString docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
	if(docs.isEmpty())
		docs = QDir::homePath();

	QString fileName = QString("%1-%2-%3.%4").arg(name, loc, QDate::currentDate().toString("yyyyMMdd"),
	                                              ProfileArchive::fileExtension());
	fileName.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
	ui.backupPathEdit->setText(QDir::toNativeSeparators(QDir(docs).absoluteFilePath(fileName)));

	ui.progressBar->hide();
	ui.logEdit->hide();

	connect(ui.upgradeButton, SIGNAL(clicked()), this, SLOT(upgradeClicked()));
	connect(ui.backupBrowseButton, SIGNAL(clicked()), this, SLOT(browseBackup()));

	ui.passwordEdit->setFocus();
}

void ProfileUpgradeDialog::reject()
{
	if(mRunning)
		return;		// cannot stop half-way

	if(mDone)
	{
		// The new node is already logged in: closing means "continue".
		accept();
		return;
	}
	QDialog::reject();
}

void ProfileUpgradeDialog::browseBackup()
{
	QString file = ui.backupPathEdit->text();
	if(misc::getSaveFileName(this, RshareSettings::LASTDIR_CERT, tr("Save node backup"),
	                         tr("RetroShare node backup (*.%1)").arg(ProfileArchive::fileExtension()), file))
		ui.backupPathEdit->setText(QDir::toNativeSeparators(file));
}

void ProfileUpgradeDialog::appendLog(const QString& line)
{
	ui.logEdit->appendPlainText(line);
	ui.logEdit->verticalScrollBar()->setValue(ui.logEdit->verticalScrollBar()->maximum());
	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void ProfileUpgradeDialog::setRunning(bool running)
{
	mRunning = running;

	ui.passwordEdit->setEnabled(!running);
	ui.locationEdit->setEnabled(!running);
	ui.backupCheckBox->setEnabled(!running);
	ui.backupPathEdit->setEnabled(!running && ui.backupCheckBox->isChecked());
	ui.backupBrowseButton->setEnabled(!running && ui.backupCheckBox->isChecked());
	ui.upgradeButton->setEnabled(!running);
	ui.closeButton->setEnabled(!running);

	if(running)
		QApplication::setOverrideCursor(Qt::WaitCursor);
	else
		QApplication::restoreOverrideCursor();
}

void ProfileUpgradeDialog::upgradeClicked()
{
	if(mDone)
	{
		accept();
		return;
	}

	if(ui.passwordEdit->text().isEmpty())
	{
		ui.statusLabel->setText(tr("Please enter your password."));
		ui.passwordEdit->setFocus();
		return;
	}

	if(ui.locationEdit->text().trimmed().isEmpty())
	{
		ui.statusLabel->setText(tr("Please enter a name for the new node."));
		ui.locationEdit->setFocus();
		return;
	}

	if(ui.backupCheckBox->isChecked() && ui.backupPathEdit->text().trimmed().isEmpty())
	{
		ui.statusLabel->setText(tr("Please choose where to save the backup."));
		return;
	}

	ui.logEdit->clear();
	ui.logEdit->show();
	ui.statusLabel->clear();

	setRunning(true);

	if(!ui.backupCheckBox->isChecked() || runBackup())
		runUpgrade();

	setRunning(false);

	if(mDone)
	{
		ui.upgradeButton->setText(tr("Start RetroShare"));
		ui.closeButton->hide();
		ui.passwordEdit->setEnabled(false);
		ui.locationEdit->setEnabled(false);
		ui.backupCheckBox->setEnabled(false);
		ui.backupPathEdit->setEnabled(false);
		ui.backupBrowseButton->setEnabled(false);
	}
	else if(mNeedsRestart)
	{
		ui.upgradeButton->setEnabled(false);
		ui.closeButton->setText(tr("Close"));
	}
}

bool ProfileUpgradeDialog::runBackup()
{
	const QString path = QDir::fromNativeSeparators(ui.backupPathEdit->text().trimmed());

	appendLog(tr("Saving a backup to %1...").arg(QDir::toNativeSeparators(path)));

	ui.progressBar->setValue(0);
	ui.progressBar->show();

	QString error;
	bool ok = ProfileArchive::exportProfile(mOldId, path, error,
	        [this](qint64 done, qint64 total, const QString&)
	{
		ui.progressBar->setValue(total > 0 ? int(done * 1000 / total) : 0);
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
		return true;
	});

	ui.progressBar->hide();

	if(!ok)
	{
		appendLog(tr("Backup failed:") + " " + error);
		ui.statusLabel->setText(tr("The backup failed, nothing was changed:") + " " + error);
		return false;
	}

	appendLog(tr("Backup saved."));
	return true;
}

void ProfileUpgradeDialog::runUpgrade()
{
	ProfileUpgrader::Result res = ProfileUpgrader::upgrade(
	            mOldId, ui.passwordEdit->text(), ui.locationEdit->text().trimmed(),
	            [this](const QString& m) { appendLog(m); });

	ui.passwordEdit->clear();

	if(!res.ok)
	{
		mNeedsRestart = res.oldNodeStillLoaded;
		appendLog(tr("Upgrade failed:") + " " + res.error);

		QString msg = tr("The upgrade failed:") + " " + res.error;
		if(mNeedsRestart)
			msg += "\n" + tr("Please restart RetroShare before trying again or logging in.");
		ui.statusLabel->setText(msg);
		return;
	}

	mDone = true;
	mNewId = res.newSslId;

	QString msg = tr("Your node was upgraded. New node ID: %1").arg(QString::fromStdString(mNewId.toStdString()));
	if(res.configFilesFailed > 0)
		msg += "\n" + tr("%1 configuration file(s) could not be migrated, see the log above.").arg(res.configFilesFailed);
	msg += "\n" + tr("Send your new certificate to your friends so that they can connect to you.");

	ui.statusLabel->setText(msg);
}
