/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2009 The RetroShare Team, Oleksiy Bilyanskyy
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

#include <QUrl>
#include <QDesktopServices>
#include <QMessageBox>
#include <QClipboard>
#include <QFileDialog>
#include <QTextStream>
#include <QTextCodec>

#include "ConnectFriendWizard.h"
#include "ui_ConnectFriendWizard.h"
#include "gui/common/PeerDefs.h"
#include "gui/common/GroupDefs.h"
#include "gui/GetStartedDialog.h"

#include <retroshare/rsiface.h>

//#define FRIEND_WIZARD_DEBUG

ConnectFriendPage::ConnectFriendPage(QWidget *parent) : QWizardPage(parent)
{
	useComplete = false;
	complete = true;
}

void ConnectFriendPage::setComplete(bool isComplete)
{
	useComplete = true;
	complete = isComplete;
	emit completeChanged();
}

bool ConnectFriendPage::isComplete() const
{
	if (useComplete) {
		return complete;
	}

	return QWizardPage::isComplete();
}

ConnectFriendWizard::ConnectFriendWizard(QWidget *parent) :
	QWizard(parent), ui(new Ui::ConnectFriendWizard)
{
	ui->setupUi(this);

	/* add stylesheet to title */
	QList<int> ids = pageIds();
	for (QList<int>::iterator it = ids.begin(); it != ids.end(); ++it) {
		QWizardPage *p = page(*it);
		p->setTitle(QString("<span style=\"font-size:16pt; font-weight:500; color:white;\">%1</span>").arg(p->title()));
	}

// this define comes from Qt example. I don't have mac, so it wasn't tested
#ifndef Q_WS_MAC
	setWizardStyle(ModernStyle);
#endif

	setStartId(Page_Intro);

// at this moment I don't know, what information should be in help
//	setOption(HaveHelpButton, true);
//	connect(this, SIGNAL(helpRequested()), this, SLOT(showHelp()));

	setPixmap(QWizard::LogoPixmap, QPixmap(":/images/connect/connectFriendLogo.png"));

// we have no good pictures for watermarks
//	setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/connectFriendWatermark.png"));
	setPixmap(QWizard::BannerPixmap, QPixmap(":/images/connect/connectFriendBanner.png"));

	/* register global fields */
	ui->ErrorMessagePage->registerField("errorMessage", ui->messageLabel, "text");

	/* disable not used pages */
	ui->foffRadioButton->hide();
	ui->rsidRadioButton->hide();
}

QString ConnectFriendWizard::getErrorString(uint32_t error_code)
{
	switch(error_code)
	{
		case CERTIFICATE_PARSING_ERROR_SIZE_ERROR: 					return tr("Abnormal size read is bigger than memory block.") ;
		case CERTIFICATE_PARSING_ERROR_INVALID_LOCATION_ID: 		return tr("Invalid location id.") ;
		case CERTIFICATE_PARSING_ERROR_INVALID_EXTERNAL_IP: 		return tr("Invalid external IP.") ;
		case CERTIFICATE_PARSING_ERROR_INVALID_LOCAL_IP: 			return tr("Invalid local IP.") ;
		case CERTIFICATE_PARSING_ERROR_INVALID_CHECKSUM_SECTION: return tr("Invalid checksum section.") ;
		case CERTIFICATE_PARSING_ERROR_CHECKSUM_ERROR: 				return tr("Checksum mismatch. Certificate is corrupted.") ;
		case CERTIFICATE_PARSING_ERROR_UNKNOWN_SECTION_PTAG:		return tr("Unknown section type found (Certificate might be corrupted).") ;
		case CERTIFICATE_PARSING_ERROR_MISSING_CHECKSUM:			return tr("Missing checksum.") ;

		default:
			return tr("Unknown certificate error") ;
	}
}

void ConnectFriendWizard::setCertificate(const QString &certificate, bool friendRequest)
{
	if (certificate.isEmpty()) {
		setStartId(Page_Intro);
		return;
	}

	uint32_t cert_load_error_code;

	if (rsPeers->loadDetailsFromStringCert(certificate.toUtf8().constData(), peerDetails, cert_load_error_code))
	{
#ifdef FRIEND_WIZARD_DEBUG
		std::cerr << "ConnectFriendWizard got id : " << peerDetails.id << "; gpg_id : " << peerDetails.gpg_id << std::endl;
#endif
		mCertificate = certificate.toUtf8().constData();
		setStartId(friendRequest ? Page_FriendRequest : Page_Conclusion);
	} else {
		// error message
		setField("errorMessage", tr("Certificate Load Failed") + ": \n\n" + getErrorString(cert_load_error_code)) ;
		setStartId(Page_ErrorMessage);
	}
}

