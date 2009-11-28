/***************************************************************************
 *   Copyright (C) 2007 by Balázs Béla                                     *
 *   balazsbela@gmail.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 2 of the License                *
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

#include "settings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include "qorganizer.h"
#include <QTextStream>
#include <QCoreApplication>


SettingsDialog::SettingsDialog(qOrganizer* parent)
{
  
  parent->C_OLD_STORINGMODE=parent->C_STORINGMODE;
  //Interface 

  //Title string hack:
  QString titleString = tr("&Settings"); //This is translated but we need Settings
  titleString.remove("&",Qt::CaseSensitive);
  setWindowTitle(titleString);
  //Widgets
  
  warnLabel = new QLabel("<center>"+tr("Some settings require restarting qOrganizer")+"</center>");
  pathLabel=new QLabel(tr("Path to data folder:"));
  pathEdit = new QLineEdit(parent->C_PATH);
  if(parent->C_PATH=="home")
   pathEdit->setText(QDir::homePath());
  pathButton = new QPushButton(tr("Browse"));
  
  pathEdit->setEnabled(false);
  
  rLabel = new QLabel(tr("Reminder check interval:"));
  rBox = new QSpinBox();
  rBox ->setRange(10000,60000);
  rBox -> setValue(parent->C_TIMEOUT);
 
  firstDayLabel = new QLabel(tr("First day of week:")); 
  firstDay = new QComboBox();
  firstDay -> addItem(tr("Monday")); 
  firstDay -> addItem(tr("Tuesday")); 
  firstDay -> addItem(tr("Wednesday")); 
  firstDay -> addItem(tr("Thursday")); 
  firstDay -> addItem(tr("Friday")); 
  firstDay -> addItem(tr("Saturday")); 
  firstDay -> addItem(tr("Sunday")); 
  firstDay -> setCurrentIndex(parent->C_FIRST_DAY_OF_WEEK-1);

  dateFormatLabel = new QLabel(tr("Date format:"));
  dateFormatEdit = new QComboBox();
  dateFormatEdit -> setEditable(true);
  dateFormatEdit -> addItem("dddd MMMM d. yyyy");
  dateFormatEdit -> addItem("d. MMMM dddd yyyy");
  dateFormatEdit -> addItem("MMMM d. dddd yyyy"); 
  dateFormatEdit -> addItem("yyyy MMMM d. dddd"); 
  dateFormatEdit -> addItem("yyyy d. MMMM dddd"); 
  dateFormatEdit -> addItem("yyyy MMMM d. dddd"); 
  dateFormatEdit -> addItem("yyyy dddd MMMM d. "); 
  dateFormatEdit -> addItem("dddd d. MMMM yyyy");
  
  autoLabel = new QLabel(tr("Autosave:"));
  autoBox = new QCheckBox();
  if(parent->C_AUTOSAVE_TOGGLE)
   autoBox -> setChecked(true);

  messageLabel = new QLabel(tr("Remind type:"));
  messageCombo = new QComboBox();
  messageCombo -> addItem("Baloon");
  messageCombo -> addItem("Message box");
  if(!parent->C_BALOON_SET) messageCombo->setCurrentIndex(1);

  langLabel = new QLabel(tr("Language:"));
  langCombo = new QComboBox();
  langCombo -> addItem("English");
  readLangDir();  
  if(langCombo->findText(parent->C_LANGUAGETEXT)!=-1) langCombo->setCurrentIndex(langCombo->findText(parent->C_LANGUAGETEXT));

  rownrLabel = new QLabel(tr("Row number for empty schedule:"));
  rownrBox = new QSpinBox();
  rownrBox -> setRange(0,40);
  rownrBox -> setValue(parent->C_NRROWS); 

  QLabel *roundLabel = new QLabel(tr("Round subject averages when calculating total average"));
  roundBox = new QCheckBox;
  roundBox->setChecked(parent->C_ROUND_ON_AVERAGE);  
  
  QLabel *saveAllLabel = new QLabel(tr("Show saving buttons on toolbar"));
  saveAllBox = new QCheckBox;
  saveAllBox->setChecked(parent->C_SHOW_SAVE); 
 
  QHBoxLayout *roundLayout = new QHBoxLayout;
  roundLayout -> addWidget(roundLabel);
  roundLayout -> addWidget(roundBox);
   
  QHBoxLayout *saveAllLayout = new QHBoxLayout;
  saveAllLayout -> addWidget(saveAllLabel);
  saveAllLayout -> addWidget(saveAllBox);

  QPushButton *okButton = new QPushButton("OK");
  QPushButton *cancelButton = new QPushButton(tr("Cancel"));

  okButton->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
  cancelButton->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);

  QLabel *loadLabel = new QLabel(tr("Reload data on view change"));
  loadBox = new QCheckBox(this);
  if(parent->C_LOAD_ON_CHANGE) loadBox->setChecked(true); else loadBox->setChecked(false);
  QHBoxLayout *loadLayout = new QHBoxLayout;
  loadLayout -> addWidget(loadLabel);
  loadLayout -> addWidget(loadBox);
 
  trayLabel = new QLabel(tr("Hide to system tray on close"));
  trayBox = new QCheckBox;
  if(parent->C_SYSTRAY) trayBox->setChecked(true); 
   else trayBox->setChecked(false);

  QHBoxLayout *trayLayout = new QHBoxLayout;
  trayLayout-> addWidget(trayLabel);
  trayLayout-> addWidget(trayBox);
  //Ftp sync labels
  portLabel=new QLabel(tr("Port:"));
  hostLabel=new QLabel(tr("Host:"));
  userLabel=new QLabel(tr("Username:"));
  passLabel=new QLabel(tr("Password:"));
  ftppathLabel=new QLabel(tr("Path on ftp server:"));
  ftpSyncLabel=new QLabel(tr("Synchronize with ftp server:"));

  hostEdit=new QLineEdit;
  hostEdit->setFixedWidth(190);
  hostEdit -> setText(parent->C_HOST);
  portBox=new QSpinBox;
  portBox->setRange(21,65535);
  portBox->setValue(parent->C_PORT);
  portBox->setFixedWidth(90);
  //I don't think ftp servers are running on a lower port than 21
  //If this ever causes problems contact me.

  userEdit=new QLineEdit;
  userEdit->setFixedWidth(100);
  userEdit->setText(parent->C_USER);
  passEdit=new QLineEdit;
  passEdit->setFixedWidth(100);
  passEdit->setEchoMode(QLineEdit::Password);
  passEdit->setText(parent->C_PASSWD);    
 
  ftppathEdit = new QLineEdit;
  ftppathEdit->setFixedWidth(240);
  ftppathEdit->setText(parent->C_FTPPATH);
 
  ftpBox = new QCheckBox;
  connect(ftpBox,SIGNAL(stateChanged(int)),this,SLOT(toggleFTP(int)));   
  if(parent->C_SYNCFTP)
   ftpBox->setChecked(true);
    else 
      {
       ftpBox->setChecked(false);
       toggleFTP(0);
      }

  QHBoxLayout *soundLayout = new QHBoxLayout;
  soundLabel = new QLabel(tr("Play sound on remind"));
  soundBox = new QCheckBox;
  
  if(parent->C_SOUND)
   soundBox->setChecked(true);
    else soundBox->setChecked(false);
   
  soundLayout -> addWidget(soundLabel);
  soundLayout -> addWidget(soundBox);
    
 
  QLabel *oddTTLabel = new QLabel(tr("Use different timetables for even and odd weeks"));
  oddTTBox = new QCheckBox;
  
  QHBoxLayout *oddTTLayout = new QHBoxLayout;
  oddTTLayout -> addWidget(oddTTLabel);
  oddTTLayout -> addWidget(oddTTBox);  
  oddTTBox->setChecked(parent->C_USE_ODDTT);
 
  QLabel *reverseTTLabel = new QLabel(tr("Reverse normal order of weeks for timetable"));
  reverseTTBox = new QCheckBox;
  QHBoxLayout *reverseTTLayout = new QHBoxLayout;
  reverseTTBox->setChecked(parent->C_TT_REVERSORDER);
  reverseTTLayout->addWidget(reverseTTLabel);
  reverseTTLayout->addWidget(reverseTTBox);

  //FTP Layouts
  QHBoxLayout *ftpBoxLayout = new QHBoxLayout;
  ftpBoxLayout -> addWidget(ftpSyncLabel);
  ftpBoxLayout -> addWidget(ftpBox);
  
  QHBoxLayout *portLayout = new QHBoxLayout;
  portLayout -> addWidget(portLabel);
  portLayout -> addWidget(portBox);
  
  QHBoxLayout *hostLayout = new QHBoxLayout;
  hostLayout -> addWidget(hostLabel);
  hostLayout -> addWidget(hostEdit);
  hostLayout -> addLayout(portLayout);
  
  QHBoxLayout *passLayout = new QHBoxLayout;
  passLayout -> addWidget(passLabel);
  passLayout -> addWidget(passEdit);
 
  QHBoxLayout *userLayout = new QHBoxLayout;
  userLayout -> addWidget(userLabel);
  userLayout -> addWidget(userEdit);
  userLayout -> addLayout(passLayout);
  
  QHBoxLayout *ftppathLayout = new QHBoxLayout;
  ftppathLayout -> addWidget(ftppathLabel);
  ftppathLayout -> addWidget(ftppathEdit);
  
  //Layouts
  QHBoxLayout *pathLayout = new QHBoxLayout;
  pathLayout -> addWidget(pathLabel);
  pathLayout -> addWidget(pathEdit);
  pathLayout -> addWidget(pathButton);
 
  QHBoxLayout *rLayout = new QHBoxLayout;
  rLayout -> addWidget(rLabel);
  rLayout -> addWidget(rBox);
 
  QHBoxLayout *dayLayout = new QHBoxLayout;
  dayLayout -> addWidget(firstDayLabel); 
  dayLayout -> addWidget(firstDay);
  
  QHBoxLayout *autoLayout = new QHBoxLayout;
  autoLayout -> addWidget(autoLabel);
  autoLayout -> addWidget(autoBox);

  QHBoxLayout *messageLayout = new QHBoxLayout;
  messageLayout -> addWidget(messageLabel);
  messageLayout -> addWidget(messageCombo);
 
  QHBoxLayout *langLayout = new QHBoxLayout;
  langLayout -> addWidget(langLabel);
  langLayout -> addWidget(langCombo); 
  
  QHBoxLayout *rownrLayout = new QHBoxLayout;
  rownrLayout -> addWidget(rownrLabel);
  rownrLayout -> addWidget(rownrBox);
 
  QHBoxLayout *buttonLayout = new QHBoxLayout;
  buttonLayout -> addWidget(okButton);
  buttonLayout -> addWidget(cancelButton);

  QHBoxLayout *dateFormatLayout = new QHBoxLayout;
  dateFormatLayout-> addWidget(dateFormatLabel);
  dateFormatLayout-> addWidget(dateFormatEdit);
  
  QVBoxLayout *ftpVLayout = new QVBoxLayout;
  ftpVLayout -> addLayout(ftpBoxLayout); 
  ftpVLayout -> addLayout(hostLayout);
  ftpVLayout -> addLayout(userLayout);
  ftpVLayout -> addLayout(ftppathLayout);

  storingBox = new QComboBox;
  storingBox-> addItem(tr("Text files"));
  storingBox-> addItem(tr("SQLite database"));
  storingBox-> addItem(tr("MySQL database"));
 

  QLabel *storingLabel = new QLabel(tr("Store data in:"));
  QHBoxLayout *storingSelectLayout = new QHBoxLayout;
  storingSelectLayout -> addWidget(storingLabel);
  storingSelectLayout -> addWidget(storingBox);

  storingBox->setCurrentIndex(parent->C_STORINGMODE);
 

  QVBoxLayout *storingLayout = new QVBoxLayout;
  storingLayout -> addLayout(storingSelectLayout);
  storingLayout -> addSpacing(20);
  

  mysqlHostEdit = new QLineEdit;
  mysqlHostEdit->setFixedWidth(190);
  mysqlHostEdit->setText(parent->C_MYSQL_HOST);

  mysqlPortBox = new QSpinBox;  
  mysqlPortBox->setRange(21,65535);
  mysqlPortBox->setValue(3306);
  if((parent->C_MYSQL_PORT) > 0) mysqlPortBox->setValue(parent->C_MYSQL_PORT);

  mysqlPortBox->setFixedWidth(90);  

  QLabel *mysqlHostLabel = new QLabel(tr("Host:"));
  QLabel *mysqlPortLabel = new QLabel(tr("Port:"));
  
  QHBoxLayout *mysqlHostLayout = new QHBoxLayout;
  mysqlHostLayout->addWidget(mysqlHostLabel);
  mysqlHostLayout->addWidget(mysqlHostEdit);
  mysqlHostLayout->addWidget(mysqlPortLabel);
  mysqlHostLayout->addWidget(mysqlPortBox);
  storingLayout -> addLayout(mysqlHostLayout);


  mysqlUserEdit = new QLineEdit;
  mysqlUserEdit->setFixedWidth(100);
  mysqlUserEdit->setText(parent->C_MYSQL_USER);
 
  mysqlPassEdit = new QLineEdit;
  mysqlPassEdit->setFixedWidth(100);
  mysqlPassEdit->setText(parent->C_MYSQL_PASSWD);
  mysqlPassEdit->setEchoMode(QLineEdit::Password);
  QLabel *mysqlUserLabel = new QLabel(tr("Username:"));
  QLabel *mysqlPassLabel = new QLabel(tr("Password:"));
     

  QHBoxLayout *mysqlUserLayout = new QHBoxLayout;
  mysqlUserLayout->addWidget(mysqlUserLabel);
  mysqlUserLayout->addWidget(mysqlUserEdit);
  mysqlUserLayout->addWidget(mysqlPassLabel);
  mysqlUserLayout->addWidget(mysqlPassEdit);
  storingLayout -> addLayout(mysqlUserLayout);
  
  QLabel *mysqlDBLabel = new QLabel(tr("Database name:"));
  mysqldbEdit = new QLineEdit;
  mysqldbEdit->setText(parent->C_MYSQL_DB);
  QHBoxLayout *mysqlDBLayout = new QHBoxLayout;
  mysqlDBLayout->addWidget(mysqlDBLabel);
  mysqlDBLayout->addWidget(mysqldbEdit);
  storingLayout -> addLayout(mysqlDBLayout);
   
  QLabel *mysqlFtpLabel = new QLabel(tr("FTP synchronization is not available for MySQL database"));
  storingLayout -> addWidget(mysqlFtpLabel);

  QVBoxLayout *main = new QVBoxLayout;
  /*main -> addWidget(warnLabel);
  main -> addLayout(pathLayout);
  main -> addLayout(rLayout);
  main -> addLayout(dayLayout);
  main -> addLayout(dateFormatLayout);
  main -> addLayout(autoLayout);
  main -> addLayout(messageLayout);
  main -> addLayout(langLayout);
  main -> addLayout(rownrLayout);
  main -> addLayout(loadLayout);
  main -> addLayout(trayLayout);
  main -> addLayout(soundLayout);
  main -> addSpacing(10);
  main -> addLayout(ftpVLayout);
  main -> addSpacing(10);
  main -> addLayout(storingLayout);
  main -> addSpacing(10);  
  main -> addLayout(buttonLayout);*/
 
  //QTabWidget
  tabWidget = new QTabWidget(this);
  main->addWidget(tabWidget);
  main->addWidget(warnLabel);
  main->addLayout(buttonLayout);

  setLayout(main);
  
  QVBoxLayout *generalLayout = new QVBoxLayout;
  generalLayout -> addLayout(pathLayout);
  generalLayout -> addLayout(autoLayout);
  generalLayout -> addLayout(langLayout);
  generalLayout -> addLayout(loadLayout);
  generalLayout -> addLayout(trayLayout);
  generalLayout -> addLayout(oddTTLayout);
  generalLayout -> addLayout(reverseTTLayout);
  generalLayout -> addLayout(roundLayout);
  generalLayout -> addLayout(saveAllLayout);
 
  QVBoxLayout *calendarLayout = new QVBoxLayout;
  calendarLayout -> addLayout(rLayout);
  calendarLayout -> addLayout(dayLayout); 
  calendarLayout -> addLayout(dateFormatLayout);
  calendarLayout -> addLayout(messageLayout);
  calendarLayout -> addLayout(rownrLayout);
  calendarLayout -> addLayout(soundLayout);

  

  General = new QWidget();
  General -> setLayout(generalLayout);
  
  Calendar = new QWidget(); 
  Calendar -> setLayout(calendarLayout); 
 
  Ftp = new QWidget();
  Ftp -> setLayout(ftpVLayout);
 
  Storing = new QWidget();
  Storing -> setLayout(storingLayout);

  tabWidget -> addTab(General,tr("&General")); 
  tabWidget -> addTab(Calendar,tr("&Calendar")); 
  tabWidget -> addTab(Ftp,tr("FTP"));
  tabWidget -> addTab(Storing,tr("Storing"));
  toggleMySQL(parent->C_STORINGMODE);
 
  okButton->setFocus();
  //Connecting buttons
  connect(pathButton,SIGNAL(clicked()),this,SLOT(setDir()));
  connect(cancelButton,SIGNAL(clicked()),this,SLOT(close()));
  connect(okButton, SIGNAL(clicked()),parent,SLOT(saveConfigFile()));
  connect(storingBox,SIGNAL(currentIndexChanged(int)),this,SLOT(toggleMySQL(int)));   
}


