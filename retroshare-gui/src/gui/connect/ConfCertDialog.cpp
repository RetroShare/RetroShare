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

#include <QMessageBox>
#include <QDateTime>
#include <QMenu>
#include <QClipboard>

#include <iostream>

#include <retroshare/rspeers.h>
#include <retroshare/rsdisc.h>

#include "gui/help/browser/helpbrowser.h"

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
  connect(ui.make_friend_button, SIGNAL(clicked()), this, SLOT(makeFriend()));
  connect(ui.denyFriendButton, SIGNAL(clicked()), this, SLOT(denyFriend()));
  connect(ui.signKeyButton, SIGNAL(clicked()), this, SLOT(signGPGKey()));
  connect(ui.trusthelpButton, SIGNAL(clicked()), this, SLOT(showHelpDialog()));
  connect(ui.signers_listWidget, SIGNAL(customContextMenuRequested( QPoint ) ), this, SLOT( listWidgetContextMenuPopup( QPoint ) ) );

  ui.applyButton->setToolTip(tr("Apply and Close"));
}

void ConfCertDialog::show(const std::string& peer_id)
{
	/* set the Id */

	instance()->loadId(peer_id);
        instance()->show();
}

void ConfCertDialog::showTrust(const std::string& peer_id)
{
        /* set the Id */

        instance()->loadId(peer_id);
        instance()->showTrust();
}


/**
 Overloads the default show() slot so we can set opacity*/

void
ConfCertDialog::show()
{
  //loadSettings();
  ui.stabWidget->setCurrentIndex(0);
  if(!this->isVisible()) {
    QDialog::show();

  }
}

