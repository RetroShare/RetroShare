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
	connect(ui->LoadCertFileButton, SIGNAL(clicked()), this, SLOT(loadCert()));
	
    QAction *CopyAction = new QAction(QIcon(),tr("Copy your Cert to Clipboard"), this);
    connect(CopyAction, SIGNAL(triggered()), this, SLOT(copyCert()));
	
    QAction *SaveAction = new QAction(QIcon(),tr("Save your Cert into a File"), this);
    connect(SaveAction, SIGNAL(triggered()), this, SLOT(saveCert()));
    
    QAction *SendAction = new QAction(QIcon(),tr("Send via Email"), this);
    connect(SendAction, SIGNAL(triggered()), this, SLOT(runEmailClient()));

    QAction *WebMailAction = new QAction(QIcon(),tr("Invite via WebMail"), this);
    connect(WebMailAction, SIGNAL(triggered()), this, SLOT(webMail()));
	
    QAction *RecAction = new QAction(QIcon(),tr("Recommend friends to each others"), this);
    connect(RecAction, SIGNAL(triggered()), this, SLOT(recommendFriends()));

		QMenu *menu = new QMenu();
    menu->addAction(CopyAction);
    menu->addAction(SaveAction);
    menu->addAction(SendAction);
	menu->addAction(WebMailAction);
    menu->addAction(RecAction);

    ui->shareButton->setMenu(menu);

    connect(ui->runStartWizard_PB,SIGNAL(clicked()), this,SLOT(runStartWizard())) ;
	connect(ui->openwebhelp,SIGNAL(clicked()), this,SLOT(openWebHelp())) ;

	ui->runStartWizard_PB->hide(); // until future rework
	ui->LoadCertFileButton->hide(); // duplicates functionality => not good.

    int S = QFontMetricsF(font()).height();
 QString help_str = tr(
 " <h1><img width=\"%1\" src=\":/home/img/question-64.png\">&nbsp;&nbsp;Welcome to UnseenP2P!</h1>\
   <p>You need to <b>make friends</b>! After you create a network of friends or join an existing network,\
	  you'll be able to exchange files, chat, talk in forums, etc. </p>\
	<div align=center>\
	<IMG align=\"center\" width=\"%2\" src=\":/images/network_map.png\"/> \
    </div>\
   <p>To do so, copy your certificate on this page and send it to friends, and add your friends' certificate.</p> \
   <p>Another option is to search the internet for \"UnseenP2P chat servers\" (independently administrated). These servers allow you to exchange \
    certificates with a dedicated UnseenP2P node, through which\
	  you will be able to anonymously meet other people.</p> ").arg(QString::number(2*S)).arg(width()*0.5);
             registerHelpButton(ui->helpButton,help_str,"HomePage") ;
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
    sendMail("", tr("UnseenP2P Invite"), ui->userCertEdit->toPlainText());
}

void HomePage::copyCert()
{
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(ui->userCertEdit->toPlainText());
    QMessageBox::information(this, "UnseenP2P", tr("Your Cert is copied to Clipboard, paste and send it to your friend via email or some other way"));
}

void HomePage::saveCert()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save as..."), "", tr("UnseenP2P Certificate (*.rsc );;All Files (*)"));
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

void HomePage::webMail()
{
    ConnectFriendWizard connwiz (this);

    connwiz.setStartId(ConnectFriendWizard::Page_WebMail);
    connwiz.exec ();
}

void HomePage::loadCert()
{
    ConnectFriendWizard connwiz (this);

    connwiz.setStartId(ConnectFriendWizard::Page_Cert);
    connwiz.exec ();
}

void HomePage::runStartWizard()
{
    QuickStartWizard(this).exec();
}

void HomePage::openWebHelp()
{
    QDesktopServices::openUrl(QUrl(QString("https://chatserver.unseen.is/howto/"))); //D
}
