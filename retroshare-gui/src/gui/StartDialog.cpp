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

/**
#if (QT_VERSION >= 040300)
  skinobject = new QSkinObject(this);
  skinobject->startSkinning();
#endif**/

  RshareSettings config;
  config.loadWidgetInformation(this);
 
  _rsLogoBar = NULL;
 
  //LogoBar
  _rsLogoBar = new LogoBar(ui.callBarFrame);
  Widget::createLayout(ui.callBarFrame)->addWidget(_rsLogoBar);

  /* Create Bandwidth Graph related QObjects */
  _settings = new RshareSettings();
  
  ui.loadPasswd->setFocus();
  
  //connect(ui.genButton, SIGNAL(clicked()), this, SLOT(genPerson()));
  connect(ui.loadButton, SIGNAL(clicked()), this, SLOT(loadPerson()));
  connect(ui.loadPasswd, SIGNAL(returnPressed()), this, SLOT(loadPerson()));
  connect(ui.loadGPGPasswd, SIGNAL(returnPressed()), this, SLOT(loadPerson()));
  //connect(ui.selectButton, SIGNAL(clicked()), this, SLOT(selectFriend()));
  //connect(ui.friendBox, SIGNAL(stateChanged(int)), this, SLOT(checkChanged(int)));
  connect(ui.createaccountButton, SIGNAL(clicked()), this, SLOT(createnewaccount()));
  connect(ui.infoButton,SIGNAL(clicked()), this, SLOT(infodlg()));

  /* load the Certificate File name */
  std::string userName;

#ifdef RS_USE_PGPSSL

#ifndef WINDOWS_SYS /* UNIX */
	//hide autologin because it's not functionnal on other than windows system
	ui.autoBox->hide();

	//comment those to show the pgp and ssl password dialog
	ui.loadPasswd->hide();
	ui.label_4->hide();

	ui.loadGPGPasswd->hide();
	ui.label_5->hide();
#endif

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
			std::string gpgid, name, email, sslname;
			RsInit::getAccountDetails(*it, gpgid, name, email, sslname);
       			QString accountName = QString::fromStdString(name);
       			accountName += "/";
       			accountName += QString::fromStdString(sslname);
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

#if 0
	std::list<std::string> pgpIds;
	std::list<std::string>::iterator it;
	if (RsInit::GetPGPLogins(pgpIds))
	{
		for(it = pgpIds.begin(); it != pgpIds.end(); it++)
		{
			const QVariant & userData = QVariant(QString::fromStdString(*it));
			std::string name, email;
			RsInit::GetPGPLoginDetails(*it, name, email);
       			ui.loadName->addItem(QString::fromStdString(name), userData);
		}
	}
#endif

#else

  if (RsInit::ValidateCertificate(userName))
  {
  	/* just need to enter password */
        ui.loadName->addItem(QString::fromStdString(userName));
	//ui.loadName->setText(QString::fromStdString(userName));
	ui.loadPasswd->setFocus(Qt::OtherFocusReason);
	ui.loadButton -> setEnabled(true);
  }
  else
  {
  	/* need to generate new user */
        ui.loadName->addItem("<No Existing User>");
	//ui.loadName->setText("<No Existing User>");
	ui.loadButton -> setEnabled(false);
	//ui.genName->setFocus(Qt::OtherFocusReason);
  }
#ifndef Q_WS_WIN
	ui.autoBox->setChecked(false) ;
	ui.autoBox->setEnabled(false) ;
#endif
#endif

  //ui.genFriend -> setText("<None Selected>");

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
	std::string passwd = ui.loadPasswd->text().toStdString();
#ifdef RS_USE_PGPSSL

	std::string gpgPasswd = ui.loadGPGPasswd->text().toStdString();
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
		RsInit::LoadGPGPassword(gpgPasswd);
	}
#else
#endif
	RsInit::LoadPassword(accountId, passwd);
	loadCertificates();
}

void StartDialog::loadCertificates()
{
	bool autoSave = (Qt::Checked == ui.autoBox -> checkState());
	/* Final stage of loading */
	if (RsInit::LoadCertificates(autoSave))
	{
		close();
	}
	else
	{
		/* some error msg */
                QMessageBox::StandardButton sb = QMessageBox::warning ( NULL,
                                "Login Failure",
                                "*** Wrong Password ***",
				QMessageBox::Ok);
	        ui.loadPasswd->setText("");
	}

}


void StartDialog::createnewaccount()
{
    //static GenCertDialog *gencertdialog = new GenCertDialog();
    //gencertdialog->show();
    
    QMessageBox::StandardButton sb = QMessageBox::question ( NULL,
                        "Create a New Profil", 
			"This will generate a new Profile\n Are you sure you want to continue",
			(QMessageBox::Ok | QMessageBox::No));

    if (sb == QMessageBox::Ok)
    {
    	reqNewCert = true;
    	close();
    }
}


void StartDialog::infodlg()
{
    static InfoDialog *infodialog = new InfoDialog();
    infodialog->show();
}



bool  StartDialog::requestedNewCert()
{
	return reqNewCert;
}


LogoBar & StartDialog::getLogoBar() const {
	return *_rsLogoBar;
}

