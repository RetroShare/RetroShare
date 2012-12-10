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
#include "gui/MainWindow.h"
#include "util/DateTime.h"

static QMap<std::string, ConfCertDialog*> instances;

ConfCertDialog *ConfCertDialog::instance(const std::string& peer_id)
{
    ConfCertDialog *d = instances[peer_id];
    if (d) {
        return d;
    }

    d = new ConfCertDialog(peer_id);
    instances[peer_id] = d;

    return d;
}

/** Default constructor */
ConfCertDialog::ConfCertDialog(const std::string& id, QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags), mId(id)
{
    /* Invoke Qt Designer generated QObject setup routine */
    ui.setupUi(this);

	 ui._useOldFormat_CB->setChecked(true) ;

	ui.headerFrame->setHeaderImage(QPixmap(":/images/user/identityinfo64.png"));
	ui.headerFrame->setHeaderText(tr("Friend Details"));

	ui._chat_CB->hide() ;

	setAttribute(Qt::WA_DeleteOnClose, true);

    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(applyDialog()));
    connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(close()));
    connect(ui.make_friend_button, SIGNAL(clicked()), this, SLOT(makeFriend()));
    connect(ui.denyFriendButton, SIGNAL(clicked()), this, SLOT(denyFriend()));
    connect(ui.signKeyButton, SIGNAL(clicked()), this, SLOT(signGPGKey()));
    connect(ui.trusthelpButton, SIGNAL(clicked()), this, SLOT(showHelpDialog()));
    connect(ui._shouldAddSignatures_CB, SIGNAL(toggled(bool)), this, SLOT(loadInvitePage()));
    connect(ui._useOldFormat_CB, SIGNAL(toggled(bool)), this, SLOT(loadInvitePage()));
    // connect(ui._anonymous_routing_CB, SIGNAL(toggled(bool)), this, SLOT(setServiceFlags()));
    // connect(ui._discovery_CB, SIGNAL(toggled(bool)), this, SLOT(setServiceFlags()));
    // connect(ui._forums_channels_CB, SIGNAL(toggled(bool)), this, SLOT(setServiceFlags()));

    ui.avatar->setFrameType(AvatarWidget::NORMAL_FRAME);

    MainWindow *w = MainWindow::getInstance();
    if (w) {
        connect(this, SIGNAL(configChanged()), w->getPage(MainWindow::Network), SLOT(insertConnect()));
    }
}

ConfCertDialog::~ConfCertDialog()
{
    QMap<std::string, ConfCertDialog*>::iterator it = instances.find(mId);
    if (it != instances.end()) {
        instances.erase(it);
    }
}

void ConfCertDialog::showIt(const std::string& peer_id, enumPage page)
{
    ConfCertDialog *confdialog = instance(peer_id);

    switch (page) {
    case PageDetails:
        confdialog->ui.stabWidget->setCurrentIndex(0);
        break;
    case PageTrust:
        confdialog->ui.stabWidget->setCurrentIndex(1);
        break;
    case PageCertificate:
        confdialog->ui.stabWidget->setCurrentIndex(2);
        break;
    }

    confdialog->load();
    confdialog->show();
    confdialog->raise();
    confdialog->activateWindow();

    /* window will destroy itself! */
}

void ConfCertDialog::setServiceFlags()
{
    RsPeerDetails detail;
    if (!rsPeers->getPeerDetails(mId, detail))
		 return ;

	 ServicePermissionFlags flags(0) ;

	 if(ui._anonymous_routing_CB->isChecked()) flags = flags | RS_SERVICE_PERM_TURTLE ;
	 if(        ui._discovery_CB->isChecked()) flags = flags | RS_SERVICE_PERM_DISCOVERY ;
	 if(  ui._forums_channels_CB->isChecked()) flags = flags | RS_SERVICE_PERM_DISTRIB ;

	 rsPeers->setServicePermissionFlags(detail.gpg_id,flags) ;
}

void ConfCertDialog::loadAll()
{
    QMap<std::string, ConfCertDialog*>::iterator it;
    for (it = instances.begin(); it != instances.end(); it++) {
        it.value()->load();
    }
}

