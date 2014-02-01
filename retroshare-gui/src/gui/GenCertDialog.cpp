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
#include <QGraphicsOpacityEffect>
#include <QTimer>
#include <QMessageBox>
#include <QMouseEvent>
#include <QTextBrowser>
#include <QProgressBar>
#include <time.h>
#include <math.h>
#include <iostream>

class EntropyCollectorWidget: public QTextBrowser
{
	public:
		EntropyCollectorWidget(QProgressBar *pr,QWidget *p = NULL)
			: QTextBrowser(p) 
		{
			progress = pr ;
			setMouseTracking(true) ;
			entropy_values_collected = 0 ;
		}
		virtual void mouseMoveEvent(QMouseEvent *e)
		{
			std::cerr << "Mouse moved: " << e->x() << ", " << e->y() << std::endl;
			++entropy_values_collected ;

			progress->setValue(entropy_values_collected*100 / 4096) ;
		}

		int entropy_values_collected ;
		QProgressBar *progress ;
};

class MyFilter: public QObject
{
	public:
		virtual bool eventFilter(QObject *obj, QEvent *event)
		{
			if(event->type() == QEvent::MouseMove)
				std::cerr << "Mouse moved !"<< std::endl;

			return QObject::eventFilter(obj,event) ;
		}
};

void GenCertDialog::grabMouse()
{
	static int last_x = 0 ;
	static int last_y = 0 ;
	static uint32_t count = 0 ;

	uint32_t x = QCursor::pos().x() ;
	uint32_t y = QCursor::pos().y() ;

	if(last_x == x && last_y == y)
		return ;

	last_x = x ;
	last_y = y ;

	// Let's do some shuffle with mouse coordinates. Does not need to be cryptographically random,
	// since the random number generator will shuffle this appropriately in openssl.
	//
	uint32_t E = ((count*x*86243 + y*y*15641) & 0xffff) ^ 0xb374;
	uint32_t F = ((x*44497*y*count + x*x) & 0xffff) ^ 0x395b ;

	++count ;

//	std::cerr << "Mouse grabed at " << x << " " << y << ". Adding entropy E=" << std::hex << E << ", F=" << F << ", digit =" << E + (F << 16) << std::dec << std::endl;

	ui.entropy_bar->setValue(count*100/2048) ;

	if(ui.entropy_bar->value() < 20)
		ui.genButton->setEnabled(false) ;
	else
		ui.genButton->setEnabled(true) ;

	RsInit::collectEntropy(E+(F << 16)) ;
}
//static bool MyEventFilter(void *message, long *result)
//{
//	std::cerr << "Event called " << message << std::endl;
//	return false ;
//}
/** Default constructor */
GenCertDialog::GenCertDialog(bool onlyGenerateIdentity, QWidget *parent)
	: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint), mOnlyGenerateIdentity(onlyGenerateIdentity)
{
	/* Invoke Qt Designer generated QObject setup routine */
	ui.setupUi(this);
	
	ui.headerFrame->setHeaderImage(QPixmap(":/images/contact_new128.png"));
	ui.headerFrame->setHeaderText(tr("Create a new Identity"));

	connect(ui.new_gpg_key_checkbox, SIGNAL(clicked()), this, SLOT(newGPGKeyGenUiSetup()));
	connect(ui.hidden_checkbox, SIGNAL(clicked()), this, SLOT(hiddenUiSetup()));

	connect(ui.genButton, SIGNAL(clicked()), this, SLOT(genPerson()));
	connect(ui.importIdentity_PB, SIGNAL(clicked()), this, SLOT(importIdentity()));
	connect(ui.exportIdentity_PB, SIGNAL(clicked()), this, SLOT(exportIdentity()));

	//ui.genName->setFocus(Qt::OtherFocusReason);

//	QObject *obj = QCoreApplication::eventFilter() ;
//	std::cerr << "Event filter : " << obj << std::endl;
//	QCoreApplication::instance()->setEventFilter(MyEventFilter) ;

	entropy_timer = new QTimer ;
	entropy_timer->start(20) ;
	QObject::connect(entropy_timer,SIGNAL(timeout()),this,SLOT(grabMouse())) ;

//	EntropyCollectorWidget *ecw = new EntropyCollectorWidget(ui.entropy_bar,this) ;
//	ecw->resize(size()) ;
//	ecw->move(0,0) ;
//
//	QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect ;
//	effect->setOpacity(0.2) ;
//	ecw->setGraphicsEffect(effect) ;
	//ecw->setBackgroundColor(QColor::fromRGB(1,1,1)) ;
//	ecw->show() ;

	ui.entropy_bar->setValue(0) ;

#if QT_VERSION >= 0x040700
	ui.email_input->setPlaceholderText(tr("[Optional] Visible to your friends, and friends of friends.")) ;
	ui.location_input->setPlaceholderText(tr("[Required] Examples: Home, Laptop,...")) ;
	ui.name_input->setPlaceholderText(tr("[Required] Visible to your friends, and friends of friends."));
	ui.password_input->setPlaceholderText(tr("[Required] This password protects your PGP key."));
	ui.password_input_2->setPlaceholderText(tr("[Required] Type the same password again here."));
#endif

	ui.location_input->setToolTip(tr("Put a meaningful location. ex : home, laptop, etc. \nThis field will be used to differentiate different installations with\nthe same identity (PGP key).")) ;

	ui.email_input->hide() ;
	ui.email_label->hide() ;

	/* get all available pgp private certificates....
	 * mark last one as default.
	 */

	init();
}

