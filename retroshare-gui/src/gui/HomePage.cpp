/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2016, defnax
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

#include "HomePage.h"
#include "ui_HomePage.h"

#include "gui/notifyqt.h"
#include "gui/msgs/MessageComposer.h"
#include "gui/connect/ConnectFriendWizard.h"
#include <gui/QuickStartWizard.h>
#include "gui/connect/FriendRecommendDialog.h"

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#include <QUrlQuery>
#endif

#include <iostream>
#include <string>
#include <QTime>
#include <QMenu>
#include <QDesktopServices>
#include <QMessageBox>
#include <QFileDialog>
#include <QClipboard>
#include <QTextStream>
#include <QTextCodec>

HomePage::HomePage(QWidget *parent) :
	MainPage(parent),
	ui(new Ui::HomePage)
{
    ui->setupUi(this);

		updateOwnCert();
		
	connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addFriend()));
		
    QAction *CopyAction = new QAction(QIcon(),tr("Copy your Cert to Clipboard"), this);
    connect(CopyAction, SIGNAL(triggered()), this, SLOT(copyCert()));
	
    QAction *SaveAction = new QAction(QIcon(),tr("Save your Cert into a File"), this);
    connect(SaveAction, SIGNAL(triggered()), this, SLOT(saveCert()));
    
    QAction *SendAction = new QAction(QIcon(),tr("Send via Email"), this);
    connect(SendAction, SIGNAL(triggered()), this, SLOT(runEmailClient()));
		
    QAction *RecAction = new QAction(QIcon(),tr("Recommend friends to each others"), this);
    connect(RecAction, SIGNAL(triggered()), this, SLOT(recommendFriends()));

		QMenu *menu = new QMenu();
    menu->addAction(CopyAction);
    menu->addAction(SaveAction);
    menu->addAction(SendAction);
    menu->addAction(RecAction);

    ui->shareButton->setMenu(menu);
	
	connect(ui->runStartWizard_PB,SIGNAL(clicked()), this,SLOT(runStartWizard())) ;
}

HomePage::~HomePage()
{
	delete ui;
}

void HomePage::updateOwnCert()
{
	std::string invite = rsPeers->GetRetroshareInvite(false);

	ui->userCertEdit->setPlainText(QString::fromUtf8(invite.c_str()));
}

static void sendMail(QString sAddress, QString sSubject, QString sBody)
{
#ifdef Q_OS_WIN
	/* search and replace the end of lines with: "%0D%0A" */
	sBody.replace("\n", "%0D%0A");
#endif

	QUrl url = QUrl("mailto:" + sAddress);

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	QUrlQuery urlQuery;
#else
	QUrl &urlQuery(url);
#endif

	urlQuery.addQueryItem("subject", sSubject);
	urlQuery.addQueryItem("body", sBody);

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	url.setQuery(urlQuery);
#endif

	std::cerr << "MAIL STRING:" << (std::string)url.toEncoded().constData() << std::endl;

	/* pass the url directly to QDesktopServices::openUrl */
	QDesktopServices::openUrl (url);
}

void HomePage::recommendFriends()
{
    FriendRecommendDialog::showIt() ;
}

void HomePage::runEmailClient()
{
	sendMail("", tr("RetroShare Invite"), ui->userCertEdit->toPlainText());
}

void HomePage::copyCert()
{
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(ui->userCertEdit->toPlainText());
	QMessageBox::information(this, "RetroShare", tr("Your Cert is copied to Clipboard, paste and send it to your friend via email or some other way"));
}

void HomePage::saveCert()
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

/** Add a Friends Text Certificate */
void HomePage::addFriend()
{
    ConnectFriendWizard connwiz (this);

    connwiz.setStartId(ConnectFriendWizard::Page_Text);
    connwiz.exec ();
}

void HomePage::runStartWizard()
{
    QuickStartWizard(this).exec();
}