void ConnectFriendWizard::setGpgId(const std::string &gpgId, bool friendRequest)
{
	if (!rsPeers->getPeerDetails(gpgId, peerDetails)) {
		setField("errorMessage", tr("Cannot get peer details of PGP key %1").arg(QString::fromStdString(gpgId)));
		setStartId(Page_ErrorMessage);
		return;
	}

	setStartId(friendRequest ? Page_FriendRequest : Page_Conclusion);
}

ConnectFriendWizard::~ConnectFriendWizard()
{
	delete ui;
}

static void fillGroups(ConnectFriendWizard *wizard, QComboBox *comboBox, const QString &groupId)
{
	std::list<RsGroupInfo> groupInfoList;
	rsPeers->getGroupInfoList(groupInfoList);
	GroupDefs::sortByName(groupInfoList);
	comboBox->addItem("", ""); // empty value
	for (std::list<RsGroupInfo>::iterator groupIt = groupInfoList.begin(); groupIt != groupInfoList.end(); ++groupIt) {
		comboBox->addItem(GroupDefs::name(*groupIt), QString::fromStdString(groupIt->id));
	}

	if (groupId.isEmpty() == false) {
		comboBox->setCurrentIndex(comboBox->findData(groupId));
	}
	QObject::connect(comboBox, SIGNAL(currentIndexChanged(int)), wizard, SLOT(groupCurrentIndexChanged(int)));
}

