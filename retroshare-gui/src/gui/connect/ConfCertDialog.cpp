/*******************************************************************************
 * gui/connect/ConfCertDialog.cpp                                              *
 *                                                                             *
 * Copyright (C) 2006 Crypton         <retroshare.project@gmail.com>           *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

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

#include <retroshare-gui/mainpage.h>

#include "gui/help/browser/helpbrowser.h"
#include "gui/common/PeerDefs.h"
#include "gui/common/StatusDefs.h"
#include "gui/RetroShareLink.h"
#include "gui/notifyqt.h"
#include "gui/common/AvatarDefs.h"
#include "gui/common/FilesDefs.h"
#include "gui/MainWindow.h"
#include "util/DateTime.h"
#include "util/misc.h"

static QMap<RsPeerId, ConfCertDialog*> instances_ssl;
static QMap<RsPgpId, ConfCertDialog*> instances_pgp;

ConfCertDialog *ConfCertDialog::instance(const RsPeerId& peer_id)
{

    ConfCertDialog *d = instances_ssl[peer_id];
    if (d) {
        return d;
    }

    RsPeerDetails details ;
    if(!rsPeers->getPeerDetails(peer_id,details))
        return NULL ;

    d = new ConfCertDialog(peer_id,details.gpg_id);
    instances_ssl[peer_id] = d;

    return d;
}
ConfCertDialog *ConfCertDialog::instance(const RsPgpId& pgp_id)
{

    ConfCertDialog *d = instances_pgp[pgp_id];
    if (d) {
        return d;
    }

    d = new ConfCertDialog(RsPeerId(),pgp_id);
    instances_pgp[pgp_id] = d;

    return d;
}
/** Default constructor */
ConfCertDialog::ConfCertDialog(const RsPeerId& id, const RsPgpId &pgp_id, QWidget *parent, Qt::WindowFlags flags)
  : QDialog(parent, flags), peerId(id), pgpId(pgp_id)
{
    /* Invoke Qt Designer generated QObject setup routine */
    ui.setupUi(this);
	Settings->loadWidgetInformation(this);
    ui.headerFrame->setHeaderImage(FilesDefs::getPixmapFromQtResourcePath(":/images/user/identityinfo64.png"));
    //ui.headerFrame->setHeaderText(tr("Friend node details"));

    //ui._chat_CB->hide() ;

	setAttribute(Qt::WA_DeleteOnClose, true);
    ui._shortFormat_CB->setChecked(true);

    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(applyDialog()));
    connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(close()));
    connect(ui._shouldAddSignatures_CB, SIGNAL(toggled(bool)), this, SLOT(loadInvitePage()));
    connect(ui._includeIPHistory_CB, SIGNAL(toggled(bool)), this, SLOT(loadInvitePage()));
    connect(ui._shortFormat_CB, SIGNAL(toggled(bool)), this, SLOT(loadInvitePage()));

    ui.avatar->setFrameType(AvatarWidget::NORMAL_FRAME);

    MainWindow *w = MainWindow::getInstance();
    if (w) {
        connect(this, SIGNAL(configChanged()), w->getPage(MainWindow::Network), SLOT(insertConnect()));
    }
}

ConfCertDialog::~ConfCertDialog()
{
	Settings->saveWidgetInformation(this);
    QMap<RsPeerId, ConfCertDialog*>::iterator it = instances_ssl.find(peerId);
    if (it != instances_ssl.end())
	    instances_ssl.erase(it);

    QMap<RsPgpId, ConfCertDialog*>::iterator it2 = instances_pgp.find(pgpId);
    if (it2 != instances_pgp.end())
	    instances_pgp.erase(it2);
}


void ConfCertDialog::loadAll()
{
    for(QMap<RsPeerId, ConfCertDialog*>::iterator it = instances_ssl.begin(); it != instances_ssl.end(); ++it)  it.value()->load();
    for(QMap<RsPgpId , ConfCertDialog*>::iterator it = instances_pgp.begin(); it != instances_pgp.end(); ++it)  it.value()->load();
}

