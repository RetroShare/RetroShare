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
#include "form_fileRecive.h"
#include "src/FileTransferRecive.h"

form_fileRecive::form_fileRecive(cFileTransferRecive * FileRecive)
{
	setupUi(this);
	this->setAttribute(Qt::WA_DeleteOnClose,true);

	this->FileRecive=FileRecive;
	init();

	connect(FileRecive,SIGNAL(event_FileRecivedFinishedOK()),this,
		SLOT(slot_FileRecivedFinishedOK()));

	connect(FileRecive,SIGNAL(event_allreadyRecivedSizeChanged(quint64)),this,
		SLOT(slot_allreadyRecivedSizeChanged(quint64)));

	connect(FileRecive,SIGNAL(event_FileReciveError()),this,
		SLOT(slot_FileReciveError()));

	connect(FileRecive,SIGNAL(event_FileReciveAbort()),this,
		SLOT(slot_FileReciveAbort()));

	connect(pushButton,SIGNAL(pressed()),this,
		SLOT(slot_Button()));	

}

void form_fileRecive::init()
{
	QLabel *label_4=this->label_4;
	QLabel *label_6=this->label_6;
	QLabel *label_7=this->label_7;
	QProgressBar * progressBar= this->progressBar;
	

	label_4->setText(FileRecive->get_FileName());
		
	QString SSize;		
	SSize.setNum(FileRecive->get_FileSize(),10);	
	label_6->setText(SSize);
	label_7->setText("Bits");
	checkBox_3->setChecked(true);
	progressBar->setMinimum(0);
	progressBar->setMaximum(FileRecive->get_FileSize());
	progressBar->setValue(0);
}

void form_fileRecive::slot_Button()
{
	QPushButton* pushButton= this->pushButton;
	FileRecive->abbortFileRecive();
	this->close();
}

void form_fileRecive::slot_allreadyRecivedSizeChanged(quint64 value)
{
	progressBar->setValue(value);
}

void form_fileRecive::slot_FileRecivedFinishedOK()
{
	QCheckBox* checkBox_4= this->checkBox_4;
	checkBox_4->setChecked(true);

	QMessageBox* msgBox= new QMessageBox(NULL);
	msgBox->setIcon(QMessageBox::Information);
	msgBox->setText("cFileTransfer");
	msgBox->setInformativeText("FileRecive Finished");
	msgBox->setStandardButtons(QMessageBox::Ok);
	msgBox->setDefaultButton(QMessageBox::Ok);
	msgBox->setWindowModality(Qt::NonModal);
	msgBox->show();
		
	this->close();
}

void form_fileRecive::slot_FileReciveError()
{
	QMessageBox* msgBox= new QMessageBox(NULL);
	msgBox->setIcon(QMessageBox::Information);
	msgBox->setText("cFileTransferRecive(StreamStatus)");
	msgBox->setInformativeText("FileRecive Error(connection Broke)\nnincomplead File deleted");
	msgBox->setStandardButtons(QMessageBox::Ok);
	msgBox->setDefaultButton(QMessageBox::Ok);
	msgBox->setWindowModality(Qt::NonModal);
	msgBox->show();

	this->close();
}

void form_fileRecive::slot_FileReciveAbort()
{
	QMessageBox* msgBox= new QMessageBox(NULL);
	msgBox->setIcon(QMessageBox::Information);
	msgBox->setText("Filetransfer");
	msgBox->setInformativeText("the Sender abort the Filetransfer\nincomplead File deleted");
	msgBox->setStandardButtons(QMessageBox::Ok);
	msgBox->setDefaultButton(QMessageBox::Ok);
	msgBox->setWindowModality(Qt::NonModal);
	msgBox->show();
	
	this->close();
}
