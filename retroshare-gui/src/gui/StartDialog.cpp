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
#include "InfoDialog.h"
#include "LogoBar.h"
#include <QFileDialog>
#include <QMessageBox>
#include "util/Widget.h"
#include "settings/rsharesettings.h"

/* Define the format used for displaying the date and time */
#define DATETIME_FMT  "MMM dd hh:mm:ss"



/** Default constructor */
StartDialog::StartDialog(QWidget *parent, Qt::WFlags flags)
  : QMainWindow(parent, flags), reqNewCert(false)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);

  /* Create Bandwidth Graph related QObjects */
  Settings->loadWidgetInformation(this);
 
  _rsLogoBar = NULL;
 
  //LogoBar
  _rsLogoBar = new LogoBar(ui.callBarFrame);
  Widget::createLayout(ui.callBarFrame)->addWidget(_rsLogoBar);
  
  ui.loadButton->setFocus();


  connect(ui.loadButton, SIGNAL(clicked()), this, SLOT(loadPerson()));
  connect(ui.autologin_checkbox, SIGNAL(clicked()), this, SLOT(notSecureWarning()));

  /* load the Certificate File name */
  std::string userName;

	/* get all available pgp private certificates....
	 * mark last one as default.
	 */
	std::list<std::string> accountIds;
	std::list<std::string>::iterator it;
	std::string preferedId;
	RsInit::getPreferedAccountId(preferedId);
	int pidx = -1;
	int i;

	if (RsInit::getAccountIds(accountIds))
	{
		for(it = accountIds.begin(), i = 0; it != accountIds.end(); it++, i++)
		{
			const QVariant & userData = QVariant(QString::fromStdString(*it));
                        std::string gpgid, name, email, location;
                        RsInit::getAccountDetails(*it, gpgid, name, email, location);
                        QString accountName = QString::fromStdString(name + " (") + QString::fromStdString(gpgid).right(8) + ") - " + QString::fromStdString(location);
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



/** 
 Overloads the default show() slot so we can set opacity*/

void StartDialog::show()
{
  //loadSettings();
  if(!this->isVisible()) {
    QMainWindow::show();

  }
}

void StartDialog::closeEvent (QCloseEvent * event)
{
 Settings->saveWidgetInformation(this);

 QWidget::closeEvent(event);
}

void StartDialog::closeinfodlg()
{
	close();
}



void StartDialog::loadPerson()
{
        std::string accountId = "";

        int pgpidx = ui.loadName->currentIndex();
        if (pgpidx < 0)
        {
                /* Message Dialog */
                QMessageBox::warning ( this,
                                "Load Person Failure",
                                "Missing PGP Certificate",
                                  QMessageBox::Ok);
                return;
        }

        QVariant data = ui.loadName->itemData(pgpidx);
        accountId = (data.toString()).toStdString();

	std::string gpgId, gpgName, gpgEmail, sslName;
	if (RsInit::getAccountDetails(accountId, 
			gpgId, gpgName, gpgEmail, sslName))
	{
		RsInit::SelectGPGAccount(gpgId);
	}

        RsInit::LoadPassword(accountId, "");
	loadCertificates();
}

void StartDialog::loadCertificates()
{
	/* Final stage of loading */
	int retVal = RsInit::LockAndLoadCertificates(ui.autologin_checkbox->isChecked());
	switch(retVal)
	{
		case 0: close();
				break;
		case 1:	QMessageBox::warning(	this,
										tr("Multiple instances"),
										tr("Another RetroShare using the same profile is "
											"already running on your system. Please close "
											"that instance first, or choose another profile") );
				break;
		case 2:	QMessageBox::warning(	this,
										tr("Multiple instances"),
										tr("An unexpected error occurred when Retroshare"
											"tried to acquire the single instance lock") );
				break;
		case 3: QMessageBox::warning(	this,
										tr("Login Failure"),
										tr("Maybe password is wrong") );
				break;
		default: std::cerr << "StartDialog::loadCertificates() unexpected switch value " << retVal << std::endl;
	}
}



void StartDialog::on_labelProfile_linkActivated(QString link)
{

    if ((QMessageBox::question(this, tr("Create a New Profile"),tr("This will generate a new Profile\n Are you sure you want to continue?"),QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes))== QMessageBox::Yes)
    {
        reqNewCert = true;
        close();
    }
    else
    return;                    

}

void StartDialog::on_labelInfo_linkActivated(QString link)
{
    InfoDialog infodialog (this);
    infodialog.exec ();
}

bool StartDialog::requestedNewCert()
{
	return reqNewCert;
}

LogoBar & StartDialog::getLogoBar() const {
	return *_rsLogoBar;
}

void StartDialog::notSecureWarning() {

	/* some error msg */
		if(ui.autologin_checkbox->isChecked())
#ifdef UBUNTU
				QMessageBox::warning ( this, tr("Warning"), tr(" The passwd to your SSL certificate (your location) will be stored encrypted in your Gnome Keyring. \n\n Your PGP passwd will not be stored.\n\nThis choice can be reverted in settings."), QMessageBox::Ok);
#else
				QMessageBox::warning ( this, tr("Warning"), tr(" The passwd to your SSL certificate (your location) will be stored encrypted in the keys/help.dta file. This is not secure. \n\n Your PGP passwd will not be stored.\n\nThis choice can be reverted in settings."), QMessageBox::Ok);
#endif

		return;
}
