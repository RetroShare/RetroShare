/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2009,  RetroShre Team
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
#include "SendLinkDialog.h"

#include <retroshare/rsiface.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsdisc.h>
#include <retroshare/rsmsgs.h>


#include <QTime>

#include <sstream>
#include <iomanip>


/** Default constructor */
SendLinkDialog::SendLinkDialog(QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);
  
  connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
  connect(ui.sendButton, SIGNAL(clicked()), this, SLOT(sendLinktoChat()));
  
  
}

/** Destructor. */
SendLinkDialog::~SendLinkDialog()
{

}

void SendLinkDialog::closeEvent (QCloseEvent * event)
{
  QDialog::closeEvent(event);
}


void SendLinkDialog::insertHtmlText(std::string msg)
{
	ui.linkText->setHtml(QString("<a href='") + QString::fromStdString(std::string(msg + "'> ") ) + QString::fromStdString(std::string(msg)) + "</a>") ;
	
}

void SendLinkDialog::sendLinktoChat()
{
	ChatInfo ci;
	ci.msg = ui.linkText->toHtml().toStdWString();
	ci.chatflags = RS_CHAT_PUBLIC;

	rsMsgs -> ChatSend(ci);
	close();
}


