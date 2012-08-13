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

void ConnectFriendWizard::setCertificate(const QString &certificate)
{
	if (certificate.isEmpty()) {
		setStartId(Page_Intro);
		return;
	}

	std::string error_string;

	if (rsPeers->loadDetailsFromStringCert(certificate.toUtf8().constData(), peerDetails, error_string)) 
	{
#ifdef FRIEND_WIZARD_DEBUG
		std::cerr << "ConnectFriendWizard got id : " << peerDetails.id << "; gpg_id : " << peerDetails.gpg_id << std::endl;
#endif
		setStartId(Page_Conclusion);
	} else {
		// error message
		setField("errorMessage", tr("Certificate Load Failed") + ": " + QString::fromUtf8(error_string.c_str()));
		setStartId(Page_ErrorMessage);
	}
}

ConnectFriendWizard::~ConnectFriendWizard()
{
	delete ui;
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
		connect(ui->friendCertCleanButton, SIGNAL(clicked()), this, SLOT(cleanFriendCert()));

		ui->TextPage->registerField("friendCert*", ui->friendCertEdit, "plainText", SIGNAL(textChanged()));

		toggleSignatureState(); // updateOwnCert
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
				ts += QString("%1<%2>\n").arg(QString::fromUtf8(rsPeers->getPeerName(*it).c_str()), QString::fromStdString(*it));
			}

			ui->nameEdit->setText(QString::fromUtf8(peerDetails.name.c_str()));
			ui->trustEdit->setText(trustString);
			ui->emailEdit->setText(QString::fromUtf8(peerDetails.email.c_str()));
			ui->locationEdit->setText(QString::fromUtf8(peerDetails.location.c_str()));
			ui->signersEdit->setPlainText(ts);

			std::list<RsGroupInfo> groupInfoList;
			rsPeers->getGroupInfoList(groupInfoList);
			GroupDefs::sortByName(groupInfoList);
			ui->groupComboBox->addItem("", ""); // empty value
			for (std::list<RsGroupInfo>::iterator groupIt = groupInfoList.begin(); groupIt != groupInfoList.end(); ++groupIt) {
				ui->groupComboBox->addItem(GroupDefs::name(*groupIt), QString::fromStdString(groupIt->id));
			}

			if (groupId.isEmpty() == false) {
				ui->groupComboBox->setCurrentIndex(ui->groupComboBox->findData(groupId));
			}
			connect(ui->groupComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(groupCurrentIndexChanged(int)));
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
			cleanFriendCert() ;

			std::string certstr = ui->friendCertEdit->toPlainText().toUtf8().constData();
			std::string error_string;

			if (rsPeers->loadDetailsFromStringCert(certstr, peerDetails, error_string)) {
#ifdef FRIEND_WIZARD_DEBUG
				std::cerr << "ConnectFriendWizard got id : " << peerDetails.id << "; gpg_id : " << peerDetails.gpg_id << std::endl;
#endif
				break;
			}
			// error message
			setField("errorMessage", tr("Certificate Load Failed") + ": " + QString::fromUtf8(error_string.c_str()));
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

				std::string error_string;
				if (rsPeers->loadDetailsFromStringCert(certstr, peerDetails, error_string)) {
#ifdef FRIEND_WIZARD_DEBUG
					std::cerr << "ConnectFriendWizard got id : " << peerDetails.id << "; gpg_id : " << peerDetails.gpg_id << std::endl;
#endif
				} else {
					setField("errorMessage", QString(tr("Certificate Load Failed:something is wrong with %1 ")).arg(fn) + ": " + QString::fromUtf8(error_string.c_str()));
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
		return -1;
	}

	return -1;
}

void ConnectFriendWizard::accept()
{
	if (hasVisitedPage(Page_Conclusion)) {
		std::cerr << "ConnectFriendWizard::accept() called with page conclusion visited" << std::endl;

		bool sign = ui->signGPGCheckBox->isChecked();
		bool accept_connection = ui->acceptNoSignGPGCheckBox->isChecked();

		if(accept_connection || sign)
		{
			std::string certstr = ui->friendCertEdit->toPlainText().toUtf8().constData();

			std::string ssl_id, pgp_id ;

			if(!rsPeers->loadCertificateFromString(certstr,ssl_id,pgp_id)) 
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
				rsPeers->addFriend("", peerDetails.gpg_id);
			}

			if (!groupId.isEmpty()) {
				rsPeers->assignPeerToGroup(groupId.toStdString(), peerDetails.gpg_id, true);
			}
		}

		if (peerDetails.id != "") {
			rsPeers->addFriend(peerDetails.id, peerDetails.gpg_id);
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
	}

	QDialog::accept();
}

//============================= TextPage =====================================

void ConnectFriendWizard::updateOwnCert()
{
	std::string invite = rsPeers->GetRetroshareInvite(ui->userCertIncludeSignaturesButton->isChecked(),ui->userCertOldFormatButton->isChecked());

	std::cerr << "TextPage() getting Invite: " << invite << std::endl;

	ui->userCertEdit->setPlainText(QString::fromUtf8(invite.c_str()));
}
void ConnectFriendWizard::toggleFormatState()
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

	updateOwnCert();
}
void ConnectFriendWizard::toggleSignatureState()
{
	if (ui->userCertIncludeSignaturesButton->isChecked()) {
		ui->userCertIncludeSignaturesButton->setToolTip(tr("Remove signatures"));
	} else {
		ui->userCertIncludeSignaturesButton->setToolTip(tr("Include signatures"));
	}

	updateOwnCert();
}

void ConnectFriendWizard::runEmailClient()
{
	sendMail("", tr("RetroShare Invite"), ui->userCertEdit->toPlainText());
}

void ConnectFriendWizard::cleanFriendCert()
{
	std::string cert = ui->friendCertEdit->toPlainText().toUtf8().constData();
	cert += "\n";	// add an end of line to avoid a bug
	std::string cleanCert;
	int error_code;

	if (rsPeers->cleanCertificate(cert, cleanCert, error_code)) {
		ui->friendCertEdit->setPlainText(QString::fromStdString(cleanCert));

		if (error_code > 0) {
			QString msg;

			switch (error_code) {
			case RS_PEER_CERT_CLEANING_CODE_NO_BEGIN_TAG:
				msg = tr("No or misspelled BEGIN tag found") ;
				break ;
			case RS_PEER_CERT_CLEANING_CODE_NO_END_TAG:
				msg = tr("No or misspelled END tag found") ;
				break ;
			case RS_PEER_CERT_CLEANING_CODE_NO_CHECKSUM:
				msg = tr("No checksum found (the last 5 chars should be separated by a '=' char), or no newline after tag line (e.g. line beginning with Version:)") ;
				break ;
			default:
				msg = tr("Unknown error. Your cert is probably not even a certificate.") ;
			}
			QMessageBox::information(NULL, tr("Certificate cleaning error"), msg) ;
		}


	}
}

void ConnectFriendWizard::showHelpUserCert()
{
	QMessageBox::information(this, tr("Connect Friend Help"), tr("You can copy this text and send it to your friend via email or some other way"));
}

void ConnectFriendWizard::copyCert()
{
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(ui->userCertEdit->toPlainText());
	QMessageBox::information(this, "RetroShare", tr("Your Cert is copied to Clipboard, paste and send it to your riend via email or some other way"));
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
	groupId = ui->groupComboBox->itemData(index, Qt::UserRole).toString();
}