void ConfCertDialog::load()
{
    RsPeerDetails detail;

    if(!rsPeers->getPeerDetails(peerId, detail))
    {
        QMessageBox::information(this,
                                 tr("RetroShare"),
                                 tr("Error : cannot get peer details."));
        close();
        return;
    }

      //ui.pgpfingerprint->setText(QString::fromUtf8(detail.name.c_str()));
      ui.peerid->setText(QString::fromStdString(detail.id.toStdString()));
      
      nameAndLocation = QString("%1 (%2)").arg(QString::fromUtf8(detail.name.c_str())).arg(QString::fromUtf8(detail.location.c_str()));

      ui.headerFrame->setHeaderText(nameAndLocation);


	RetroShareLink link = RetroShareLink::createPerson(detail.gpg_id);
	ui.pgpfingerprint->setText(link.toHtml());
	ui.pgpfingerprint->setToolTip(link.title());

      ui.avatar->setId(ChatId(peerId));

		 ui.loc->setText(QString::fromUtf8(detail.location.c_str()));
		 // Dont Show a timestamp in RS calculate the day
		 ui.lastcontact->setText(DateTime::formatLongDateTime(detail.lastConnect));

		 /* set retroshare version */
		 std::string version;
		 rsDisc->getPeerVersion(detail.id, version);
		 ui.version->setText(QString::fromStdString(version));
		
		 /* Custom state string */ 
		 QString statustring =  QString::fromUtf8(rsMsgs->getCustomStateString(detail.id).c_str());
     	 ui.statusmessage->setText(statustring);


		 RsPeerCryptoParams cdet ;
		 if(RsControl::instance()->getPeerCryptoDetails(detail.id,cdet) && cdet.connexion_state!=0)
			 ui.crypto_info->setText(QString::fromStdString(cdet.cipher_name));
		 else
			 ui.crypto_info->setText(tr("Not connected")) ;

		 if (detail.isHiddenNode)
		 {
			 // enable only the first row and set name of the first label to "Hidden Address"
			 ui.l_localAddress->setText(tr("Hidden Address"));

			 ui.l_extAddress->setEnabled(false);
			 ui.extAddress->setEnabled(false);
			 ui.l_portExternal->setEnabled(false);
			 ui.extPort->setEnabled(false);

			 ui.l_dynDNS->setEnabled(false);
			 ui.dynDNS->setEnabled(false);

			 /* set hidden address */
			 ui.localAddress->setText(QString::fromStdString(detail.hiddenNodeAddress));
			 ui.localPort -> setValue(detail.hiddenNodePort);

			 // set everything else to none
			 ui.extAddress->setText(tr("none"));
			 ui.extPort->setValue(0);
			 ui.dynDNS->setText(tr("none"));
		 }
		 else
		 {
			 // enable everything and set name of the first label to "Local Address"
			 ui.l_localAddress->setText(tr("Local Address"));

			 ui.l_extAddress->setEnabled(true);
			 ui.extAddress->setEnabled(true);
			 ui.l_portExternal->setEnabled(true);
			 ui.extPort->setEnabled(true);

			 ui.l_dynDNS->setEnabled(true);
			 ui.dynDNS->setEnabled(true);

			 /* set local address */			 
			 ui.localAddress->setText(QString::fromStdString(detail.localAddr));
			 ui.localPort -> setValue(detail.localPort);
			 /* set the server address */
			 ui.extAddress->setText(QString::fromStdString(detail.extAddr));
			 ui.extPort -> setValue(detail.extPort);

			 ui.dynDNS->setText(QString::fromStdString(detail.dyndns));
		 }

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

		 //ui.groupBox->show();
		 ui.groupBox_4->show();
		 //ui.tabWidget->show();
         //ui.rsid->hide();
         //ui.label_rsid->hide();
		 ui.pgpfingerprint->show();
         //ui.pgpfingerprint_label->show();

		ui.stabWidget->setTabEnabled(2,true) ;
		ui.stabWidget->setTabEnabled(3,true) ;
		ui.stabWidget->setTabEnabled(4,true) ;

     loadInvitePage() ;
}

void ConfCertDialog::loadInvitePage()
{
    RsPeerDetails detail;

    if (!rsPeers->getPeerDetails(peerId, detail))
    {
        QMessageBox::information(this,
                                 tr("RetroShare"),
                                 tr("Error : cannot get peer details."));
        close();
        return;
    }

    ui._shouldAddSignatures_CB->setEnabled(detail.gpgSigners.size() > 1) ;

 //   std::string pgp_key = rsPeers->getPGPKey(detail.gpg_id,ui._shouldAddSignatures_CB_2->isChecked()) ; // this needs to be a SSL id

//	ui.userCertificateText_2->setReadOnly(true);
//	ui.userCertificateText_2->setMinimumHeight(200);
//	ui.userCertificateText_2->setMinimumWidth(530);
//	QFont font("Courier New",10,50,false);
//	font.setStyleHint(QFont::TypeWriter,QFont::PreferMatch);
//	font.setStyle(QFont::StyleNormal);
//	ui.userCertificateText_2->setFont(font);
//	ui.userCertificateText_2->setText(QString::fromUtf8(pgp_key.c_str()));

	std::string invite ;

    if(ui._shortFormat_CB->isChecked())
	{
		rsPeers->getShortInvite(invite,detail.id,true,!ui._includeIPHistory_CB->isChecked() );
		ui.stabWidget->setTabText(1, tr("Retroshare ID"));
	}
	else
	{
		invite = rsPeers->GetRetroshareInvite(detail.id, ui._shouldAddSignatures_CB->isChecked(), ui._includeIPHistory_CB->isChecked() ) ;
		ui.stabWidget->setTabText(1, tr("Retroshare Certificate"));
	}
	
	QString infotext = getCertificateDescription(detail,ui._shouldAddSignatures_CB->isChecked(),ui._shortFormat_CB->isChecked(), ui._includeIPHistory_CB->isChecked() );

    ui.userCertificateText->setToolTip(infotext) ;

        ui.userCertificateText->setReadOnly(true);
        ui.userCertificateText->setMinimumHeight(200);
        ui.userCertificateText->setMinimumWidth(530);
        QFont font("Courier New",10,50,false);
        font.setStyleHint(QFont::TypeWriter,QFont::PreferMatch);
        font.setStyle(QFont::StyleNormal);
        ui.userCertificateText->setFont(font);
        ui.userCertificateText->setText(QString::fromUtf8(invite.c_str()));
}

