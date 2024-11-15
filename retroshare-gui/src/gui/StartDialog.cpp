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

#include "retroshare/rsinit.h"
#include "retroshare/rsnotify.h"

#include <QLineEdit>
#include <QMessageBox>
#include <QDesktopWidget>

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
	const QRect screen = QApplication::desktop()->screenGeometry();
	this->move( screen.center() - this->rect().center() );

	/* get all available pgp private certificates....
	* mark last one as default.
	*/
	std::list<RsPeerId> accountIds;
	std::list<RsPeerId>::iterator it;
	RsPeerId preferedId;
	RsAccounts::GetPreferredAccountId(preferedId);
	int pidx = -1;
	int i;

	if (RsAccounts::GetAccountIds(accountIds))
	{
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

	QObject::connect(ui.loadName,SIGNAL(currentIndexChanged(int)),this,SLOT(updateSelectedProfile(int))) ;
	//QObject::connect(ui.password_input,SIGNAL(returnPressed()),this,SLOT(loadPerson())) ;//Already called by StartDialog.returnPressed->loadButton.clicked
	QObject::connect(ui.loadButton, SIGNAL(clicked()), this, SLOT(loadPerson()));

	if (pidx > 0)
	{
		ui.loadName->setCurrentIndex(pidx);
	}

	ui.password_input->setFocus();
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
	rsNotify->cachePgpPassphrase(ui.password_input->text().toUtf8().constData()) ;
	rsNotify->setDisableAskPassword(true);

    bool res = RsApplication::loadCertificate(accountId, ui.autologin_checkbox->isChecked()) ;

	rsNotify->setDisableAskPassword(false);
    rsNotify->clearPgpPassphrase();

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