void
ConfCertDialog::showTrust()
{
  //loadSettings();
  ui.stabWidget->setCurrentIndex(1);
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
            QMessageBox::information(this,
                     tr("RetroShare"),
                     tr("Error : cannot get peer details."));
            closeinfodlg();
        }

	ui.name->setText(QString::fromStdString(detail.name));
    ui.peerid->setText(QString::fromStdString(detail.id));
    ui.rsid->setText(QString::fromStdString(detail.name) + "@" + QString::fromStdString(detail.id));

        if (!detail.isOnlyGPGdetail) {
            
            ui.loc->setText(QString::fromStdString(detail.location));
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

            ui.dynDNS->setText(QString::fromStdString(detail.dyndns));

            ui.ipAddressList->clear();
            for(std::list<std::string>::const_iterator it(detail.ipAddressList.begin());it!=detail.ipAddressList.end();++it)
                   ui.ipAddressList->addItem(QString::fromStdString(*it));

            ui.loc->show();
            ui.label_loc->show();
            ui.lastcontact->show();
            ui.label_last_contact->show();
            ui.version->show();
            ui.label_version->show();

            ui.groupBox->show();
            ui.rsid->hide();
            ui.label_rsid->hide();
        } else {
            ui.rsid->show();
            ui.label_rsid->show();
            ui.loc->hide();
            ui.label_loc->hide();
            ui.lastcontact->hide();
            ui.label_last_contact->hide();
            ui.version->hide();
            ui.label_version->hide();

            ui.groupBox->hide();
        }

        if (detail.gpg_id == rsPeers->getGPGOwnId()) {
            ui.make_friend_button->hide();
            ui.signGPGKeyCheckBox->hide();
            ui.signKeyButton->hide();
            ui.denyFriendButton->hide();

            ui.web_of_trust_label->hide();
            ui.radioButton_trust_fully->hide();
            ui.radioButton_trust_marginnaly->hide();
            ui.radioButton_trust_never->hide();

            ui.is_signing_me->hide();
            ui.signersBox->setTitle(tr("Your key is signed by : "));

        } else {
            ui.make_friend_button->show();
            ui.signGPGKeyCheckBox->show();
            ui.signKeyButton->show();
            ui.denyFriendButton->show();

            ui.web_of_trust_label->show();
            ui.radioButton_trust_fully->show();
            ui.radioButton_trust_marginnaly->show();
            ui.radioButton_trust_never->show();

            ui.is_signing_me->show();
            ui.signersBox->setTitle(tr("Peer key is signed by : "));

            if (detail.accept_connection) {
                ui.make_friend_button->hide();
                ui.denyFriendButton->show();
                ui.signGPGKeyCheckBox->hide();
                //connection already accepted, propose to sign gpg key
                if (!detail.ownsign) {
                    ui.signGPGKeyCheckBox->hide();
                    ui.signKeyButton->show();
                } else {
                    ui.signGPGKeyCheckBox->hide();
                    ui.signKeyButton->hide();
                }
            } else {
                ui.make_friend_button->show();
                ui.denyFriendButton->hide();
                ui.signKeyButton->hide();
                if (!detail.ownsign) {
                    ui.signGPGKeyCheckBox->show();
                    ui.signGPGKeyCheckBox->setChecked(true);
                } else {
                    ui.signGPGKeyCheckBox->hide();
                }
            }

           //web of trust
           if (detail.trustLvl == 5) {
               //trust is ultimate, it means it's one of our own keys
               ui.web_of_trust_label->setText(tr("Your trust in this peer is ultimate, it's probably a key you own."));
               ui.radioButton_trust_fully->hide();
               ui.radioButton_trust_marginnaly->hide();
               ui.radioButton_trust_never->hide();
           } else {
                ui.radioButton_trust_fully->show();
                ui.radioButton_trust_marginnaly->show();
                ui.radioButton_trust_never->show();
                if (detail.trustLvl == 4) {
                    ui.web_of_trust_label->setText(tr("Your trust in this peer is full."));
                    ui.radioButton_trust_fully->setChecked(true);
                    ui.radioButton_trust_fully->setIcon(QIcon(":/images/security-high-48.png"));
                    ui.radioButton_trust_marginnaly->setIcon(QIcon(":/images/security-medium-off-48.png"));
                    ui.radioButton_trust_never->setIcon(QIcon(":/images/security-low-off-48.png"));
                } else if (detail.trustLvl == 3) {
                    ui.web_of_trust_label->setText(tr("Your trust in this peer is marginal."));
                    ui.radioButton_trust_marginnaly->setChecked(true);
                    ui.radioButton_trust_marginnaly->setIcon(QIcon(":/images/security-medium-48.png"));
                    ui.radioButton_trust_never->setIcon(QIcon(":/images/security-low-off-48.png"));
                    ui.radioButton_trust_fully->setIcon(QIcon(":/images/security-high-off-48.png"));
                } else if (detail.trustLvl == 2) {
                    ui.web_of_trust_label->setText(tr("Your trust in this peer is none."));
                    ui.radioButton_trust_never->setChecked(true);
                    ui.radioButton_trust_never->setIcon(QIcon(":/images/security-low-48.png"));
                    ui.radioButton_trust_fully->setIcon(QIcon(":/images/security-high-off-48.png"));
                    ui.radioButton_trust_marginnaly->setIcon(QIcon(":/images/security-medium-off-48.png"));
                } else {
                    ui.web_of_trust_label->setText(tr("Your trust in this peer is not set."));

                    //we have to set up the set exclusive to false in order to uncheck it all
                    ui.radioButton_trust_fully->setAutoExclusive(false);
                    ui.radioButton_trust_marginnaly->setAutoExclusive(false);
                    ui.radioButton_trust_never->setAutoExclusive(false);

                    ui.radioButton_trust_fully->setChecked(false);
                    ui.radioButton_trust_marginnaly->setChecked(false);
                    ui.radioButton_trust_never->setChecked(false);

                    ui.radioButton_trust_fully->setAutoExclusive(true);
                    ui.radioButton_trust_marginnaly->setAutoExclusive(true);
                    ui.radioButton_trust_never->setAutoExclusive(true);

                    ui.radioButton_trust_never->setIcon(QIcon(":/images/security-low-off-48.png"));
                    ui.radioButton_trust_fully->setIcon(QIcon(":/images/security-high-off-48.png"));
                    ui.radioButton_trust_marginnaly->setIcon(QIcon(":/images/security-medium-off-48.png"));
               }
           }

            if (detail.hasSignedMe) {
                ui.is_signing_me->setText(tr("Peer has authenticated me as a friend and did sign my GPG key"));
            } else {
                ui.is_signing_me->setText(tr("Peer has not authenticated me as a friend and did not sign my GPG key"));
            }
       }

        ui.signers_listWidget->clear() ;
        for(std::list<std::string>::const_iterator it(detail.gpgSigners.begin());it!=detail.gpgSigners.end();++it) {	
            RsPeerDetails signerDetail;
            if (rsPeers->getGPGDetails(*it, signerDetail)) {
                ui.signers_listWidget->addItem(QString::fromStdString(signerDetail.name) + " (" + QString::fromStdString(signerDetail.id) +")");
            }
        }

			  std::string invite = rsPeers->GetRetroshareInvite(detail.id) ; // this needs to be a SSL id

			  ui.userCertificateText->setReadOnly(true);
			  ui.userCertificateText->setMinimumHeight(200);
			  ui.userCertificateText->setMinimumWidth(530);
			  QFont font("Courier New",10,50,false);
			  font.setStyleHint(QFont::TypeWriter,QFont::PreferMatch);
			  font.setStyle(QFont::StyleNormal);
			  ui.userCertificateText->setFont(font);
			  ui.userCertificateText->setText(QString::fromStdString(invite));
}


