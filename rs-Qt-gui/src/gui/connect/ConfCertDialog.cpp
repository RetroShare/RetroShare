/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,  crypton
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
#include "ConfCertDialog.h"

#include "rsiface/rsiface.h"

/* Define the format used for displaying the date and time */
#define DATETIME_FMT  "MMM dd hh:mm:ss"

/** Default constructor */
ConfCertDialog::ConfCertDialog(QWidget *parent, Qt::WFlags flags)
  : QMainWindow(parent, flags)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);


  connect(ui.applyButton, SIGNAL(clicked()), this, SLOT(applyDialog()));
  connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(closeinfodlg()));

 
  setFixedSize(QSize(434, 462));
}



/** 
 Overloads the default show() slot so we can set opacity*/

void
ConfCertDialog::show()
{
  //loadSettings();
  if(!this->isVisible()) {
    QMainWindow::show();

  }
}

void ConfCertDialog::closeEvent (QCloseEvent * event)
{
 QWidget::closeEvent(event);
}

void ConfCertDialog::closeinfodlg()
{
	close();
}

void ConfCertDialog::loadId(std::string id)
{
	mId = id;
	loadDialog();
}


void ConfCertDialog::loadDialog()
{
	/* open up the rsiface and get the data */
        /* get the shared directories */
	rsiface->lockData(); /* Lock Interface */

	/* get the correct friend */
	const NeighbourInfo *ni = rsiface->getFriend(mId);
	if (!ni)
	{
		/* fail */
		rsiface->unlockData(); /* UnLock Interface */
		return;
	}
	
	ui.name->setText(QString::fromStdString(ni -> name));
	ui.orgloc->setText(QString::fromStdString(ni -> org + "/" + ni -> loc));
	ui.country->setText(QString::fromStdString(ni -> country + "/" + ni -> state));
	
	/* set local address */
	ui.localAddress->setText(QString::fromStdString(ni->localAddr));
	ui.localPort -> setValue(ni->localPort);
	/* set the server address */
	ui.extAddress->setText(QString::fromStdString(ni->extAddr));
	ui.extPort -> setValue(ni->extPort);
	/* set the url for DNS access */
	ui.extName->setText(QString::fromStdString(ni->extName));
	ui.chkFirewall  ->setChecked(ni->firewalled);
	ui.chkForwarded ->setChecked(ni->forwardPort);
	
	ui.indivRate->setValue(ni->maxRate);

	ui.trustLvl->setText(QString::fromStdString(ni->trustString));

	if (ni->ownsign)
	{
		ui.signBox -> setCheckState(Qt::Checked);
		ui.signBox -> setEnabled(false);
		if (ni->trustLvl == 5) /* 5 = Trusted, 6 = OwnSign */
		{
			ui.trustBox -> setCheckState(Qt::Checked);
		}
		else
		{
			ui.trustBox -> setCheckState(Qt::Unchecked);
		}
		ui.trustBox -> setCheckable(true);
	}
	else
	{
		ui.signBox -> setCheckState(Qt::Unchecked);
		ui.signBox -> setEnabled(true);

		ui.trustBox -> setCheckState(Qt::Unchecked);
		ui.trustBox -> setEnabled(false);
	}
		
	rsiface->unlockData(); /* UnLock Interface */
	

}


void ConfCertDialog::applyDialog()
{
	/* open up the rsiface and get the data */
        /* get the shared directories */
	rsiface->lockData(); /* Lock Interface */

	/* get the correct friend */
	const NeighbourInfo *ni = rsiface->getFriend(mId);
	if (!ni)
	{
		/* fail */
		rsiface->unlockData(); /* UnLock Interface */
		return;
	}
	

	/* check if the data is the same */
	bool localChanged = false;
	bool extChanged = false;
	bool fwChanged = false;
	bool dnsChanged = false;
	bool signChanged = false;
	bool trustChanged = false;

	/* set local address */
	if ((ni->localAddr != ui.localAddress->text().toStdString())
		|| (ni->localPort != ui.localPort -> value()))
	{
		/* changed ... set it */
		localChanged = true;
	}

	if ((ni->extAddr != ui.extAddress->text().toStdString())
		|| (ni->extPort != ui.extPort -> value()))
	{
		/* changed ... set it */
		extChanged = true;
	}

	if (ni->extName != ui.extName->text().toStdString())
	{
		/* changed ... set it */
		dnsChanged = true;
	}

	if ((ni->firewalled != ui.chkFirewall  ->isChecked()) ||
		(ni->forwardPort != ui.chkForwarded ->isChecked()))
	{
		/* changed ... set it */
		fwChanged = true;
	}
	
	if (ni -> maxRate != ui.indivRate->value())
	{
		/* nada */
	}

	if (ni->ownsign)
	{
		/* check the trust tick */
		bool trsted = (ni->trustLvl == 5); /* 5 = Trusted, 6 = OwnSign */
		
		if (trsted != ui.trustBox->isChecked())
		{
			trustChanged = true;
		}
	}
	else
	{
		if (ui.signBox->isChecked())
		{
			signChanged = true;
		}
	}
		
	rsiface->unlockData(); /* UnLock Interface */


	/* now we can action the changes */
	if (localChanged)
		rsicontrol -> FriendSetLocalAddress(mId, 
			ui.localAddress->text().toStdString(), ui.localPort->value());

	if (extChanged)
		rsicontrol -> FriendSetExtAddress(mId, 
			ui.extAddress->text().toStdString(), ui.extPort->value());

	if (dnsChanged)
		rsicontrol -> FriendSetDNSAddress(mId, ui.extName->text().toStdString());

	if (fwChanged)
		rsicontrol -> FriendSetFirewall(mId, ui.chkFirewall->isChecked(),
						ui.chkForwarded->isChecked());

	if (trustChanged)
		rsicontrol -> FriendTrustSignature(mId, ui.trustBox->isChecked());

	if (signChanged)
		rsicontrol -> FriendSignCert(mId);

	/* reload now */
	loadDialog();
	
	/* close the Dialog after the Changes applied */
	closeinfodlg();
}


void ConfCertDialog::setInfo(std::string name, 
				std::string trust,
				std::string org,
				std::string loc,
				std::string country,
				std::string signers)
{
	ui.name->setText(QString::fromStdString(name));
	ui.trustLvl->setText(QString::fromStdString(trust));
	ui.orgloc->setText(QString::fromStdString(org + loc));
	//ui.loc->setText(QString::fromStdString(loc));
	//ui.country->setText(QString::fromStdString(country));
	//ui.signers->setText(QString::fromStdString(signers));
}

		     
