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

ConfCertDialog *ConfCertDialog::instance()
{
	static ConfCertDialog *confdialog = new ConfCertDialog ;

	return confdialog ;
}

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
  connect(ui._makeFriendPB, SIGNAL(clicked()), this, SLOT(makeFriend()));

 
  ui.applyButton->setToolTip(tr("Apply and Close"));
}

void ConfCertDialog::show(const std::string& peer_id)
{
	/* set the Id */

	instance()->loadId(peer_id);
	instance()->show();
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
	//ui.extName->setText(QString::fromStdString(""));

	/**** TODO ****/
	//ui.chkFirewall  ->setChecked(ni->firewalled);
	//ui.chkForwarded ->setChecked(ni->forwardPort);
	//ui.chkFirewall  ->setChecked(0);
	//ui.chkForwarded ->setChecked(0);
	
	//ui.indivRate->setValue(0);

	//ui.trustLvl->setText(QString::fromStdString(RsPeerTrustString(detail.trustLvl)));

	ui._peerTrustsMeCB->setChecked(rsPeers->isOnline(detail.id) || rsPeers->isTrustingMe(detail.id)) ;
	ui._peerTrustsMeCB->setEnabled(false);
	ui.signBox->setChecked(detail.ownsign) ;
	ui.signBox->setEnabled(!detail.ownsign) ;

	ui._peerAcceptedCB->setChecked(detail.state & RS_PEER_STATE_FRIEND) ;
	ui._peerAcceptedCB->setEnabled(detail.ownsign) ;

	ui.signers->clear() ;
	for(std::list<std::string>::const_iterator it(detail.signers.begin());it!=detail.signers.end();++it)
		ui.signers->append(QString::fromStdString(*it)) ;
}


void ConfCertDialog::applyDialog()
{
	std::cerr << "In apply dialog" << std::endl ;
	RsPeerDetails detail;
	if (!rsPeers->getPeerDetails(mId, detail))
	{
		std::cerr << "Could not get details from " << mId << std::endl ;
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
	if ((detail.localAddr != ui.localAddress->text().toStdString()) || (detail.localPort != ui.localPort -> value()))
		localChanged = true;

	if ((detail.extAddr != ui.extAddress->text().toStdString()) || (detail.extPort != ui.extPort -> value()))
		extChanged = true;

	if (detail.ownsign)
	{
		if (ui._peerAcceptedCB->isChecked() != ((detail.state & RS_PEER_STATE_FRIEND) > 0)) 
			trustChanged = true;
	}
	else if (ui.signBox->isChecked())
		signChanged = true;

	/* now we can action the changes */
	if (localChanged)
		rsPeers->setLocalAddress(mId, ui.localAddress->text().toStdString(), ui.localPort->value());

	if (extChanged)
		rsPeers->setExtAddress(mId,ui.extAddress->text().toStdString(), ui.extPort->value());

#if 0
	if (fwChanged)
		rsicontrol -> FriendSetFirewall(mId, ui.chkFirewall->isChecked(),
						ui.chkForwarded->isChecked());
#endif

	if (signChanged)
	{
		std::cerr << "Signature changed. Signing certificate" << mId << std::endl ;
		rsPeers->SignCertificate(mId);
	}

	if (trustChanged)
	{
		std::cerr << "Acceptance changed. Authing ceAuthrtificate" << mId << std::endl ;
		if(ui._peerAcceptedCB->isChecked())
			rsPeers->AuthCertificate(mId, "");
		else
			rsPeers->removeFriend(mId);
	}

	/* reload now */
	loadDialog();
	
	/* close the Dialog after the Changes applied */
	closeinfodlg();

	if(trustChanged || signChanged)
		emit configChanged() ;
}

void ConfCertDialog::makeFriend()
{
	ui.signBox->setChecked(true) ;
	ui._peerAcceptedCB->setChecked(true) ;

//	rsPeers->TrustCertificate(mId, ui.trustBox->isChecked());
//	rsPeers->SignCertificate(mId);
}

#if 0
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
#endif

		     