void ConnectFriendWizard::initializePage(int id)
{
	switch ((Page) id) {
	case Page_Intro:
		ui->textRadioButton->setChecked(true);
		break;
	case Page_Text:
		connect(ui->userCertHelpButton, SIGNAL( clicked()), this, SLOT(showHelpUserCert()));
		connect(ui->userCertIncludeSignaturesButton, SIGNAL(clicked()), this, SLOT(toggleSignatureState()));
		connect(ui->userCertOldFormatButton, SIGNAL(clicked()), this, SLOT(toggleFormatState()));
		connect(ui->userCertCopyButton, SIGNAL(clicked()), this, SLOT(copyCert()));
		connect(ui->userCertSaveButton, SIGNAL(clicked()), this, SLOT(saveCert()));
		connect(ui->userCertMailButton, SIGNAL(clicked()), this, SLOT(runEmailClient()));
		connect(ui->friendCertEdit, SIGNAL(textChanged()), this, SLOT(friendCertChanged()));

		cleanfriendCertTimer = new QTimer(this);
		cleanfriendCertTimer->setSingleShot(true);
		cleanfriendCertTimer->setInterval(1000); // 1 second
		connect(cleanfriendCertTimer, SIGNAL(timeout()), this, SLOT(cleanFriendCert()));

		ui->userCertOldFormatButton->setChecked(true); 

		toggleFormatState(false);
		toggleSignatureState(false);
		updateOwnCert();

		cleanFriendCert();

		break;
	case Page_Cert:
		connect(ui->userFileCreateButton, SIGNAL(clicked()), this, SLOT(generateCertificateCalled()));
		connect(ui->friendFileNameOpenButton, SIGNAL(clicked()), this, SLOT(loadFriendCert()));

		ui->friendFileNameEdit->setAcceptFile(true);

		ui->CertificatePage->registerField("friendCertificateFile*", ui->friendFileNameEdit);
		break;
	case Page_Foff:
		ui->userSelectionCB->addItem(tr("Any peer I've not signed"));
		ui->userSelectionCB->addItem(tr("Friends of my friends who already trust me"));
		ui->userSelectionCB->addItem(tr("Signed peers showing as denied"));

		ui->selectedPeersTW->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("")));
		ui->selectedPeersTW->setHorizontalHeaderItem(1, new QTableWidgetItem(tr("Peer name")));
		ui->selectedPeersTW->setHorizontalHeaderItem(2, new QTableWidgetItem(tr("Also signed by")));
		ui->selectedPeersTW->setHorizontalHeaderItem(3, new QTableWidgetItem(tr("Peer id")));

		connect(ui->makeFriendButton, SIGNAL(clicked()), this, SLOT(signAllSelectedUsers()));
		connect(ui->userSelectionCB, SIGNAL(activated(int)), this, SLOT(updatePeersList(int)));

		updatePeersList(ui->userSelectionCB->currentIndex());

		ui->FofPage->setComplete(false);
		break;
	case Page_Rsid:
		ui->RsidPage->registerField("friendRSID*", ui->friendRsidEdit);
		break;
	case Page_Email:
		ui->EmailPage->registerField("addressEdit*", ui->addressEdit);
		ui->EmailPage->registerField("subjectEdit*", ui->subjectEdit);

		ui->subjectEdit->setText(tr("RetroShare Invitation"));
		ui->inviteTextEdit->setPlainText(GetStartedDialog::GetInviteText());

		break;
	case Page_ErrorMessage:
		break;
	case Page_Conclusion:
		{
			std::cerr << "Conclusion page id : " << peerDetails.id << "; gpg_id : " << peerDetails.gpg_id << std::endl;

			ui->_anonymous_routing_CB_2->setChecked(peerDetails.service_perm_flags & RS_SERVICE_PERM_TURTLE) ;
			ui->_discovery_CB_2        ->setChecked(peerDetails.service_perm_flags & RS_SERVICE_PERM_DISCOVERY) ;
			ui->_forums_channels_CB_2  ->setChecked(peerDetails.service_perm_flags & RS_SERVICE_PERM_DISTRIB) ;
			ui->_direct_transfer_CB_2  ->setChecked(peerDetails.service_perm_flags & RS_SERVICE_PERM_DIRECT_DL) ;

			//set the radio button to sign the GPG key
			if (peerDetails.accept_connection && !peerDetails.ownsign) {
				//gpg key connection is already accepted, don't propose to accept it again
				ui->signGPGCheckBox->setChecked(false);
				ui->acceptNoSignGPGCheckBox->hide();
				ui->acceptNoSignGPGCheckBox->setChecked(false);
			}
			if (!peerDetails.accept_connection && peerDetails.ownsign) {
				//gpg key is already signed, don't propose to sign it again
				ui->acceptNoSignGPGCheckBox->setChecked(true);
				ui->signGPGCheckBox->hide();
				ui->signGPGCheckBox->setChecked(false);
			}
			if (!peerDetails.accept_connection && !peerDetails.ownsign) {
				ui->acceptNoSignGPGCheckBox->setChecked(true);
				ui->signGPGCheckBox->show();
				ui->signGPGCheckBox->setChecked(false);
				ui->acceptNoSignGPGCheckBox->show();
			}
			if (peerDetails.accept_connection && peerDetails.ownsign) {
				ui->acceptNoSignGPGCheckBox->setChecked(false);
				ui->acceptNoSignGPGCheckBox->hide();
				ui->signGPGCheckBox->setChecked(false);
				ui->signGPGCheckBox->hide();
				ui->alreadyRegisteredLabel->show();
			} else {
				ui->alreadyRegisteredLabel->hide();
			}

			QString trustString;
			switch (peerDetails.validLvl) {
			case RS_TRUST_LVL_ULTIMATE:
				trustString = tr("Ultimate");
				break;
			case RS_TRUST_LVL_FULL:
				trustString = tr("Full");
				break;
			case RS_TRUST_LVL_MARGINAL:
				trustString = tr("Marginal");
				break;
			case RS_TRUST_LVL_NEVER:
				trustString = tr("None");
				break;
			default:
				trustString = tr("No Trust");
				break;
			}

			QString ts;
			std::list<std::string>::iterator it;
			for (it = peerDetails.gpgSigners.begin(); it != peerDetails.gpgSigners.end(); ++it) {
				{
					std::string peer_name = rsPeers->getPeerName(*it) ;

					// This is baaaad code. We should handle this kind of errors with proper exceptions.
					// This happens because signers from a unknown key cannt be found in the keyring, including
					// self-signatures.
					//
					if(peer_name == "[Unknown PGP Cert name]" && *it == peerDetails.gpg_id)
						peer_name = peerDetails.name ;

				ts += QString("%1<%2>\n").arg(QString::fromUtf8(peer_name.c_str()), QString::fromStdString(*it));
				}
			}

			ui->nameEdit->setText(QString::fromUtf8(peerDetails.name.c_str()));
			ui->trustEdit->setText(trustString);
			ui->emailEdit->setText(QString::fromUtf8(peerDetails.email.c_str()));
			ui->locationEdit->setText(QString::fromUtf8(peerDetails.location.c_str()));
			ui->signersEdit->setPlainText(ts);

			fillGroups(this, ui->groupComboBox, groupId);
		}
		break;
	case Page_FriendRequest:
		{
			std::cerr << "Friend request page id : " << peerDetails.id << "; gpg_id : " << peerDetails.gpg_id << std::endl;

			ui->fr_avatar->setFrameType(AvatarWidget::NORMAL_FRAME);
			setPixmap(QWizard::LogoPixmap, QPixmap(":/images/user/user_request48.png"));

			//set the radio button to sign the GPG key
			if (peerDetails.accept_connection && !peerDetails.ownsign) {
				//gpg key connection is already accepted, don't propose to accept it again
				ui->fr_signGPGCheckBox->setChecked(false);
				ui->fr_acceptNoSignGPGCheckBox->hide();
				ui->fr_acceptNoSignGPGCheckBox->setChecked(false);
			}
			if (!peerDetails.accept_connection && peerDetails.ownsign) {
				//gpg key is already signed, don't propose to sign it again
				ui->fr_acceptNoSignGPGCheckBox->setChecked(true);
				ui->fr_signGPGCheckBox->hide();
				ui->fr_signGPGCheckBox->setChecked(false);
			}
			if (!peerDetails.accept_connection && !peerDetails.ownsign) {
				ui->fr_acceptNoSignGPGCheckBox->setChecked(true);
				ui->fr_signGPGCheckBox->show();
				ui->fr_signGPGCheckBox->setChecked(false);
				ui->fr_acceptNoSignGPGCheckBox->show();
			}
			if (peerDetails.accept_connection && peerDetails.ownsign) {
				ui->fr_acceptNoSignGPGCheckBox->setChecked(false);
				ui->fr_acceptNoSignGPGCheckBox->hide();
				ui->fr_signGPGCheckBox->setChecked(false);
				ui->fr_signGPGCheckBox->hide();
			}

			ui->fr_nameEdit->setText(QString::fromUtf8(peerDetails.name.c_str()));
			ui->fr_emailEdit->setText(QString::fromUtf8(peerDetails.email.c_str()));
			ui->fr_locationEdit->setText(QString::fromUtf8(peerDetails.location.c_str()));
			
			ui->fr_label->setText(tr("You have a friend request from") + " " + QString::fromUtf8(peerDetails.name.c_str()));

			fillGroups(this, ui->fr_groupComboBox, groupId);
		}
		break;
	}
}

