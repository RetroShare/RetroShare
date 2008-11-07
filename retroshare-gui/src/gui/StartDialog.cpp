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
#include "StartDialog.h"
#include "GenCertDialog.h"
#include "LogoBar.h"
#include <QFileDialog>
#include <QMessageBox>
#include "util/Widget.h"

/* Define the format used for displaying the date and time */
#define DATETIME_FMT  "MMM dd hh:mm:ss"



/** Default constructor */
StartDialog::StartDialog(RsInit *conf, QWidget *parent, Qt::WFlags flags)
  : QMainWindow(parent, flags), rsConfig(conf), reqNewCert(false)
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
  

  
  //connect(ui.genButton, SIGNAL(clicked()), this, SLOT(genPerson()));
  connect(ui.loadButton, SIGNAL(clicked()), this, SLOT(loadPerson()));
  connect(ui.loadPasswd, SIGNAL(returnPressed()), this, SLOT(loadPerson()));
  //connect(ui.selectButton, SIGNAL(clicked()), this, SLOT(selectFriend()));
  //connect(ui.friendBox, SIGNAL(stateChanged(int)), this, SLOT(checkChanged(int)));
  connect(ui.createaccountButton, SIGNAL(clicked()), this, SLOT(createnewaccount()));

  /* load the Certificate File name */
  std::string userName;

  if (ValidateCertificate(rsConfig, userName))
  {
  	/* just need to enter password */
	ui.loadName->setText(QString::fromStdString(userName));
	ui.loadPasswd->setFocus(Qt::OtherFocusReason);
	ui.loadButton -> setEnabled(true);
  }
  else
  {
  	/* need to generate new user */
	ui.loadName->setText("<No Existing User>");
	ui.loadButton -> setEnabled(false);
	//ui.genName->setFocus(Qt::OtherFocusReason);
  }

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
	std::string passwd = ui.loadPasswd->text().toStdString();
	LoadPassword(rsConfig, passwd);
	loadCertificates();
}

void StartDialog::loadCertificates()
{
	bool autoSave = (Qt::Checked == ui.autoBox -> checkState());
	/* Final stage of loading */
	if (LoadCertificates(rsConfig, autoSave))
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
                        "Create New Certificate", 
			"This will delete your existing Certificate\n Are you sure you want to continue",
			(QMessageBox::Ok | QMessageBox::No));

    if (sb == QMessageBox::Ok)
    {
    	reqNewCert = true;
    	close();
    }
}

bool  StartDialog::requestedNewCert()
{
	return reqNewCert;
}


LogoBar & StartDialog::getLogoBar() const {
	return *_rsLogoBar;
}

