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
#include "rsiface/rsdisc.h"

#include <QTime>
#include <QtGui>

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
  connect(ui.sign_button, SIGNAL(clicked()), this, SLOT(makeFriend()));


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
        isPGPId = false;
	RsPeerDetails detail;
	if (!rsPeers->getPeerDetails(mId, detail))
	{
            isPGPId = true;
            if (!rsPeers->getPGPDetails(mId, detail)) {
                QMessageBox::information(this,
                         tr("RetroShare"),
                         tr("Error : cannot get peer details."));
                this->close();
            }
        }

	ui.name->setText(QString::fromStdString(detail.name));
        ui.peerid->setText(QString::fromStdString(detail.id));
        if (!isPGPId) {
            ui.orgloc->setText(QString::fromStdString(detail.org));
            ui.country->setText(QString::fromStdString(detail.location));
            // Dont Show a timestamp in RS calculate the day
            QDateTime date = QDateTime::fromTime_t(detail.lastConnect);
            QString stime = date.toString(Qt::LocalDate);
            ui.lastcontact-> setText(stime);

            /* set retroshare version */
            std::map<std::string, std::string>::iterator vit;
            std::map<std::string, std::string> versions;
            bool retv = rsDisc->getDiscVersions(versions);
            if (retv && versions.end() != (vit = versions.find(detail.id)))
            {
                    ui.version->setText(QString::fromStdString(vit->second));
            }

            /* set local address */
            ui.localAddress->setText(QString::fromStdString(detail.localAddr));
            ui.localPort -> setValue(detail.localPort);
            /* set the server address */
            ui.extAddress->setText(QString::fromStdString(detail.extAddr));
            ui.extPort -> setValue(detail.extPort);

            ui.ipAddressList->clear();
            for(std::list<std::string>::const_iterator it(detail.ipAddressList.begin());it!=detail.ipAddressList.end();++it)
                   ui.ipAddressList->addItem(QString::fromStdString(*it));
        } else {
            ui.orgloc->hide();
            ui.label_11->hide();
            ui.country->hide();
            ui.label_8->hide();
            ui.lastcontact->hide();
            ui.label_7->hide();
            ui.version->hide();
            ui.label_3->hide();

            ui.groupBox->hide();
        }

	/* set the url for DNS access (OLD) */
	//ui.extName->setText(QString::fromStdString(""));

	/**** TODO ****/
	//ui.chkFirewall  ->setChecked(ni->firewalled);
	//ui.chkForwarded ->setChecked(ni->forwardPort);
	//ui.chkFirewall  ->setChecked(0);
	//ui.chkForwarded ->setChecked(0);

	//ui.indivRate->setValue(0);

	//ui.trustLvl->setText(QString::fromStdString(RsPeerTrustString(detail.trustLvl)));

        if (detail.ownsign) {
            ui.sign_button->hide();
            ui.signed_already_label->show();
        } else {
             ui.sign_button->show();
            ui.signed_already_label->hide();
        }

        bool hasSignedMe = false;
        RsPeerDetails ownGPGDetails ;
        rsPeers->getPGPDetails(rsPeers->getPGPOwnId(), ownGPGDetails);
        std::list<std::string>::iterator signersIt;
        for(signersIt = ownGPGDetails.gpgSigners.begin(); signersIt != ownGPGDetails.gpgSigners.end() ; ++signersIt) {
            if (*signersIt == detail.id) {
                hasSignedMe = true;
                break;
            }
        }
        if (hasSignedMe) {
                ui.is_signing_me->setText(tr("Peer has acepted me as a friend and did not signed my GPG key"));
        } else {
                ui.is_signing_me->setText(tr("Peer has not acepted me as a friend and did not signed my GPG key"));
       }

        ui.signers->clear() ;
        for(std::list<std::string>::const_iterator it(detail.gpgSigners.begin());it!=detail.gpgSigners.end();++it) {	
            RsPeerDetails signerDetail;
            if (rsPeers->getPGPDetails(*it, signerDetail)) {
                ui.signers->append(QString::fromStdString(signerDetail.name));
            }
        }
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

	/* set local address */
	if ((detail.localAddr != ui.localAddress->text().toStdString()) || (detail.localPort != ui.localPort -> value()))
		localChanged = true;

	if ((detail.extAddr != ui.extAddress->text().toStdString()) || (detail.extPort != ui.extPort -> value()))
		extChanged = true;

	/* now we can action the changes */
	if (localChanged)
		rsPeers->setLocalAddress(mId, ui.localAddress->text().toStdString(), ui.localPort->value());

	if (extChanged)
		rsPeers->setExtAddress(mId,ui.extAddress->text().toStdString(), ui.extPort->value());

	/* reload now */
	loadDialog();

	/* close the Dialog after the Changes applied */
	closeinfodlg();

        if(localChanged || extChanged)
		emit configChanged() ;
}

void ConfCertDialog::makeFriend()
{
        rsPeers->SignGPGCertificate(mId);
        loadDialog();
}