static void sendMail(QString sAddress, QString sSubject, QString sBody)
{
#ifdef Q_WS_WIN
	/* search and replace the end of lines with: "%0D%0A" */
	sBody.replace("\n", "%0D%0A");
#endif

	QUrl url = QUrl("mailto:" + sAddress);
	url.addEncodedQueryItem("subject", QUrl::toPercentEncoding(sSubject));
	url.addEncodedQueryItem("body", QUrl::toPercentEncoding(sBody));

	std::cerr << "MAIL STRING:" << (std::string)url.toEncoded().constData() << std::endl;

	/* pass the url directly to QDesktopServices::openUrl */
	QDesktopServices::openUrl (url);
}

bool ConnectFriendWizard::validateCurrentPage()
{
	error = true;

	switch ((Page) currentId()) {
	case Page_Intro:
		break;
	case Page_Text:
		{
			std::string certstr = ui->friendCertEdit->toPlainText().toUtf8().constData();
			uint32_t cert_load_error_code;

			if (rsPeers->loadDetailsFromStringCert(certstr, peerDetails, cert_load_error_code)) {
				mCertificate = certstr;
#ifdef FRIEND_WIZARD_DEBUG
				std::cerr << "ConnectFriendWizard got id : " << peerDetails.id << "; gpg_id : " << peerDetails.gpg_id << std::endl;
#endif
				break;
			}
			// error message
			setField("errorMessage", tr("Certificate Load Failed") + ": \n\n" + getErrorString(cert_load_error_code)) ;
			error = false;
			break;
		}
	case Page_Cert:
		{
			QString fn = ui->friendFileNameEdit->text();
			if (QFile::exists(fn)) {
				//Todo: move read from file to p3Peers::loadCertificateFromFile

				// read from file
				std::string certstr;
				QFile CertFile(fn);
				if (CertFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
					certstr = QString(CertFile.readAll()).toStdString();
					CertFile.close();
				}

				if (certstr.empty()) {
					setField("errorMessage", QString(tr("Certificate Load Failed:can't read from file %1 ")).arg(fn) );
					error = false;
					break;
				}

				uint32_t cert_error_code;
				if (rsPeers->loadDetailsFromStringCert(certstr, peerDetails, cert_error_code)) {
					mCertificate = certstr;
#ifdef FRIEND_WIZARD_DEBUG
					std::cerr << "ConnectFriendWizard got id : " << peerDetails.id << "; gpg_id : " << peerDetails.gpg_id << std::endl;
#endif
				} else {
					setField("errorMessage", QString(tr("Certificate Load Failed:something is wrong with %1 ")).arg(fn) + ": " + getErrorString(cert_error_code));
					error = false;
				}
			} else {
				setField("errorMessage", QString(tr("Certificate Load Failed:file %1 not found")).arg(fn));
				error = false;
			}
			break;
		}
	case Page_Foff:
		break;
	case Page_Rsid:
		{
			QString rsidstring = ui->friendRsidEdit->text();

			if (rsidstring.isEmpty()) {
				return false;
			}

			// search for peer id in string
			std::string rsidstr = PeerDefs::idFromRsid(rsidstring, false);

			if (rsidstr.empty() || !rsPeers->getPeerDetails(rsidstr, peerDetails)) {
				setField("errorMessage", tr("This Peer %1 is not available in your Network").arg(rsidstring));
				error = false;
			}
			break;
		}
	case Page_Email:
		{
			QString mailaddresses = ui->addressEdit->text();
			if (mailaddresses.isEmpty()) {
				return false;
			}

			QString body = ui->inviteTextEdit->toPlainText();

			body += "\n" + GetStartedDialog::GetCutBelowText();
			body += "\n\n" + QString::fromUtf8(rsPeers->GetRetroshareInvite(false).c_str());

			sendMail (mailaddresses, ui->subjectEdit->text(), body);
			return true;
		}
		break;
	case Page_ErrorMessage:
		break;
	case Page_Conclusion:
		break;
	case Page_FriendRequest:
		break;
	}

	return true;
}

