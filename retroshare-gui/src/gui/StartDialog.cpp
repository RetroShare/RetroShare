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
#include <rsiface/rsinit.h>
#include "StartDialog.h"
#include "GenCertDialog.h"
#include "InfoDialog.h"
#include "LogoBar.h"
#include <QFileDialog>
#include <QMessageBox>
#include "util/Widget.h"

/* Define the format used for displaying the date and time */
#define DATETIME_FMT  "MMM dd hh:mm:ss"



/** Default constructor */
StartDialog::StartDialog(QWidget *parent, Qt::WFlags flags)
  : QMainWindow(parent, flags), reqNewCert(false)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);

  RshareSettings config;
  config.loadWidgetInformation(this);
 
  _rsLogoBar = NULL;
 
  //LogoBar
  _rsLogoBar = new LogoBar(ui.callBarFrame);
  Widget::createLayout(ui.callBarFrame)->addWidget(_rsLogoBar);

  /* Create Bandwidth Graph related QObjects */
  _settings = new RshareSettings();
  
  ui.loadButton->setFocus();

  connect(ui.loadButton, SIGNAL(clicked()), this, SLOT(loadPerson()));

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
                        QString accountName = QString::fromStdString(name + " (" + gpgid + ") - " + location);
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
 RshareSettings config;
 config.saveWidgetInformation(this);

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
                QMessageBox::StandardButton sb = QMessageBox::warning ( NULL,
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
	if (RsInit::LoadCertificates(false))
	{
		close();
	}
	else
	{
		/* some error msg */
                QMessageBox::StandardButton sb = QMessageBox::warning ( NULL,
                                tr("Login Failure"),
                                tr("Maybe password is wrong"),
				QMessageBox::Ok);
	}
}

void StartDialog::on_labelProfile_linkActivated(QString link)
{
    //static GenCertDialog *gencertdialog = new GenCertDialog();
    //gencertdialog->show();
    
    QMessageBox::StandardButton sb = QMessageBox::question ( NULL,
                        tr("Create a New Profil"),
                        tr("This will generate a new Profile\n Are you sure you want to continue"),
			(QMessageBox::Ok | QMessageBox::No));

    if (sb == QMessageBox::Ok)
    {
    	reqNewCert = true;
    	close();
    }
}

void StartDialog::on_labelInfo_linkActivated(QString link)
{
    static InfoDialog *infodialog = new InfoDialog();
    infodialog->show();
}

bool StartDialog::requestedNewCert()
{
	return reqNewCert;
}

LogoBar & StartDialog::getLogoBar() const {
	return *_rsLogoBar;
}

