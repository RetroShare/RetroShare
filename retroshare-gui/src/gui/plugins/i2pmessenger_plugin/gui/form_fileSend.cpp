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

#include "form_fileSend.h"



form_fileSend::form_fileSend(cFileTransferSend * FileTransfer)
{
	setupUi(this);
	this->setAttribute(Qt::WA_DeleteOnClose,true);

	this->FileTransfer=FileTransfer;
	QPushButton* pushButton= this->pushButton;

	init();
	
	connect(FileTransfer,SIGNAL(event_allreadySendedSizeChanged(quint64)),this,
		SLOT(slot_allreadySendedSizeChanged(quint64)));

	connect(FileTransfer,SIGNAL(event_FileTransferFinishedOK()),this,
		SLOT(slot_FileTransferFinishedOK()));

	connect(FileTransfer,SIGNAL(event_FileTransferAccepted(bool)),this,
		SLOT(slot_FileTransferAccepted(bool)));

	connect(FileTransfer,SIGNAL(event_FileTransferAborted()),this,
		SLOT(slot_FileTransferAborted()));

	connect(FileTransfer,SIGNAL(event_FileTransferError()),this,
		SLOT(slot_FileTransferError()));

	connect(pushButton,SIGNAL(pressed()),this,
		SLOT(slot_Button()));
}

void form_fileSend::init()
{
	
	QLabel *label_4=this->label_4;
	QLabel *label_6=this->label_6;
	QLabel *label_7=this->label_7;
	QProgressBar * progressBar= this->progressBar;
	

	label_4->setText(FileTransfer->get_FileName());
		
	QString SSize;		
	SSize.setNum(FileTransfer->get_FileSize(),10);	
	label_6->setText(SSize);
	label_7->setText("Bits");
	progressBar->setMinimum(0);
	progressBar->setMaximum(FileTransfer->get_FileSize());
	progressBar->setValue(0);
}

void form_fileSend::slot_allreadySendedSizeChanged(quint64 value)
{
	progressBar->setValue(value);
}

void form_fileSend::slot_FileTransferFinishedOK()
{
	QCheckBox* checkBox_4= this->checkBox_4;
	checkBox_4->setChecked(true);

	QMessageBox* msgBox= new QMessageBox(NULL);
	msgBox->setIcon(QMessageBox::Information);
	msgBox->setText("Filetransfer");
	msgBox->setInformativeText("Filetransfer Finished (OK)");
	msgBox->setStandardButtons(QMessageBox::Ok);
	msgBox->setDefaultButton(QMessageBox::Ok);
	msgBox->setWindowModality(Qt::NonModal);
	msgBox->show();
	this->close();

	this->close();
}

void form_fileSend::slot_FileTransferAccepted(bool t)
{
	QPushButton* pushButton= this->pushButton;

	if(t==true){
		checkBox_2->setChecked(true);
		checkBox_3->setChecked(true);
	}
	else{
		QMessageBox* msgBox= new QMessageBox(NULL);
		msgBox->setIcon(QMessageBox::Information);
		msgBox->setText("FileTransfer)");
		msgBox->setInformativeText("Filetransfer don't Accepted\nFileSending Abborted");
		msgBox->setStandardButtons(QMessageBox::Ok);
		msgBox->setDefaultButton(QMessageBox::Ok);
		msgBox->setWindowModality(Qt::NonModal);
		msgBox->show();

		this->close();
	}
}

void form_fileSend::slot_Button()
{
	QPushButton* pushButton= this->pushButton;
	FileTransfer->abbortFileSend();

	this->close();
}

void form_fileSend::slot_FileTransferError()
{
	QMessageBox* msgBox= new QMessageBox(NULL);
	msgBox->setIcon(QMessageBox::Information);
	msgBox->setText("Filetransfer");
	msgBox->setInformativeText("FileTransfer Error(connection Broke)");
	msgBox->setStandardButtons(QMessageBox::Ok);
	msgBox->setDefaultButton(QMessageBox::Ok);
	msgBox->setWindowModality(Qt::NonModal);
	msgBox->show();
	
	this->close();
}

void form_fileSend::slot_FileTransferAborted()
{
	QMessageBox* msgBox= new QMessageBox(NULL);
	msgBox->setIcon(QMessageBox::Information);
	msgBox->setText("Filetransfer");
	msgBox->setInformativeText("The Reciver abort the Filerecive");
	msgBox->setStandardButtons(QMessageBox::Ok);
	msgBox->setDefaultButton(QMessageBox::Ok);
	msgBox->setWindowModality(Qt::NonModal);
	msgBox->show();
	this->close();
}




