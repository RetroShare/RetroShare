/***************************************************************************
 *   Copyright (C) 2008 by normal   *
 *   normal@Desktop2   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "form_DebugMessages.h"

form_DebugMessages::form_DebugMessages(cCore* core,QDialog *parent){
	setupUi(this);
	this->setAttribute(Qt::WA_DeleteOnClose,true);

	this->core=core;
	this->DebugMessageManager=core->get_DebugMessageHandler();

	connect(cmd_clear,SIGNAL(clicked() ),this,SLOT(clearDebugMessages()));
	connect(DebugMessageManager,SIGNAL(newDebugMessage(QString) ),this,SLOT(newDebugMessage( QString)));
	connect(cmd_connectionDump,SIGNAL(clicked()),this,
		SLOT(connectionDump()));
	
	newDebugMessage("");
}

form_DebugMessages::~form_DebugMessages()
{
}

void form_DebugMessages::connectionDump()
{
	QString Message=core->get_connectionDump();
	
	QMessageBox* msgBox= new QMessageBox(this);
	msgBox->setIcon(QMessageBox::Information);
	msgBox->setText("DebugInformations fromCore");
	msgBox->setInformativeText(Message);
	msgBox->setStandardButtons(QMessageBox::Ok);
	msgBox->setDefaultButton(QMessageBox::Ok);
	msgBox->setWindowModality(Qt::NonModal);
	msgBox->show();
}





void form_DebugMessages::newDebugMessage(QString Message)
{
	textEdit->clear();
	
	QStringList temp=DebugMessageManager->getAllMessages();
	for(int i=0;i<temp.count();i++){
		this->textEdit->append(temp[i]);
	}

	QTextCursor cursor = textEdit->textCursor();
	cursor.movePosition(QTextCursor::Start);
	textEdit->setTextCursor(cursor);
	
}
void form_DebugMessages::clearDebugMessages(){
	DebugMessageManager->clearAllMessages();
	textEdit->clear();
}