void ConfCertDialog::load()
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

    if(detail.isOnlyGPGdetail && !rsPeers->isKeySupported(mId))
	 {
		 ui.make_friend_button->setEnabled(false) ;
		 ui.make_friend_button->setToolTip(tr("The supplied key algorithm is not supported by RetroShare\n(Only RSA keys are supported at the moment)")) ;
	 }
	 else
	 {
		 ui.make_friend_button->setEnabled(true) ;
		 ui.make_friend_button->setToolTip("") ;
	 }

	 ui._anonymous_routing_CB->setChecked(detail.service_perm_flags & RS_SERVICE_PERM_TURTLE    ) ;
	 ui._discovery_CB->setChecked(        detail.service_perm_flags & RS_SERVICE_PERM_DISCOVERY ) ;
	 ui._forums_channels_CB->setChecked(  detail.service_perm_flags & RS_SERVICE_PERM_DISTRIB   ) ;

    ui.name->setText(QString::fromUtf8(detail.name.c_str()));
    ui.peerid->setText(QString::fromStdString(detail.id));

    RetroShareLink link;
    link.createPerson(detail.id);

    ui.rsid->setText(link.toHtml());
    ui.rsid->setToolTip(link.title());

    if (!detail.isOnlyGPGdetail) {
        ui.avatar->setId(mId, false);

        ui.loc->setText(QString::fromUtf8(detail.location.c_str()));
        // Dont Show a timestamp in RS calculate the day
        ui.lastcontact->setText(DateTime::formatLongDateTime(detail.lastConnect));

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

        ui.statusline->setText(StatusDefs::connectStateString(detail));

        ui.ipAddressList->clear();
        for(std::list<std::string>::const_iterator it(detail.ipAddressList.begin());it!=detail.ipAddressList.end();++it)
            ui.ipAddressList->addItem(QString::fromStdString(*it));

        ui.loc->show();
        ui.label_loc->show();
        ui.statusline->show();
        ui.label_status->show();
        ui.lastcontact->show();
        ui.label_last_contact->show();
        ui.version->show();
        ui.label_version->show();

        ui.groupBox->show();
        ui.groupBox_4->show();
        ui.rsid->hide();
        ui.label_rsid->hide();
    } else {
        ui.avatar->setId(mId, true);

        ui.rsid->show();
        ui.label_rsid->show();
        ui.loc->hide();
        ui.label_loc->hide();
        ui.statusline->hide();
        ui.label_status->hide();
        ui.lastcontact->hide();
        ui.label_last_contact->hide();
        ui.version->hide();
        ui.label_version->hide();
        ui.groupBox_4->hide();

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
        ui.signersBox->setTitle(tr("My key is signed by : "));

    } else {
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
                ui.signKeyButton->show();
            } else {
                ui.signKeyButton->hide();
            }
        } else {
            ui.make_friend_button->show();
            ui.denyFriendButton->hide();
            ui.signKeyButton->hide();
            if (!detail.ownsign) {
                ui.signGPGKeyCheckBox->show();
                ui.signGPGKeyCheckBox->setChecked(false);
            } else {
                ui.signGPGKeyCheckBox->hide();
            }
        }

        //web of trust
        if (detail.trustLvl == RS_TRUST_LVL_ULTIMATE) {
            //trust is ultimate, it means it's one of our own keys
            ui.web_of_trust_label->setText(tr("Your trust in this peer is ultimate, it's probably a key you own."));
            ui.radioButton_trust_fully->hide();
            ui.radioButton_trust_marginnaly->hide();
            ui.radioButton_trust_never->hide();
        } else {
            ui.radioButton_trust_fully->show();
            ui.radioButton_trust_marginnaly->show();
            ui.radioButton_trust_never->show();
            if (detail.trustLvl == RS_TRUST_LVL_FULL) {
                ui.web_of_trust_label->setText(tr("Your trust in this peer is full."));
                ui.radioButton_trust_fully->setChecked(true);
                ui.radioButton_trust_fully->setIcon(QIcon(":/images/security-high-48.png"));
                ui.radioButton_trust_marginnaly->setIcon(QIcon(":/images/security-medium-off-48.png"));
                ui.radioButton_trust_never->setIcon(QIcon(":/images/security-low-off-48.png"));
            } else if (detail.trustLvl == RS_TRUST_LVL_MARGINAL) {
                ui.web_of_trust_label->setText(tr("Your trust in this peer is marginal."));
                ui.radioButton_trust_marginnaly->setChecked(true);
                ui.radioButton_trust_marginnaly->setIcon(QIcon(":/images/security-medium-48.png"));
                ui.radioButton_trust_never->setIcon(QIcon(":/images/security-low-off-48.png"));
                ui.radioButton_trust_fully->setIcon(QIcon(":/images/security-high-off-48.png"));
            } else if (detail.trustLvl == RS_TRUST_LVL_NEVER) {
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
            ui.is_signing_me->setText(tr("Peer has authenticated me as a friend and did sign my PGP key"));
        } else {
            ui.is_signing_me->setText(tr("Peer has not authenticated me as a friend and did not sign my PGP key"));
        }
    }

    QString text;
    for(std::list<std::string>::const_iterator it(detail.gpgSigners.begin());it!=detail.gpgSigners.end();++it) {
        link.createPerson(*it);
        if (link.valid()) {
            text += link.toHtml() + "<BR>";
        }
    }
    ui.signers->setHtml(text);

	 loadInvitePage() ;
}

