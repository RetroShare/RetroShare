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

#include <rshare.h>
#include <util/rsrandom.h>
#include <retroshare/rsinit.h>
#include "GenCertDialog.h"
#include <QAbstractEventDispatcher>
#include <QFileDialog>
#include <QMessageBox>
#include <time.h>
#include <iostream>

/* Define the format used for displaying the date and time */
#define DATETIME_FMT  "MMM dd hh:mm:ss"

/** Default constructor */
GenCertDialog::GenCertDialog(bool onlyGenerateIdentity, QWidget *parent)
	: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint), mOnlyGenerateIdentity(onlyGenerateIdentity)
{
	/* Invoke Qt Designer generated QObject setup routine */
	ui.setupUi(this);

	connect(ui.new_gpg_key_checkbox, SIGNAL(clicked()), this, SLOT(newGPGKeyGenUiSetup()));

	connect(ui.genButton, SIGNAL(clicked()), this, SLOT(genPerson()));
	connect(ui.importIdentity_PB, SIGNAL(clicked()), this, SLOT(importIdentity()));
	connect(ui.exportIdentity_PB, SIGNAL(clicked()), this, SLOT(exportIdentity()));

	//ui.genName->setFocus(Qt::OtherFocusReason);

#if QT_VERSION >= 0x040700
	ui.email_input->setPlaceholderText(tr("[Optional] Visible to your friends, and friends of friends.")) ;
	ui.location_input->setPlaceholderText(tr("[Required] Examples: Home, Laptop,...")) ;
	ui.name_input->setPlaceholderText(tr("[Required] Visible to your friends, and friends of friends."));
	ui.password_input->setPlaceholderText(tr("[Required] This password protects your PGP key."));
#endif
	/* get all available pgp private certificates....
	 * mark last one as default.
	 */

	init();
}

void GenCertDialog::init()
{
	std::cerr << "Finding PGPUsers" << std::endl;

	ui.genPGPuser->clear() ;

	std::list<std::string> pgpIds;
	std::list<std::string>::iterator it;
	bool foundGPGKeys = false;
	if (!mOnlyGenerateIdentity) {
		if (RsInit::GetPGPLogins(pgpIds)) {
			for(it = pgpIds.begin(); it != pgpIds.end(); it++)
			{
				QVariant userData(QString::fromStdString(*it));
				std::string name, email;
				RsInit::GetPGPLoginDetails(*it, name, email);
				std::cerr << "Adding PGPUser: " << name << " id: " << *it << std::endl;
				QString gid = QString::fromStdString(*it).right(8) ;
				ui.genPGPuser->addItem(QString::fromUtf8(name.c_str()) + " <" + QString::fromUtf8(email.c_str()) + "> (" + gid + ")", userData);
				foundGPGKeys = true;
			}
		}
	}

	if (foundGPGKeys) {
		ui.no_gpg_key_label->hide();
		ui.new_gpg_key_checkbox->setChecked(false);
		setWindowTitle(tr("Create new Location"));
		ui.genButton->setText(tr("Generate new Location"));
		ui.headerLabel->setText(tr("Create a new Location"));
		genNewGPGKey = false;
	} else {
		ui.no_gpg_key_label->setVisible(!mOnlyGenerateIdentity);
		ui.new_gpg_key_checkbox->setChecked(true);
		ui.new_gpg_key_checkbox->setEnabled(false);
		setWindowTitle(tr("Create new Identity"));
		ui.genButton->setText(tr("Generate new Identity"));
		ui.headerLabel->setText(tr("Create a new Identity"));
		genNewGPGKey = true;
	}

	QString text = ui.headerLabel2->text() + "\n";

	if (mOnlyGenerateIdentity) {
		ui.new_gpg_key_checkbox->setChecked(true);
		ui.new_gpg_key_checkbox->hide();
		ui.label->hide();
		text += tr("You can create a new identity with this form.");
	} else {
		text += tr("You can use an existing identity (i.e. a gpg key pair), from the list below, or create a new one with this form.");
	}
	ui.headerLabel2->setText(text);

	newGPGKeyGenUiSetup();
}

void GenCertDialog::newGPGKeyGenUiSetup() {

	if (ui.new_gpg_key_checkbox->isChecked()) {
		genNewGPGKey = true;
		ui.name_label->show();
		ui.name_input->show();
		ui.email_label->show();
		ui.email_input->show();
		ui.password_label->show();
		ui.password_input->show();
		ui.genPGPuserlabel->hide();
		ui.genPGPuser->hide();
		ui.importIdentity_PB->hide() ;
		ui.exportIdentity_PB->hide();
		setWindowTitle(tr("Create new Identity"));
		ui.genButton->setText(tr("Generate new Identity"));
		ui.headerLabel->setText(tr("Create a new Identity"));
	} else {
		genNewGPGKey = false;
		ui.name_label->hide();
		ui.name_input->hide();
		ui.email_label->hide();
		ui.email_input->hide();
		ui.password_label->hide();
		ui.password_input->hide();
		ui.genPGPuserlabel->show();
		ui.genPGPuser->show();
		ui.importIdentity_PB->setVisible(!mOnlyGenerateIdentity);
		ui.exportIdentity_PB->setVisible(!mOnlyGenerateIdentity);
		ui.exportIdentity_PB->setEnabled(ui.genPGPuser->count() != 0);
		setWindowTitle(tr("Create new Location"));
		ui.genButton->setText(tr("Generate new Location"));
		ui.headerLabel->setText(tr("Create a new Location"));
	}
}