int ConnectFriendWizard::nextId() const
{
	switch ((Page) currentId()) {
	case Page_Intro:
		if (ui->textRadioButton->isChecked()) return Page_Text;
		if (ui->certRadioButton->isChecked()) return Page_Cert;
		if (ui->foffRadioButton->isChecked()) return Page_Foff;
		if (ui->rsidRadioButton->isChecked()) return Page_Rsid;
		if (ui->emailRadioButton->isChecked()) return Page_Email;
		return ConnectFriendWizard::Page_Foff;
	case Page_Text:
	case Page_Cert:
	case Page_Rsid:
		return error ? ConnectFriendWizard::Page_Conclusion : ConnectFriendWizard::Page_ErrorMessage;
	case Page_Foff:
	case Page_Email:
	case Page_ErrorMessage:
	case Page_Conclusion:
	case Page_FriendRequest:
		return -1;
	}

	return -1;
}

ServicePermissionFlags ConnectFriendWizard::serviceFlags() const
{
	ServicePermissionFlags flags(0) ;

	if (hasVisitedPage(Page_FriendRequest))
	{
		if(ui->_anonymous_routing_CB->isChecked()) flags |= RS_SERVICE_PERM_TURTLE ;
		if(        ui->_discovery_CB->isChecked()) flags |= RS_SERVICE_PERM_DISCOVERY ;
		if(  ui->_forums_channels_CB->isChecked()) flags |= RS_SERVICE_PERM_DISTRIB ;
		if(  ui->_direct_transfer_CB->isChecked()) flags |= RS_SERVICE_PERM_DIRECT_DL ;
	} else if (hasVisitedPage(Page_Conclusion)) {
		if(ui->_anonymous_routing_CB_2->isChecked()) flags |= RS_SERVICE_PERM_TURTLE ;
		if(        ui->_discovery_CB_2->isChecked()) flags |= RS_SERVICE_PERM_DISCOVERY ;
		if(  ui->_forums_channels_CB_2->isChecked()) flags |= RS_SERVICE_PERM_DISTRIB ;
		if(  ui->_direct_transfer_CB_2->isChecked()) flags |= RS_SERVICE_PERM_DIRECT_DL ;
	}
	return flags ;
}
void ConnectFriendWizard::accept()
{
	bool sign = false;
	bool accept_connection = false;

	if (hasVisitedPage(Page_Conclusion)) {
		std::cerr << "ConnectFriendWizard::accept() called with page conclusion visited" << std::endl;

		sign = ui->signGPGCheckBox->isChecked();
		accept_connection = ui->acceptNoSignGPGCheckBox->isChecked();
	} else if (hasVisitedPage(Page_FriendRequest)) {
		std::cerr << "ConnectFriendWizard::accept() called with page friend request visited" << std::endl;

		sign = ui->fr_signGPGCheckBox->isChecked();
		accept_connection = ui->fr_acceptNoSignGPGCheckBox->isChecked();
	} else {
		QDialog::accept();
		return;
	}

	if (!mCertificate.empty() && (accept_connection || sign))
	{
		std::string pgp_id,ssl_id,error_string ;

		if(!rsPeers->loadCertificateFromString(mCertificate,ssl_id,pgp_id,error_string))
		{
			std::cerr << "ConnectFriendWizard::accept(): cannot load that certificate." << std::endl;
			return ;
		}
	}

	if (!peerDetails.gpg_id.empty()) {
		if (sign) {
			std::cerr << "ConclusionPage::validatePage() signing GPG key." << std::endl;
			rsPeers->signGPGCertificate(peerDetails.gpg_id); //bye default sign set accept_connection to true;
		} else if (accept_connection) {
			std::cerr << "ConclusionPage::validatePage() accepting GPG key for connection." << std::endl;
			rsPeers->addFriend("", peerDetails.gpg_id,serviceFlags()) ;
			rsPeers->setServicePermissionFlags(peerDetails.gpg_id,serviceFlags()) ;
		}

		if (!groupId.isEmpty()) {
			rsPeers->assignPeerToGroup(groupId.toStdString(), peerDetails.gpg_id, true);
		}
	}

	if (peerDetails.id != "") {
		rsPeers->addFriend(peerDetails.id, peerDetails.gpg_id,serviceFlags()) ;

		//let's check if there is ip adresses in the wizard.
		if (!peerDetails.extAddr.empty() && peerDetails.extPort) {
			std::cerr << "ConnectFriendWizard::accept() : setting ip ext address." << std::endl;
			rsPeers->setExtAddress(peerDetails.id, peerDetails.extAddr, peerDetails.extPort);
		}
		if (!peerDetails.localAddr.empty() && peerDetails.localPort) {
			std::cerr << "ConnectFriendWizard::accept() : setting ip local address." << std::endl;
			rsPeers->setLocalAddress(peerDetails.id, peerDetails.localAddr, peerDetails.localPort);
		}
		if (!peerDetails.dyndns.empty()) {
			std::cerr << "ConnectFriendWizard::accept() : setting DynDNS." << std::endl;
			rsPeers->setDynDNS(peerDetails.id, peerDetails.dyndns);
		}
		if (!peerDetails.location.empty()) {
			std::cerr << "ConnectFriendWizard::accept() : setting peerLocation." << std::endl;
			rsPeers->setLocation(peerDetails.id, peerDetails.location);
		}
	}

	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_NEIGHBOURS,1) ;

	QDialog::accept();
}

