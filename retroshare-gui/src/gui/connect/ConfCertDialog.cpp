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
#include "rsiface/rspeers.h"

#include <QTime>


/* Define the format used for displaying the date and time */
#define DATETIME_FMT  "MMM dd hh:mm:ss"

/** Default constructor */
ConfCertDialog::ConfCertDialog(QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);


  connect(ui.applyButton, SIGNAL(clicked()), this, SLOT(applyDialog()));
  connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(closeinfodlg()));

 
  //setFixedSize(QSize(434, 462));
}



/** 
 Overloads the default show() slot so we can set opacity*/

void
ConfCertDialog::show()
{
  //loadSettings();
  if(!this->isVisible()) {
    QDialog::show();

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

	RsPeerDetails detail;
	if (!rsPeers->getPeerDetails(mId, detail))
	{
		/* fail */
		return;
	}
	
	ui.name->setText(QString::fromStdString(detail.name));
	ui.orgloc->setText(QString::fromStdString(detail.org));
	ui.country->setText(QString::fromStdString(detail.location));
        ui.peerid->setText(QString::fromStdString(detail.id));
	// Dont Show a timestamp in RS calculate the day
	QDateTime date = QDateTime::fromTime_t(detail.lastConnect);
	QString stime = date.toString(Qt::LocalDate);
        ui.lastcontact-> setText(stime);
	
	/* set local address */
	ui.localAddress->setText(QString::fromStdString(detail.localAddr));
	ui.localPort -> setValue(detail.localPort);
	/* set the server address */
	ui.extAddress->setText(QString::fromStdString(detail.extAddr));
	ui.extPort -> setValue(detail.extPort);

	/* set the url for DNS access (OLD) */
	ui.extName->setText(QString::fromStdString(""));

	/**** TODO ****/
	//ui.chkFirewall  ->setChecked(ni->firewalled);
	//ui.chkForwarded ->setChecked(ni->forwardPort);
	//ui.chkFirewall  ->setChecked(0);
	//ui.chkForwarded ->setChecked(0);
	
	//ui.indivRate->setValue(0);

	ui.trustLvl->setText(QString::fromStdString(RsPeerTrustString(detail.trustLvl)));

	if (detail.ownsign)
	{
		ui.signBox -> setCheckState(Qt::Checked);
		ui.signBox -> setEnabled(false);
		if (detail.trusted)
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

}


void ConfCertDialog::applyDialog()
{
	RsPeerDetails detail;
	if (!rsPeers->getPeerDetails(mId, detail))
	{
		/* fail */
		return;
	}
	
	/* check if the data is the same */
	bool localChanged = false;
	bool extChanged = false;
	bool fwChanged = false;
	bool signChanged = false;
	bool trustChanged = false;

	/* set local address */
	if ((detail.localAddr != ui.localAddress->text().toStdString())
		|| (detail.localPort != ui.localPort -> value()))
	{
		/* changed ... set it */
		localChanged = true;
	}

	if ((detail.extAddr != ui.extAddress->text().toStdString())
		|| (detail.extPort != ui.extPort -> value()))
	{
		/* changed ... set it */
		extChanged = true;
	}

#if 0
	if ((detail.firewalled != ui.chkFirewall  ->isChecked()) ||
		(detail.forwardPort != ui.chkForwarded ->isChecked()))
	{
		/* changed ... set it */
		fwChanged = true;
	}
	
	if (ni -> maxRate != ui.indivRate->value())
	{
		/* nada */
	}
#endif

	if (detail.ownsign)
	{
		if (detail.trusted != ui.trustBox->isChecked())
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
		
	/* now we can action the changes */
	if (localChanged)
		rsPeers->setLocalAddress(mId, 
			ui.localAddress->text().toStdString(), ui.localPort->value());

	if (extChanged)
		rsPeers->setExtAddress(mId, 
			ui.extAddress->text().toStdString(), ui.extPort->value());

#if 0
	if (fwChanged)
		rsicontrol -> FriendSetFirewall(mId, ui.chkFirewall->isChecked(),
						ui.chkForwarded->isChecked());
#endif

	if (trustChanged)
		rsPeers->TrustCertificate(mId, ui.trustBox->isChecked());

	if (signChanged)
		rsPeers->SignCertificate(mId);

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

		     
