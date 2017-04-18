/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, 2007 crypton
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
#include <retroshare/rsinit.h>
#include "StartDialog.h"
#include "LogoBar.h"
#include <QMessageBox>
#include "settings/rsharesettings.h"

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

	ui.loadButton->setFocus();

	connect(ui.loadButton, SIGNAL(clicked()), this, SLOT(loadPerson()));

	/* get all available pgp private certificates....
	* mark last one as default.
	*/
	std::list<RsPeerId> accountIds;
	std::list<RsPeerId>::iterator it;
	RsPeerId preferedId;
	RsAccounts::GetPreferredAccountId(preferedId);
	int pidx = -1;

	if (RsAccounts::GetAccountIds(accountIds))
	{
		int i;
		for(it = accountIds.begin(), i = 0; it != accountIds.end(); ++it, ++i)
		{
			const QVariant & userData = QVariant(QString::fromStdString((*it).toStdString()));
			RsPgpId gpgid ;
			std::string name, email, node;
			RsAccounts::GetAccountDetails(*it, gpgid, name, email, node);
			QString accountName = QString::fromUtf8(name.c_str()) + " (" + QString::fromStdString(gpgid.toStdString()).right(8) + ") - " + QString::fromUtf8(node.c_str());
			ui.loadName->addItem(accountName, userData);

			if (preferedId == *it)
			{
				pidx = i;
			}
		}
	}

	if (pidx > 0)
	{
		ui.loadName->setCurrentIndex(pidx);
	}
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

	if (Rshare::loadCertificate(accountId, ui.autologin_checkbox->isChecked())) {
		accept();
	}
}

void StartDialog::on_labelProfile_linkActivated(QString /*link*/)
{
//	if ((QMessageBox::question(this, tr("Create a New Profile"),tr("This will generate a new Profile\n Are you sure you want to continue?"),QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes))== QMessageBox::Yes)
//	{
		reqNewCert = true;
		accept();
//	}
}

bool StartDialog::requestedNewCert()
{
	return reqNewCert;
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
        QMessageBox::warning ( this, tr("Warning"), tr("The password to your SSL certificate (your node) will be stored encrypted in your Gnome Keyring. \n\n Your PGP passwd will not be stored.\n\nThis choice can be reverted in settings."), QMessageBox::Ok);
#endif
#endif
}
#endif // RS_AUTOLOGIN
