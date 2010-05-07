/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2009 RetroShare Team
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
#include "SoundPage.h"


/** Constructor */
SoundPage::SoundPage(QWidget * parent, Qt::WFlags flags)
    : ConfigPage(parent, flags)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  /* Create RshareSettings object */
  _settings = new RshareSettings();

  connect(ui.cmd_openFile,  SIGNAL(clicked(bool) ),this,SLOT(on_cmd_openFile()));
  //connect(ui.cmd_openFile_2,SIGNAL(clicked(bool) ),this,SLOT(on_cmd_openFile2()));
	connect(ui.cmd_openFile_3,SIGNAL(clicked(bool) ),this,SLOT(on_cmd_openFile3()));
	connect(ui.cmd_openFile_4,SIGNAL(clicked(bool) ),this,SLOT(on_cmd_openFile4()));
	connect(ui.cmd_openFile_5,SIGNAL(clicked(bool) ),this,SLOT(on_cmd_openFile5()));
	connect(ui.cmd_openFile_6,SIGNAL(clicked(bool) ),this,SLOT(on_cmd_openFile6()));
	
	ui.groupBox_13->hide();
	ui.groupBox_14->hide();

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

SoundPage::~SoundPage()
{
    delete _settings;
}

/** Saves the changes on this page */
bool
SoundPage::save(QString &errmsg)
{
  _settings->beginGroup("Sound");
		_settings->beginGroup("Enable");
			_settings->setValue("User_go_Online",ui.checkBoxSound->isChecked());
                        //_settings->setValue("User_go_Offline",ui.checkBoxSound_2->isChecked());
			_settings->setValue("FileSend_Finished",ui.checkBoxSound_3->isChecked());
			_settings->setValue("FileRecive_Incoming",ui.checkBoxSound_4->isChecked());
			_settings->setValue("FileRecive_Finished",ui.checkBoxSound_5->isChecked());
			_settings->setValue("NewChatMessage",ui.checkBoxSound_6->isChecked());
			_settings->endGroup();
		_settings->beginGroup("SoundFilePath");
			_settings->setValue("User_go_Online",ui.txt_SoundFile->text());
                        //_settings->setValue("User_go_Offline",ui.txt_SoundFile2->text());
			_settings->setValue("FileSend_Finished",ui.txt_SoundFile3->text());
			_settings->setValue("FileRecive_Incoming",ui.txt_SoundFile4->text());
			_settings->setValue("FileRecive_Finished",ui.txt_SoundFile5->text());
			_settings->setValue("NewChatMessage",ui.txt_SoundFile6->text());
		_settings->endGroup();
	_settings->endGroup();

	return true;
}



/** Loads the settings for this page */
void
SoundPage::load()
{
	_settings->beginGroup("Sound");
		_settings->beginGroup("SoundFilePath");
			ui.txt_SoundFile->setText(_settings->value("User_go_Online","").toString());
                        //ui.txt_SoundFile2->setText(_settings->value("User_go_Offline","").toString());
			ui.txt_SoundFile3->setText(_settings->value("FileSend_Finished","").toString());
			ui.txt_SoundFile4->setText(_settings->value("FileRecive_Incoming","").toString());
			ui.txt_SoundFile5->setText(_settings->value("FileRecive_Finished","").toString());
			ui.txt_SoundFile6->setText(_settings->value("NewChatMessage","").toString());

			if(!ui.txt_SoundFile->text().isEmpty())ui.checkBoxSound->setEnabled(true);
                        //if(!ui.txt_SoundFile2->text().isEmpty())ui.checkBoxSound_2->setEnabled(true);
			if(!ui.txt_SoundFile3->text().isEmpty())ui.checkBoxSound_3->setEnabled(true);
			if(!ui.txt_SoundFile4->text().isEmpty())ui.checkBoxSound_4->setEnabled(true);
			if(!ui.txt_SoundFile5->text().isEmpty())ui.checkBoxSound_5->setEnabled(true);
			if(!ui.txt_SoundFile6->text().isEmpty())ui.checkBoxSound_6->setEnabled(true);

		_settings->endGroup();

		_settings->beginGroup("Enable");
			ui.checkBoxSound->setChecked(_settings->value("User_go_Online",false).toBool());
                        //ui.checkBoxSound_2->setChecked(_settings->value("User_go_Offline",false).toBool());
			ui.checkBoxSound_3->setChecked(_settings->value("FileSend_Finished",false).toBool());
			ui.checkBoxSound_4->setChecked(_settings->value("FileRecive_Incoming",false).toBool());
			ui.checkBoxSound_5->setChecked(_settings->value("FileRecive_Finished",false).toBool());
			ui.checkBoxSound_6->setChecked(_settings->value("NewChatMessage",false).toBool());
		_settings->endGroup();
	_settings->endGroup();
}

void SoundPage::on_cmd_openFile()
{

	ui.txt_SoundFile->setText(QFileDialog::getOpenFileName(this,"Open File", ".", "wav (*.wav)"));
	if(ui.txt_SoundFile->text().isEmpty()){
		ui.checkBoxSound->setChecked(false);
		ui.checkBoxSound->setEnabled(false);
	}
	else
		ui.checkBoxSound->setEnabled(true);
}

/*void SoundPage::on_cmd_openFile2()
{
	ui.txt_SoundFile2->setText(QFileDialog::getOpenFileName(this,"Open File", ".", "wav (*.wav)"));
	if(ui.txt_SoundFile2->text().isEmpty()){
		ui.checkBoxSound_2->setChecked(false);
		ui.checkBoxSound_2->setEnabled(false);
	}
	else
		ui.checkBoxSound_2->setEnabled(true);

}*/
void SoundPage::on_cmd_openFile3()
{
	ui.txt_SoundFile3->setText(QFileDialog::getOpenFileName(this,"Open File", ".", "wav (*.wav)"));
	if(ui.txt_SoundFile3->text().isEmpty()){
		ui.checkBoxSound_3->setChecked(false);
		ui.checkBoxSound_3->setEnabled(false);
	}
	else
		ui.checkBoxSound_3->setEnabled(true);
}
void SoundPage::on_cmd_openFile4()
{
	ui.txt_SoundFile4->setText(QFileDialog::getOpenFileName(this,"Open File", ".", "wav (*.wav)"));
	if(ui.txt_SoundFile4->text().isEmpty()){
		ui.checkBoxSound_4->setChecked(false);
		ui.checkBoxSound_4->setEnabled(false);
	}
	else
		ui.checkBoxSound_4->setEnabled(true);
}
void SoundPage::on_cmd_openFile5()
{
	ui.txt_SoundFile5->setText(QFileDialog::getOpenFileName(this,"Open File", ".", "wav (*.wav)"));
	if(ui.txt_SoundFile5->text().isEmpty()){
		ui.checkBoxSound_5->setChecked(false);
		ui.checkBoxSound_5->setEnabled(false);
	}
	else
		ui.checkBoxSound_5->setEnabled(true);
}
void SoundPage::on_cmd_openFile6()
{
	ui.txt_SoundFile6->setText(QFileDialog::getOpenFileName(this,"Open File", ".", "wav (*.wav)"));
	if(ui.txt_SoundFile6->text().isEmpty()){
		ui.checkBoxSound_6->setChecked(false);
		ui.checkBoxSound_6->setEnabled(false);
	}
	else
		ui.checkBoxSound_6->setEnabled(true);

}