void GenCertDialog::exportIdentity()
{
	QString fname = QFileDialog::getSaveFileName(this,tr("Export Identity"), "",tr("RetroShare Identity files (*.asc)")) ;

	if(fname.isNull())
		return ;

	QVariant data = ui.genPGPuser->itemData(ui.genPGPuser->currentIndex());
	std::string gpg_id = data.toString().toStdString() ;

	if(RsInit::exportIdentity(fname.toStdString(),gpg_id))
		QMessageBox::information(this,tr("Identity saved"),tr("Your identity was successfully saved\nIt is encrypted\n\nYou can now copy it to another computer\nand use the import button to load it")) ;
	else
		QMessageBox::information(this,tr("Identity not saved"),tr("Your identity was not saved. An error occurred.")) ;
}

void GenCertDialog::importIdentity()
{
	QString fname = QFileDialog::getOpenFileName(this,tr("Export Identity"), "",tr("RetroShare Identity files (*.asc)")) ;

	if(fname.isNull())
		return ;

	std::string gpg_id ;
	std::string err_string ;

	if(!RsInit::importIdentity(fname.toStdString(),gpg_id,err_string))
	{
		QMessageBox::information(this,tr("Identity not loaded"),tr("Your identity was not loaded properly:")+" \n    "+QString::fromStdString(err_string)) ;
		return ;
	}
	else
	{
		std::string name,email ;

		RsInit::GetPGPLoginDetails(gpg_id, name, email);
		std::cerr << "Adding PGPUser: " << name << " id: " << gpg_id << std::endl;

		QMessageBox::information(this,tr("New identity imported"),tr("Your identity was imported successfully:")+" \n"+"\nName :"+QString::fromStdString(name)+"\nemail: " + QString::fromStdString(email)+"\nKey ID: "+QString::fromStdString(gpg_id)+"\n\n"+tr("You can use it now to create a new location.")) ;
	}

	init() ;
}

void GenCertDialog::genPerson()
{
	/* Check the data from the GUI. */
	std::string genLoc  = ui.location_input->text().toUtf8().constData();
	std::string PGPId;

	if (!genNewGPGKey) {
		if (genLoc.length() < 3) {
			/* Message Dialog */
			QMessageBox::warning(this,
								 tr("Generate GPG key Failure"),
								 tr("Location field is required with a minimum of 3 characters"),
								 QMessageBox::Ok);
			return;
		}
		int pgpidx = ui.genPGPuser->currentIndex();
		if (pgpidx < 0)
		{
			/* Message Dialog */
			QMessageBox::warning(this,
								 "Generate ID Failure",
								 "Missing PGP Certificate",
								 QMessageBox::Ok);
			return;
		}
		QVariant data = ui.genPGPuser->itemData(pgpidx);
		PGPId = (data.toString()).toStdString();
	} else {
		if (ui.password_input->text().length() < 3 || ui.name_input->text().length() < 3 ||
			ui.email_input->text().length() < 3 || genLoc.length() < 3) {
			/* Message Dialog */
			QMessageBox::warning(this,
								 tr("Generate GPG key Failure"),
								 tr("All fields are required with a minimum of 3 characters"),
								 QMessageBox::Ok);
			return;
		}
		//generate a new gpg key
		std::string err_string;
		ui.no_gpg_key_label->setText(tr("Generating new GPG key, please be patient: this process needs generating large prime numbers, and can take some minutes on slow computers. \n\nFill in your GPG password when asked, to sign your new key."));
		ui.no_gpg_key_label->show();
		ui.new_gpg_key_checkbox->hide();
		ui.name_label->hide();
		ui.name_input->hide();
		ui.email_label->hide();
		ui.email_input->hide();
		ui.password_label->hide();
		ui.password_input->hide();
		ui.genPGPuserlabel->hide();
		ui.genPGPuser->hide();
		ui.location_label->hide();
		ui.location_input->hide();
		ui.genButton->hide();
		ui.label_location2->hide();
		ui.importIdentity_PB->hide();

		setCursor(Qt::WaitCursor) ;

		QCoreApplication::processEvents();
		while(QAbstractEventDispatcher::instance()->processEvents(QEventLoop::AllEvents)) ;

		RsInit::GeneratePGPCertificate(ui.name_input->text().toUtf8().constData(), ui.email_input->text().toUtf8().constData(), ui.password_input->text().toUtf8().constData(), PGPId, err_string);

		setCursor(Qt::ArrowCursor) ;
	}

	//generate a random ssl password
	std::string sslPasswd = RSRandom::random_alphaNumericString(RsInit::getSslPwdLen()) ;

	/* Initialise the PGP user first */
	RsInit::SelectGPGAccount(PGPId);

	std::string sslId;
	std::cerr << "GenCertDialog::genPerson() Generating SSL cert with gpg id : " << PGPId << std::endl;
	std::string err;
	bool okGen = RsInit::GenerateSSLCertificate(PGPId, "", genLoc, "", sslPasswd, sslId, err);

	if (okGen)
	{
		/* complete the process */
		RsInit::LoadPassword(sslId, sslPasswd);
		if (Rshare::loadCertificate(sslId, false, PGPId)) {
			accept();
		}
	}
	else
	{
		/* Message Dialog */
		QMessageBox::warning(this,
				tr("Generate ID Failure"),
				tr("Failed to Generate your new Certificate, maybe PGP password is wrong!"),
				QMessageBox::Ok);
	}
}