void ConfCertDialog::loadInvitePage()
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

    std::string invite = rsPeers->GetRetroshareInvite(detail.id,ui._shouldAddSignatures_CB->isChecked(),ui._useOldFormat_CB->isChecked()) ; // this needs to be a SSL id

    ui.userCertificateText->setReadOnly(true);
    ui.userCertificateText->setMinimumHeight(200);
    ui.userCertificateText->setMinimumWidth(530);
    QFont font("Courier New",10,50,false);
    font.setStyleHint(QFont::TypeWriter,QFont::PreferMatch);
    font.setStyle(QFont::StyleNormal);
    ui.userCertificateText->setFont(font);
    ui.userCertificateText->setText(QString::fromUtf8(invite.c_str()));
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
            close();
            return;
        }
    }

    //check the GPG trustlvl
    if (ui.radioButton_trust_fully->isChecked() && detail.trustLvl != RS_TRUST_LVL_FULL) {
        //trust has changed to fully
        rsPeers->trustGPGCertificate(detail.id, RS_TRUST_LVL_FULL);
    } else if (ui.radioButton_trust_marginnaly->isChecked() && detail.trustLvl != RS_TRUST_LVL_MARGINAL) {
        rsPeers->trustGPGCertificate(detail.id, RS_TRUST_LVL_MARGINAL);
    } else if (ui.radioButton_trust_never->isChecked() && detail.trustLvl != RS_TRUST_LVL_NEVER) {
        rsPeers->trustGPGCertificate(detail.id, RS_TRUST_LVL_NEVER);
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
            emit configChanged();
    }

	 setServiceFlags() ;

    loadAll();
    close();
}

void ConfCertDialog::makeFriend()
{
    std::string gpg_id = rsPeers->getGPGId(mId);
    if (ui.signGPGKeyCheckBox->isChecked()) {
        rsPeers->signGPGCertificate(gpg_id);
    } 
	
    rsPeers->addFriend(mId, gpg_id);
	 setServiceFlags() ;
    loadAll();

    emit configChanged();
}

void ConfCertDialog::denyFriend()
{
    std::string gpg_id = rsPeers->getGPGId(mId);
    rsPeers->removeFriend(gpg_id);
    loadAll();

    emit configChanged();
}

void ConfCertDialog::signGPGKey()
{
    std::string gpg_id = rsPeers->getGPGId(mId);
    if (!rsPeers->signGPGCertificate(gpg_id)) {
                 QMessageBox::warning ( NULL,
                                tr("Signature Failure"),
                                tr("Maybe password is wrong"),
                                QMessageBox::Ok);
    }
    loadAll();

    emit configChanged();
}

/** Displays the help browser and displays the most recently viewed help
 * topic. */
void ConfCertDialog::showHelpDialog()
{
    showHelpDialog("trust");
}

/**< Shows the help browser and displays the given help <b>topic</b>. */
void ConfCertDialog::showHelpDialog(const QString &topic)
{
    HelpBrowser::showWindow(topic);
}