void ConfCertDialog::applyDialog()
{
        std::cerr << "ConfCertDialog::applyDialog() called" << std::endl ;
        RsPeerDetails detail;
	if (!rsPeers->getPeerDetails(mId, detail))
	{
            if (!rsPeers->getGPGDetails(mId, detail)) {
                QMessageBox::information(this,
                         tr("RetroShare"),
                         tr("Error : cannot get peer details."));
                closeinfodlg();
            }
        }

        //check the GPG trustlvl
        if (ui.radioButton_trust_fully->isChecked() && detail.trustLvl != 4) {
            //trust has changed to fully
            rsPeers->trustGPGCertificate(detail.id, 4);
        } else if (ui.radioButton_trust_marginnaly->isChecked() && detail.trustLvl != 3) {
            rsPeers->trustGPGCertificate(detail.id, 3);
        } else if (ui.radioButton_trust_never->isChecked() && detail.trustLvl != 2) {
            rsPeers->trustGPGCertificate(detail.id, 2);
        }

        if (!detail.isOnlyGPGdetail) {
            /* check if the data is the same */
            bool localChanged = false;
            bool extChanged = false;
            bool dnsChanged = false;

            /* set local address */
            if ((detail.localAddr != ui.localAddress->text().toStdString()) || (detail.localPort != ui.localPort -> value()))
                    localChanged = true;

            if ((detail.extAddr != ui.extAddress->text().toStdString()) || (detail.extPort != ui.extPort -> value()))
                    extChanged = true;

            if ((detail.dyndns != ui.dynDNS->text().toStdString()))
                    dnsChanged = true;

            /* now we can action the changes */
            if (localChanged)
                    rsPeers->setLocalAddress(mId, ui.localAddress->text().toStdString(), ui.localPort->value());

            if (extChanged)
                    rsPeers->setExtAddress(mId,ui.extAddress->text().toStdString(), ui.extPort->value());

            if (dnsChanged)
                    rsPeers->setDynDNS(mId, ui.dynDNS->text().toStdString());

            if(localChanged || extChanged || dnsChanged)
                    emit configChanged() ;
        }

        closeinfodlg();
}

void ConfCertDialog::makeFriend() {
    std::string gpg_id = rsPeers->getGPGId(mId);
    if (ui.signGPGKeyCheckBox->isChecked()) {
        rsPeers->signGPGCertificate(gpg_id);
    } else {
        rsPeers->setAcceptToConnectGPGCertificate(gpg_id, true);
    }
    rsPeers->addFriend(mId, gpg_id);
    loadDialog();
}

void ConfCertDialog::denyFriend() {
    std::string gpg_id = rsPeers->getGPGId(mId);
    rsPeers->setAcceptToConnectGPGCertificate(gpg_id, false);
    loadDialog();
}

void ConfCertDialog::signGPGKey() {
    std::string gpg_id = rsPeers->getGPGId(mId);
    if (!rsPeers->signGPGCertificate(gpg_id)) {
                 QMessageBox::StandardButton sb = QMessageBox::warning ( NULL,
                                tr("Signature Failure"),
                                tr("Maybe password is wrong"),
                                QMessageBox::Ok);
    }
    loadDialog();
}

/** Displays the help browser and displays the most recently viewed help
 * topic. */
void ConfCertDialog::showHelpDialog()
{
    showHelpDialog(QString("trust"));
}

/**< Shows the help browser and displays the given help <b>topic</b>. */
void ConfCertDialog::showHelpDialog(const QString &topic)
{
    static HelpBrowser *helpBrowser = 0;
    if (!helpBrowser)
    helpBrowser = new HelpBrowser(this);
    helpBrowser->showWindow(topic);
}

void ConfCertDialog::listWidgetContextMenuPopup( const QPoint &pos)
{
    QListWidgetItem *CurrentItem = ui.signers_listWidget->currentItem();
    if ( ! CurrentItem )
        return; 

    QMenu menu( this );
    QAction *copyPeer = new QAction(tr("Copy Peer"), this );
    connect( copyPeer , SIGNAL( triggered() ), this, SLOT( copyToClipboard() ) );
    menu.addAction(copyPeer );
    menu.exec(QCursor::pos());
 
}

void ConfCertDialog::copyToClipboard( )
{
    QListWidgetItem *CurrentItem = ui.signers_listWidget->currentItem();
    if ( ! CurrentItem )
        return; 

    QClipboard * cb = QApplication::clipboard();
    QString text = CurrentItem->text();
    cb->setText( text, QClipboard::Clipboard );
}
