/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2012,  RetroShare Team
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
#include <gpgme.h>

#include "FriendRequest.h"

#include <QMessageBox>
#include <QDateTime>
#include <QMenu>
#include <QClipboard>
#include <QMap>

#include <iostream>

#include <retroshare/rspeers.h>
#include <retroshare/rsdisc.h>
#include <retroshare/rsmsgs.h>

#include "gui/help/browser/helpbrowser.h"
#include "gui/common/PeerDefs.h"
#include "gui/common/StatusDefs.h"
#include "gui/RetroShareLink.h"
#include "gui/notifyqt.h"
#include "gui/common/AvatarDefs.h"



/* Define the format used for displaying the date and time */
#define DATETIME_FMT  "MMM dd hh:mm:ss"

/** Default constructor */
FriendRequest::FriendRequest(const std::string& id, QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags), mId(id)
{
    /* Invoke Qt Designer generated QObject setup routine */
    ui.setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose, true);

    connect(ui.applyButton, SIGNAL(clicked()), this, SLOT(applyDialog()));
    connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui.signKeyButton, SIGNAL(clicked()), this, SLOT(signGPGKey()));

    load();
}

/** Destructor. */
FriendRequest::~FriendRequest()
{
}

void FriendRequest::load()
{
    RsPeerDetails detail;
    if (!rsPeers->getPeerDetails(mId, detail))
    {
        QMessageBox::information(this,
                                 tr("RetroShare"),
                                 tr("Error : cannot get peer details."));
        close();
        return;
    }

    ui.name->setText(QString::fromUtf8(detail.name.c_str()));
    ui.peerid->setText(QString::fromStdString(detail.id));

    RetroShareLink link;
    link.createPerson(detail.id);

    ui.rsid->setText(link.toHtml());
    ui.rsid->setToolTip(link.title());

    ui.avatar->setId(mId, false);


        if (detail.accept_connection) {
            ui.signGPGKeyCheckBox->hide();
            //connection already accepted, propose to sign gpg key
            if (!detail.ownsign) {
                ui.signKeyButton->show();
            } else {
                ui.signKeyButton->hide();
            }
        } else {
            ui.signKeyButton->hide();
            if (!detail.ownsign) {
                ui.signGPGKeyCheckBox->show();
                ui.signGPGKeyCheckBox->setChecked(false);
            } else {
                ui.signGPGKeyCheckBox->hide();
            }
        }

        //web of trust
        if (detail.trustLvl == GPGME_VALIDITY_ULTIMATE) {
            //trust is ultimate, it means it's one of our own keys
            ui.web_of_trust_label->setText(tr("Your trust in this peer is ultimate, it's probably a key you own."));
            ui.radioButton_trust_fully->hide();
            ui.radioButton_trust_marginnaly->hide();
            ui.radioButton_trust_never->hide();
        } else {
            ui.radioButton_trust_fully->show();
            ui.radioButton_trust_marginnaly->show();
            ui.radioButton_trust_never->show();
            if (detail.trustLvl == GPGME_VALIDITY_FULL) {
                ui.web_of_trust_label->setText(tr("Your trust in this peer is full."));
                ui.radioButton_trust_fully->setChecked(true);
                ui.radioButton_trust_fully->setIcon(QIcon(":/images/security-high-48.png"));
                ui.radioButton_trust_marginnaly->setIcon(QIcon(":/images/security-medium-off-48.png"));
                ui.radioButton_trust_never->setIcon(QIcon(":/images/security-low-off-48.png"));
            } else if (detail.trustLvl == GPGME_VALIDITY_MARGINAL) {
                ui.web_of_trust_label->setText(tr("Your trust in this peer is marginal."));
                ui.radioButton_trust_marginnaly->setChecked(true);
                ui.radioButton_trust_marginnaly->setIcon(QIcon(":/images/security-medium-48.png"));
                ui.radioButton_trust_never->setIcon(QIcon(":/images/security-low-off-48.png"));
                ui.radioButton_trust_fully->setIcon(QIcon(":/images/security-high-off-48.png"));
            } else if (detail.trustLvl == GPGME_VALIDITY_NEVER) {
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
            //ui.is_signing_me->setText(tr("Peer has authenticated me as a friend and did sign my GPG key"));
        } else {
            //ui.is_signing_me->setText(tr("Peer has not authenticated me as a friend and did not sign my GPG key"));
        }
}

void FriendRequest::applyDialog()
{
    std::cerr << "FriendRequest::applyDialog() called" << std::endl ;
    RsPeerDetails detail;
    if (!rsPeers->getPeerDetails(mId, detail))
    {
        if (!rsPeers->getGPGDetails(mId, detail)) {
            QMessageBox::information(this,
                                     tr("RetroShare"),
                                     tr("Error : cannot get peer details."));
            close();
            return;
        }
    }

    //check the GPG trustlvl
    if (ui.radioButton_trust_fully->isChecked() && detail.trustLvl != GPGME_VALIDITY_FULL) {
        //trust has changed to fully
        rsPeers->trustGPGCertificate(detail.id, GPGME_VALIDITY_FULL);
    } else if (ui.radioButton_trust_marginnaly->isChecked() && detail.trustLvl != GPGME_VALIDITY_MARGINAL) {
        rsPeers->trustGPGCertificate(detail.id, GPGME_VALIDITY_MARGINAL);
    } else if (ui.radioButton_trust_never->isChecked() && detail.trustLvl != GPGME_VALIDITY_NEVER) {
        rsPeers->trustGPGCertificate(detail.id, GPGME_VALIDITY_NEVER);
    }


    makeFriend();
    close();
}

void FriendRequest::makeFriend()
{
    std::string gpg_id = rsPeers->getGPGId(mId);
    if (ui.signGPGKeyCheckBox->isChecked()) {
        rsPeers->signGPGCertificate(gpg_id);
    } 
	
    rsPeers->addFriend(mId, gpg_id);

    emit configChanged();
}

void FriendRequest::denyFriend()
{
    std::string gpg_id = rsPeers->getGPGId(mId);
    rsPeers->removeFriend(gpg_id);

    emit configChanged();
}

void FriendRequest::signGPGKey()
{
    std::string gpg_id = rsPeers->getGPGId(mId);
    if (!rsPeers->signGPGCertificate(gpg_id)) {
                 QMessageBox::warning ( NULL,
                                tr("Signature Failure"),
                                tr("Maybe password is wrong"),
                                QMessageBox::Ok);
    }

    emit configChanged();
}