GenCertDialog::~GenCertDialog()
{
	entropy_timer->stop() ;
}

void GenCertDialog::init()
{
	std::cerr << "Finding PGPUsers" << std::endl;

	ui.genPGPuser->clear() ;

	std::list<std::string> pgpIds;
	std::list<std::string>::iterator it;
	bool foundGPGKeys = false;
	if (!mOnlyGenerateIdentity) {
		if (RsAccounts::GetPGPLogins(pgpIds)) {
			for(it = pgpIds.begin(); it != pgpIds.end(); it++)
			{
				QVariant userData(QString::fromStdString(*it));
				std::string name, email;
				RsAccounts::GetPGPLoginDetails(*it, name, email);
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
		ui.headerFrame->setHeaderText(tr("Create a new Location"));
		genNewGPGKey = false;
	} else {
		ui.no_gpg_key_label->setVisible(!mOnlyGenerateIdentity);
		ui.new_gpg_key_checkbox->setChecked(true);
		ui.new_gpg_key_checkbox->setEnabled(true);
		setWindowTitle(tr("Create new Identity"));
		ui.genButton->setText(tr("Generate new Identity"));
		ui.headerFrame->setHeaderText(tr("Create a new Identity"));
		genNewGPGKey = true;
	}

	QString text = ui.header_label->text() + "\n";

	if (mOnlyGenerateIdentity) {
		ui.new_gpg_key_checkbox->setChecked(true);
		ui.new_gpg_key_checkbox->hide();
		ui.genprofileinfo_label->hide();
		text += tr("You can create a new identity with this form.");
	} else {
		text += tr("You can use an existing identity (i.e. a PGP key pair), from the list below, or create a new one with this form.");
	}
	ui.header_label->setText(text);

	newGPGKeyGenUiSetup();
	hiddenUiSetup();
}

void GenCertDialog::mouseMoveEvent(QMouseEvent *e)
{
	std::cerr << "Mouse : " << e->x() << ", " << e->y() << std::endl;

	QDialog::mouseMoveEvent(e) ;
}

void GenCertDialog::newGPGKeyGenUiSetup() {

	if (ui.new_gpg_key_checkbox->isChecked()) {
		genNewGPGKey = true;
		ui.name_label->show();
		ui.name_input->show();
//		ui.email_label->show();
//		ui.email_input->show();
		ui.password_label->show();
		ui.password_label_2->show();
		ui.password_input->show();
		ui.password_input_2->show();
		ui.genPGPuserlabel->hide();
		ui.genPGPuser->hide();
		ui.importIdentity_PB->hide() ;
		ui.exportIdentity_PB->hide();
		setWindowTitle(tr("Create new Identity"));
		ui.genButton->setText(tr("Generate new Identity"));
		ui.headerFrame->setHeaderText(tr("Create a new Identity"));
		ui.genprofileinfo_label->hide();
		ui.header_label->show();
	} else {
		genNewGPGKey = false;
		ui.name_label->hide();
		ui.name_input->hide();
//		ui.email_label->hide();
//		ui.email_input->hide();
		ui.password_label->hide();
		ui.password_label_2->hide();
		ui.password_input->hide();
		ui.password_input_2->hide();
		ui.genPGPuserlabel->show();
		ui.genPGPuser->show();
		ui.importIdentity_PB->setVisible(!mOnlyGenerateIdentity);
		ui.exportIdentity_PB->setVisible(!mOnlyGenerateIdentity);
		ui.exportIdentity_PB->setEnabled(ui.genPGPuser->count() != 0);
		setWindowTitle(tr("Create new Location"));
		ui.genButton->setText(tr("Generate new Location"));
		ui.headerFrame->setHeaderText(tr("Create a new Location"));
		ui.genprofileinfo_label->show();
		ui.header_label->hide();
	}
}


void GenCertDialog::hiddenUiSetup() 
{

	if (ui.hidden_checkbox->isChecked()) 
	{
		ui.hiddenaddr_input->show();
		ui.hiddenaddr_label->show();
		ui.label_hiddenaddr2->show();
		ui.hiddenport_label->show();
		ui.hiddenport_spinBox->show();
	} 
	else 
	{
		ui.hiddenaddr_input->hide();
		ui.hiddenaddr_label->hide();
		ui.label_hiddenaddr2->hide();
		ui.hiddenport_label->hide();
		ui.hiddenport_spinBox->hide();
	}
}

void GenCertDialog::exportIdentity()
{
	QString fname = QFileDialog::getSaveFileName(this,tr("Export Identity"), "",tr("RetroShare Identity files (*.asc)")) ;

	if(fname.isNull())
		return ;

	QVariant data = ui.genPGPuser->itemData(ui.genPGPuser->currentIndex());
	std::string gpg_id = data.toString().toStdString() ;

	if(RsAccounts::ExportIdentity(fname.toStdString(),gpg_id))
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

	if(!RsAccounts::ImportIdentity(fname.toStdString(),gpg_id,err_string))
	{
		QMessageBox::information(this,tr("Identity not loaded"),tr("Your identity was not loaded properly:")+" \n    "+QString::fromStdString(err_string)) ;
		return ;
	}
	else
	{
		std::string name,email ;

		RsAccounts::GetPGPLoginDetails(gpg_id, name, email);
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
	bool isHiddenLoc = false;

	if (ui.hidden_checkbox->isChecked()) 
	{
		std::string hl = ui.hiddenaddr_input->text().toStdString();
		uint16_t port  = ui.hiddenport_spinBox->value();
		if (!RsInit::SetHiddenLocation(hl, port))	/* parses it */
		{
			/* Message Dialog */
			QMessageBox::warning(this,
				tr("Invalid Hidden Location"),
			tr("Please put in a valid address of the form: 31769173498.onion:7800"),
			QMessageBox::Ok);
			return;
		}
		isHiddenLoc = true;
	}

	if (!genNewGPGKey) {
		if (genLoc.length() < 3) {
			/* Message Dialog */
			QMessageBox::warning(this,
								 tr("Generate PGP key Failure"),
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
		if (ui.password_input->text().length() < 3 || ui.name_input->text().length() < 3 || genLoc.length() < 3) {
			/* Message Dialog */
			QMessageBox::warning(this,
								 tr("Generate PGP key Failure"),
								 tr("All fields are required with a minimum of 3 characters"),
								 QMessageBox::Ok);
			return;
		}

		if(ui.password_input->text() != ui.password_input_2->text())
		{
			QMessageBox::warning(this,
								 tr("Generate PGP key Failure"),
								 tr("Passwords do not match"),
								 QMessageBox::Ok);
			return;
		}
		//generate a new gpg key
		std::string err_string;
		ui.no_gpg_key_label->setText(tr("Generating new PGP key, please be patient: this process needs generating large prime numbers, and can take some minutes on slow computers. \n\nFill in your PGP password when asked, to sign your new key."));
		ui.no_gpg_key_label->show();
		ui.new_gpg_key_checkbox->hide();
		ui.name_label->hide();
		ui.name_input->hide();
//		ui.email_label->hide();
//		ui.email_input->hide();
		ui.password_label_2->hide();
		ui.password_input_2->hide();
		ui.password_label->hide();
		ui.password_input->hide();
		ui.genPGPuserlabel->hide();
		ui.genPGPuser->hide();
		ui.location_label->hide();
		ui.location_input->hide();
		ui.genButton->hide();
		ui.importIdentity_PB->hide();
		ui.genprofileinfo_label->hide();

		setCursor(Qt::WaitCursor) ;

		QCoreApplication::processEvents();
		while(QAbstractEventDispatcher::instance()->processEvents(QEventLoop::AllEvents)) ;

		std::string email_str = "" ;
		RsAccounts::GeneratePGPCertificate(ui.name_input->text().toUtf8().constData(), email_str.c_str(), ui.password_input->text().toUtf8().constData(), PGPId, err_string);

		setCursor(Qt::ArrowCursor) ;
	}

	//generate a random ssl password
	std::string sslPasswd = RSRandom::random_alphaNumericString(RsInit::getSslPwdLen()) ;

	/* GenerateSSLCertificate - selects the PGP Account */
	//RsInit::SelectGPGAccount(PGPId);

	std::string sslId;
	std::cerr << "GenCertDialog::genPerson() Generating SSL cert with gpg id : " << PGPId << std::endl;
	std::string err;
	bool okGen = RsAccounts::GenerateSSLCertificate(PGPId, "", genLoc, "", isHiddenLoc, sslPasswd, sslId, err);

	if (okGen)
	{
		/* complete the process */
		RsInit::LoadPassword(sslPasswd);
		if (Rshare::loadCertificate(sslId, false)) {
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