//============================= TextPage =====================================

void ConnectFriendWizard::updateOwnCert()
{
	std::string invite = rsPeers->GetRetroshareInvite(ui->userCertIncludeSignaturesButton->isChecked(),ui->userCertOldFormatButton->isChecked());

	std::cerr << "TextPage() getting Invite: " << invite << std::endl;

	ui->userCertEdit->setPlainText(QString::fromUtf8(invite.c_str()));
}

void ConnectFriendWizard::toggleFormatState(bool doUpdate)
{
	if (ui->userCertOldFormatButton->isChecked()) 
	{
		ui->userCertOldFormatButton->setToolTip(tr("Use new certificate format (safer, more robust)"));
		ui->userCertOldFormatButton->setIcon(QIcon(":/images/ledoff1.png")) ;
	}
	else 
	{
		ui->userCertOldFormatButton->setToolTip(tr("Use old (backward compatible) certificate format"));
		ui->userCertOldFormatButton->setIcon(QIcon(":/images/ledon1.png")) ;
	}

	if (doUpdate) {
		updateOwnCert();
	}
}

void ConnectFriendWizard::toggleSignatureState(bool doUpdate)
{
	if (ui->userCertIncludeSignaturesButton->isChecked()) {
		ui->userCertIncludeSignaturesButton->setToolTip(tr("Remove signatures"));
	} else {
		ui->userCertIncludeSignaturesButton->setToolTip(tr("Include signatures"));
	}

	if (doUpdate) {
		updateOwnCert();
	}
}

void ConnectFriendWizard::runEmailClient()
{
	sendMail("", tr("RetroShare Invite"), ui->userCertEdit->toPlainText());
}

void ConnectFriendWizard::friendCertChanged()
{
	ui->TextPage->setComplete(false);
	cleanfriendCertTimer->start();
}