QString ConfCertDialog::getCertificateDescription(const RsPeerDetails& detail,bool signatures_included,bool use_short_format,bool include_additional_locators)
{
    //infotext += tr("<p>Use this certificate to make new friends. Send it by email, or give it hand to hand.</p>") ;
    QString infotext = QObject::tr("<p>This certificate contains:") ;
    infotext += "<UL>" ;

    if(use_short_format)
    {
		infotext += "<li> a <b>Profile fingerprint</b>";
    	infotext += " (" + QString::fromUtf8(detail.name.c_str())  + "@" + detail.fpr.toStdString().c_str()+") " ;
    }
	else
    {
		infotext += "<li> a <b>Profile public key</b>";
    	infotext += " (" + QString::fromUtf8(detail.name.c_str())  + "@" + detail.gpg_id.toStdString().c_str()+") " ;
    }

    if(signatures_included && !use_short_format)
        infotext += tr("with")+" "+QString::number(detail.gpgSigners.size()-1)+" "+tr("external signatures</li>") ;
    else
        infotext += "</li>" ;

    infotext += tr("<li>a <b>node ID</b> and <b>name</b>") +" (" + detail.id.toStdString().c_str() + ", " + QString::fromUtf8(detail.location.c_str()) +")" ;
    infotext += "</li>" ;

    if(detail.isHiddenNode)
        infotext += tr("<li> <b>onion address</b> and <b>port</b>") +" (" + detail.hiddenNodeAddress.c_str() + ":" + QString::number(detail.hiddenNodePort)+ ")</li>";
    else if(!include_additional_locators)
    {
        if(!detail.localAddr.empty()) infotext += tr("<li><b>IP address</b> and <b>port</b>: ") + detail.localAddr.c_str() + ":" + QString::number(detail.localPort)+ "</li>";
        if(!detail.extAddr.empty()) infotext += tr("<li><b>IP address</b> and <b>port</b>: ") + detail.extAddr.c_str() + ":" + QString::number(detail.extPort)+ "</li>";
    }
    else for(auto it(detail.ipAddressList.begin());it!=detail.ipAddressList.end();++it)
	{
		infotext += "<li>" ;
		infotext += tr("<b>IP address</b> and <b>port</b>: ") + QString::fromStdString(*it) ;
		infotext += "</li>" ;
	}

    infotext += QString("</p>") ;

    if(rsPeers->getOwnId() == detail.id)
        infotext += tr("<p>You can use this certificate to make new friends. Send it by email, or give it hand to hand.</p>") ;

    return infotext;
}


void ConfCertDialog::applyDialog()
{
    std::cerr << "ConfCertDialog::applyDialog() called" << std::endl ;
    RsPeerDetails detail;
    if (!rsPeers->getPeerDetails(peerId, detail))
    {
	    if (!rsPeers->getGPGDetails(pgpId, detail)) {
		    QMessageBox::information(this,
		                             tr("RetroShare"),
		                             tr("Error : cannot get peer details."));
		    close();
		    return;
	    }
    }

    if(!detail.isHiddenNode)
    {
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
		    rsPeers->setLocalAddress(peerId, ui.localAddress->text().toStdString(), ui.localPort->value());

	    if (extChanged)
		    rsPeers->setExtAddress(peerId,ui.extAddress->text().toStdString(), ui.extPort->value());

	    if (dnsChanged)
		    rsPeers->setDynDNS(peerId, ui.dynDNS->text().toStdString());

	    if(localChanged || extChanged || dnsChanged)
		    emit configChanged();
    }
    else
    {
	    if((detail.hiddenNodeAddress != ui.localAddress->text().toStdString()) || (detail.hiddenNodePort != ui.localPort->value()))
	    {
		    rsPeers->setHiddenNode(peerId,ui.localAddress->text().toStdString(), ui.localPort->value());
		    emit configChanged();
	    }
    }

    loadAll();
    close();
}

void ConfCertDialog::showHelpDialog()
{
    showHelpDialog("trust");
}

/**< Shows the help browser and displays the given help <b>topic</b>. */
void ConfCertDialog::showHelpDialog(const QString &topic)
{
    HelpBrowser::showWindow(topic);
}