/*This function reads all qm files from the lang folder included in the resources
and adds them to the combo box.
We store languages in the resources to make sure it works every time and .qm files don't
get lost. So if you are translating this app, add your qm file to the lang folder,
add an entry in application.qrc and recompile qOrganizer*/

void SettingsDialog::readLangDir()
{
        QDir dir = QDir(":/lang");
        QString filter = "*.qm";
	QDir::Filters filters = QDir::Files | QDir::Readable;
	QDir::SortFlags sort = QDir::Name;
	QFileInfoList entries = dir.entryInfoList(QStringList()<< filter, filters, sort);
	foreach (QFileInfo file, entries)
	{
	  langCombo -> addItem(file.baseName());
	}
};

SettingsDialog::~SettingsDialog()
{
};

void SettingsDialog::setDir()
{
 QString dir = QFileDialog::getExistingDirectory(this, tr("Select Directory"),
                                                  "",
                                                  QFileDialog::ShowDirsOnly
                                                  | QFileDialog::DontResolveSymlinks);
  pathEdit->setText(dir);
}

void SettingsDialog::toggleFTP(int state)
{
 if(state==0)
 {
   hostEdit->setEnabled(false);
   portBox->setEnabled(false);
   userEdit->setEnabled(false);
   passEdit->setEnabled(false);
   ftppathEdit->setEnabled(false);
 }
 else 
  {
    hostEdit->setEnabled(true);
    portBox->setEnabled(true);
    userEdit->setEnabled(true);
    passEdit->setEnabled(true);
    ftppathEdit->setEnabled(true);
  } 
}

void SettingsDialog::toggleMySQL(int state)
{
 if(state!=2)
 {
   mysqlHostEdit->setEnabled(false);
   mysqlPortBox->setEnabled(false);
   mysqlUserEdit->setEnabled(false);
   mysqlPassEdit->setEnabled(false);
   mysqldbEdit->setEnabled(false);
 }
 else 
  {
   mysqlHostEdit->setEnabled(true);
   mysqlPortBox->setEnabled(true);
   mysqlUserEdit->setEnabled(true);
   mysqlPassEdit->setEnabled(true);
   mysqldbEdit->setEnabled(true);
  } 
}

void SettingsDialog::close()
{
 this -> deleteLater();
};