void ConnectFriendWizard::cleanFriendCert()
{
	bool certValid = false;
	QString errorMsg;
	std::string cert = ui->friendCertEdit->toPlainText().toUtf8().constData();

	if (cert.empty()) {
		ui->friendCertCleanLabel->setPixmap(QPixmap(":/images/delete.png"));
		ui->friendCertCleanLabel->setToolTip("");
	} else {
		std::string cleanCert;
		int error_code;

		if (rsPeers->cleanCertificate(cert, cleanCert, error_code)) {
			certValid = true;
			if (cert != cleanCert) {
				disconnect(ui->friendCertEdit, SIGNAL(textChanged()), this, SLOT(friendCertChanged()));
				QTextCursor textCursor = ui->friendCertEdit->textCursor();
				ui->friendCertEdit->setPlainText(QString::fromUtf8(cleanCert.c_str()));
				ui->friendCertEdit->setTextCursor(textCursor);
				connect(ui->friendCertEdit, SIGNAL(textChanged()), this, SLOT(friendCertChanged()));
			}
		} else {
			if (error_code > 0) {
				switch (error_code) {
				case RS_PEER_CERT_CLEANING_CODE_NO_BEGIN_TAG:
					errorMsg = tr("No or misspelled BEGIN tag found") ;
					break ;
				case RS_PEER_CERT_CLEANING_CODE_NO_END_TAG:
					errorMsg = tr("No or misspelled END tag found") ;
					break ;
				case RS_PEER_CERT_CLEANING_CODE_NO_CHECKSUM:
					errorMsg = tr("No checksum found (the last 5 chars should be separated by a '=' char), or no newline after tag line (e.g. line beginning with Version:)") ;
					break ;
				default:
					errorMsg = tr("Unknown error. Your cert is probably not even a certificate.") ;
				}
			}
		}
	}

	ui->friendCertCleanLabel->setPixmap(certValid ? QPixmap(":/images/accepted16.png") : QPixmap(":/images/delete.png"));
	ui->friendCertCleanLabel->setToolTip(errorMsg);

	ui->TextPage->setComplete(certValid);
}

void ConnectFriendWizard::showHelpUserCert()
{
	QMessageBox::information(this, tr("Connect Friend Help"), tr("You can copy this text and send it to your friend via email or some other way"));
}

void ConnectFriendWizard::copyCert()
{
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(ui->userCertEdit->toPlainText());
	QMessageBox::information(this, "RetroShare", tr("Your Cert is copied to Clipboard, paste and send it to your friend via email or some other way"));
}

void ConnectFriendWizard::saveCert()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save as..."), "", tr("RetroShare Certificate (*.rsc );;All Files (*)"));
	if (fileName.isEmpty())
		return;

	QFile file(fileName);
	if (!file.open(QFile::WriteOnly))
		return;

	//Todo: move save to file to p3Peers::SaveCertificateToFile

	QTextStream ts(&file);
	ts.setCodec(QTextCodec::codecForName("UTF-8"));
	ts << ui->userCertEdit->document()->toPlainText();
}

//========================== CertificatePage =================================

void ConnectFriendWizard::loadFriendCert()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Select Certificate"), "", tr("RetroShare Certificate (*.rsc );;All Files (*)"));

	if (!fileName.isNull()) {
		ui->friendFileNameEdit->setText(fileName);
	}
}

void ConnectFriendWizard::generateCertificateCalled()
{
#ifdef FRIEND_WIZARD_DEBUG
	std::cerr << "  generateCertificateCalled" << std::endl;
#endif

	std::string cert = rsPeers->GetRetroshareInvite(false);
	if (cert.empty()) {
		QMessageBox::information(this, "RetroShare", tr("Sorry, create certificate failed"), QMessageBox::Ok, QMessageBox::Ok);
		return;
	}

	QString qdir = QFileDialog::getSaveFileName(this, tr("Please choose a filename"), QDir::homePath(), tr("RetroShare Certificate (*.rsc );;All Files (*)"));

	//Todo: move save to file to p3Peers::SaveCertificateToFile

	if (qdir.isEmpty() == false) {
		QFile CertFile(qdir);
		if (CertFile.open(QIODevice::WriteOnly/* | QIODevice::Text*/)) {
			if (CertFile.write(QByteArray(cert.c_str())) > 0) {
				QMessageBox::information(this, "RetroShare", tr("Certificate file successfully created"), QMessageBox::Ok, QMessageBox::Ok);
			} else {
				QMessageBox::information(this, "RetroShare", tr("Sorry, certificate file creation failed"), QMessageBox::Ok, QMessageBox::Ok);
			}
			CertFile.close();
		} else {
			QMessageBox::information(this, "RetroShare", tr("Sorry, certificate file creation failed"), QMessageBox::Ok, QMessageBox::Ok);
		}
	}
}

//============================= FofPage ======================================

