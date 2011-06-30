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
#include "rsharesettings.h"


/** Constructor */
SoundPage::SoundPage(QWidget * parent, Qt::WFlags flags)
    : ConfigPage(parent, flags)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

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
}

/** Saves the changes on this page */
bool
SoundPage::save(QString &errmsg)
{
  Settings->beginGroup("Sound");
                Settings->beginGroup("Enable");
                        Settings->setValue("User_go_Online",ui.checkBoxSound->isChecked());
                        //settings.setValue("User_go_Offline",ui.checkBoxSound_2->isChecked());
                        Settings->setValue("FileSend_Finished",ui.checkBoxSound_3->isChecked());
                        Settings->setValue("FileRecive_Incoming",ui.checkBoxSound_4->isChecked());
                        Settings->setValue("FileRecive_Finished",ui.checkBoxSound_5->isChecked());
                        Settings->setValue("NewChatMessage",ui.checkBoxSound_6->isChecked());
                        Settings->endGroup();
                Settings->beginGroup("SoundFilePath");
                        Settings->setValue("User_go_Online",ui.txt_SoundFile->text());
                        //settings.setValue("User_go_Offline",ui.txt_SoundFile2->text());
                        Settings->setValue("FileSend_Finished",ui.txt_SoundFile3->text());
                        Settings->setValue("FileRecive_Incoming",ui.txt_SoundFile4->text());
                        Settings->setValue("FileRecive_Finished",ui.txt_SoundFile5->text());
                        Settings->setValue("NewChatMessage",ui.txt_SoundFile6->text());
                Settings->endGroup();
        Settings->endGroup();

	return true;
}



/** Loads the settings for this page */
void
SoundPage::load()
{
        Settings->beginGroup("Sound");
                Settings->beginGroup("SoundFilePath");
                        ui.txt_SoundFile->setText(Settings->value("User_go_Online","").toString());
                        //ui.txt_SoundFile2->setText(settings.value("User_go_Offline","").toString());
                        ui.txt_SoundFile3->setText(Settings->value("FileSend_Finished","").toString());
                        ui.txt_SoundFile4->setText(Settings->value("FileRecive_Incoming","").toString());
                        ui.txt_SoundFile5->setText(Settings->value("FileRecive_Finished","").toString());
                        ui.txt_SoundFile6->setText(Settings->value("NewChatMessage","").toString());

			if(!ui.txt_SoundFile->text().isEmpty())ui.checkBoxSound->setEnabled(true);
                        //if(!ui.txt_SoundFile2->text().isEmpty())ui.checkBoxSound_2->setEnabled(true);
			if(!ui.txt_SoundFile3->text().isEmpty())ui.checkBoxSound_3->setEnabled(true);
			if(!ui.txt_SoundFile4->text().isEmpty())ui.checkBoxSound_4->setEnabled(true);
			if(!ui.txt_SoundFile5->text().isEmpty())ui.checkBoxSound_5->setEnabled(true);
			if(!ui.txt_SoundFile6->text().isEmpty())ui.checkBoxSound_6->setEnabled(true);

                Settings->endGroup();

                Settings->beginGroup("Enable");
                        ui.checkBoxSound->setChecked(Settings->value("User_go_Online",false).toBool());
                        //ui.checkBoxSound_2->setChecked(settings.value("User_go_Offline",false).toBool());
                        ui.checkBoxSound_3->setChecked(Settings->value("FileSend_Finished",false).toBool());
                        ui.checkBoxSound_4->setChecked(Settings->value("FileRecive_Incoming",false).toBool());
                        ui.checkBoxSound_5->setChecked(Settings->value("FileRecive_Finished",false).toBool());
                        ui.checkBoxSound_6->setChecked(Settings->value("NewChatMessage",false).toBool());
                Settings->endGroup();
        Settings->endGroup();
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