void ConnectFriendWizard::updatePeersList(int index)
{
	rsiface->unlockData(); /* UnLock Interface */

	ui->selectedPeersTW->clearContents();
	ui->selectedPeersTW->setRowCount(0);

	std::string ownId = rsPeers->getGPGOwnId();

	int row = 0;

	_id_boxes.clear();

	// We have to use this trick because signers are given by their names instead of their ids. That's a cause
	// for some confusion when two peers have the same name.
	//
	std::list<std::string> gpg_ids;
	rsPeers->getGPGAllList(gpg_ids);
	for (std::list<std::string>::const_iterator it(gpg_ids.begin()); it != gpg_ids.end(); ++it) {
		if (*it == ownId) {
			// its me
			continue;
		}

#ifdef FRIEND_WIZARD_DEBUG
		std::cerr << "examining peer " << *it << " (name=" << rsPeers->getPeerName(*it);
#endif

		RsPeerDetails details ;
		if (!rsPeers->getPeerDetails(*it,details)) {
#ifdef FRIEND_WIZARD_DEBUG
			std::cerr << " no details." << std::endl ;
#endif
			continue;
		}

		// determine common friends

		std::list<std::string> common_friends;

		for (std::list<std::string>::const_iterator it2(details.gpgSigners.begin()); it2 != details.gpgSigners.end(); ++it2) {
			if(rsPeers->isGPGAccepted(*it2)) {
				common_friends.push_back(*it2);
			}
		}
		bool show = false;

		switch(index) {
		case 0: // "All unsigned friends of my friends"
			show = !details.ownsign;
#ifdef FRIEND_WIZARD_DEBUG
			std::cerr << "case 0: ownsign=" << details.ownsign << ", show=" << show << std::endl;
#endif
			break ;
		case 1: // "Unsigned peers who already signed my certificate"
			show = details.hasSignedMe && !(details.state & RS_PEER_STATE_FRIEND);
#ifdef FRIEND_WIZARD_DEBUG
			std::cerr << "case 1, ownsign=" << details.ownsign << ", is_authed_me=" << details.hasSignedMe << ", show=" << show << std::endl;
#endif
			break ;
		case 2: // "Peers shown as denied"
			show = details.ownsign && !(details.state & RS_PEER_STATE_FRIEND);
#ifdef FRIEND_WIZARD_DEBUG
			std::cerr << "case 2, ownsign=" << details.ownsign << ", state_friend=" << (details.state & RS_PEER_STATE_FRIEND) << ", show=" << show << std::endl;
#endif
			break ;
		}

		if (show) {
			ui->selectedPeersTW->insertRow(row);

			QCheckBox *cb = new QCheckBox;
			cb->setChecked(true);
			_id_boxes[cb] = details.id;
			_gpg_id_boxes[cb] = details.gpg_id;

			ui->selectedPeersTW->setCellWidget(row, 0, cb);
			ui->selectedPeersTW->setItem(row, 1, new QTableWidgetItem(QString::fromUtf8(details.name.c_str())));

			QComboBox *qcb = new QComboBox;

			if (common_friends.empty()) {
				qcb->addItem(tr("*** None ***"));
			} else {
				for (std::list<std::string>::const_iterator it2(common_friends.begin()); it2 != common_friends.end(); ++it2) {
					qcb->addItem(QString::fromStdString(*it2));
				}
			}

			ui->selectedPeersTW->setCellWidget(row, 2, qcb);
			ui->selectedPeersTW->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(details.id)));
			++row;
		}
	}
#ifdef FRIEND_WIZARD_DEBUG
	std::cerr << "FofPage::updatePeersList() finished iterating over peers" << std::endl;
#endif

	if (row>0) {
		ui->selectedPeersTW->resizeColumnsToContents();
		ui->makeFriendButton->setEnabled(true);
	} else {
		ui->makeFriendButton->setEnabled(false);
	}
}

void ConnectFriendWizard::signAllSelectedUsers()
{
#ifdef FRIEND_WIZARD_DEBUG
	std::cerr << "making lots of friends !!" << std::endl;
#endif

	for (std::map<QCheckBox*, std::string>::const_iterator it(_id_boxes.begin()); it != _id_boxes.end(); ++it) {
		if (it->first->isChecked()) {
#ifdef FRIEND_WIZARD_DEBUG
			std::cerr << "Making friend with " << it->second << std::endl ;
#endif
			//rsPeers->AuthCertificate(it->second, "");
			rsPeers->addFriend(it->second, _gpg_id_boxes[it->first]);
		}
	}

	ui->FofPage->setComplete(true);

	ui->userSelectionCB->setEnabled(false);
	ui->selectedPeersTW->setEnabled(false);
	ui->makeFriendButton->setEnabled(false);

	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_NEIGHBOURS,0);
	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_FRIENDS,0);
}

//============================= RsidPage =====================================

//============================ Emailpage =====================================

//========================= ErrorMessagePage =================================

//========================== ConclusionPage ==================================

void ConnectFriendWizard::setGroup(const std::string &id)
{
	groupId = QString::fromStdString(id);
}

void ConnectFriendWizard::groupCurrentIndexChanged(int index)
{
	QComboBox *comboBox = dynamic_cast<QComboBox*>(sender());
	if (comboBox) {
		groupId = comboBox->itemData(index, Qt::UserRole).toString();
	}
}
