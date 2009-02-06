/***************************************************************************
 *   Copyright (C) 2007 by Balázs Béla                                     *
 *   balazsbela@gmail.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation version 2.                               *
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

/*
 Special thanks to: Xavier Corredor Llano, the creator of the nuvoX iconset which my icons are mostly based on.I would like to thank him for releasing this great iconset under GPL.
*/

/* 
   For the interface we use a central QStackedWidget object named mainWid and we change the 
   currently active page with the help of the toolbar.
*/


#include <QtGui>
#include "qorganizer.h"
#include <QCloseEvent>
#include <QFileDialog>
#include <QtAlgorithms>
#include <QDebug>
#include <cstdlib>
#include <math.h>

using namespace std;

//The constructor
qOrganizer::qOrganizer()
{ 
      setWindowTitle("qOrganizer");   
      QSize size(1024,768);
      resize(size);
      settings=0;
      getWDirPath();
      readSettings();
      setWindowIcon(QIcon(":/images/icon.png"));
      readConfigFile();
      connectToDB();
      addItems();
      createActions();
      createMenus();
      createToolBars();
      createStatusBar();     
      setColumnWidths();
}

void qOrganizer::getWDirPath()
{
 QSettings registrySettings("qOrganizer","qOrganizer");
 globalWDirPath = registrySettings.value("pathtowdir","home").toString();  
 C_PATH = globalWDirPath;
 if(globalWDirPath=="home") globalWDirPath=QDir::homePath()+QDir::separator()+".qOrganizer";
  else globalWDirPath=C_PATH+QDir::separator()+".qOrganizer";

 if(settings!=0) settings->deleteLater();
 QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,globalWDirPath);
 settings = new QSettings(QSettings::IniFormat,QSettings::UserScope,"qOrganizer", "qOrganizer",this);
      
}

void qOrganizer::readSettings()
{
  //QSettings settings("qOrganizer", "qOrganizer");

  /*Already done in main: (because doing it here caused the window to not be shown on first launch.

  QSize size = settings -> value("windowsize").toSize();
  this -> resize(size);
  bool maximized = settings -> value("windowmaximized").toBool();
  if(maximized) this->showMaximized();*/

  C_LANGUAGETEXT = settings -> value("language").toString();  
  //FTP settings:
  C_HOST = settings -> value("host").toString();
  C_PORT = settings -> value("port").toInt();
  C_USER = settings -> value("user").toString();
  C_PASSWD = settings -> value("passwd").toString();
  C_FTPPATH = settings -> value("path").toString();
  //Other settings:
  C_SYSTRAY = settings -> value("tosystrayonclose",0).toBool();
  C_SOUND = settings -> value("soundonremind",1).toBool();

  C_STORINGMODE = settings -> value("storingmode").toInt();
  
  //MySQL settings

  C_MYSQL_HOST=settings -> value("mysqlhost").toString();
  C_MYSQL_PORT=settings -> value("mysqlport").toInt();
  C_MYSQL_USER=settings -> value("mysqluser").toString();
  C_MYSQL_PASSWD=settings -> value("mysqlpasswd").toString();
  C_MYSQL_DB=settings -> value("mysqldb").toString();
  
  //Different timetables
  C_USE_ODDTT=settings -> value("useoddtimetable",0).toBool();
  C_TT_REVERSORDER=settings -> value("reverseweekorder",0).toBool();
  C_ROUND_ON_AVERAGE = settings -> value("roundonaverage",1).toBool();
  C_SHOW_SAVE = settings->value("showsavebuttons",1).toBool();

}

void qOrganizer::writeSettings()
{
 //QSettings settings("qOrganizer", "qOrganizer");
 settings -> setValue("windowsize", this->size());
 settings -> setValue("windowmaximized", this->isMaximized());
}

//The about box

aboutDialog::aboutDialog()
{
 setWindowTitle(tr("About")+" qOrganizer"+" v3.1");
 aboutLabel = new QLabel(
               QString("<b>qOrganizer : Copyright 2007 Bal&aacute;zs B&eacute;la<br>\n")+
               QString("E-mail:balazsbela@gmail.com<br>")+
               QString("http://qorganizer.sourceforge.net/<br>")+
               QString("<img src=:/images/logo.png align=\"center\"><br>")+
               tr(
               "\n<br>This software is licenced under GPL version 2 <br>published by"
               " the Free Software Foundation.<br>\n" 
               "\n"
               "<br>This application aims to bring simplicity in your life by <br>"
               " organizing your data in an intuitive way so you can always<br>"
               " find what you need and never forget something important.<br>"
               "This application focuses on students, providing <br>"
               "the features they consider usefull. <br>\n")
               );

 aboutLabel->setWordWrap(1);
 textLayout = new QHBoxLayout();
 textLayout -> addWidget(aboutLabel);
 textLayout -> setAlignment(Qt::AlignHCenter);
 textWidget = new QWidget();
 textWidget -> setLayout(textLayout);
  
 tabWidget = new QTabWidget();
 tabWidget -> addTab(textWidget,tr("About"));

 thanksToLabel = new QLabel("<font size=\"5\">"
                            "Evgeniy Ivanov (Russian translation)<br>"
                            "Luis Bastos (Portuguese translation)<br>"
                            "Blaz Kure (Slovenian translation)<br>"
                            "Jefim Borissov (Estonian translation)<br>"
                            "Driton Bakiu (Albanian and Macedonian translation)<br>"
                            "Nicolas Schubert (Spanish translation)<br>"
			    "Wouter Gelderblom (Dutch translation)<br>"
			    "Jens Körner (German translation)<br>"
			    "Naji Mammeri (French translation)<br>"
			    "Dariusz Gadomski (Polish translation)<br>"
                            "Xavier Corredor Llano (nuvoX iconset)<br></font>");
 
 thanksToLabel -> setWordWrap(0);
 thanksLayout = new QHBoxLayout;
 thanksLayout -> addWidget(thanksToLabel);
 thanksLayout -> setAlignment(Qt::AlignHCenter);
 
 thanksWidget = new QWidget;
 thanksWidget -> setLayout(thanksLayout);
 tabWidget -> addTab(thanksWidget,tr("Thanks to"));
 
 okButton = new QPushButton("OK");
 okButton -> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
 okButton -> setFixedWidth(60);
 okLayout = new QHBoxLayout;
 okLayout -> setAlignment(Qt::AlignHCenter);
 okLayout -> addWidget(okButton);
 connect(okButton,SIGNAL(clicked()),this,SLOT(close()));
 
 layout = new QVBoxLayout;
 layout -> addWidget(tabWidget);
 layout -> addLayout(okLayout);
 
 setLayout(layout);
}

void aboutDialog::close()
{
 this->deleteLater();
}

aboutDialog::~aboutDialog()
{
};

void qOrganizer::about()
{
     /* QMessageBox::about(this, tr("About qOrganizer")+" v3.0",
            tr("<b>qOrganizer : Copyright 2007 Bal&aacute;zs B&eacute;la<br>\n"
               "\nE-mail:balazsbela@gmail.com<br>\n"
               "\n"
               "\n<img src=:/images/logo.png><br>\n"
               "\n"
               "\n<br>This software is licenced under GPL version 2 published by"
               " the Free Software Foundation.<br>\n" 
               "\n"
               "<br>This application aims to bring simplicity in your life by "
               " organizing your data in an intuitive way so you can " "always find what you need and never forget something " "important.<br>This application focuses on students, providing "
               "the features they consider usefull.<br><br> \n"
               "Special thanks to:my girlfriend for " "supporting me, to my friends for testing this " "application,"
               "and to Xavier Corredor Llano, for releasing the nuvoX " "iconset under GPL</b>"
              ));*/
 aboutD = new aboutDialog;
 aboutD->show();
}

//Actions
void qOrganizer::createActions()
{
      //calendar Action
      calAct = new QAction(QIcon(":/images/calendar.png"), tr("&Calendar"), this);
      calAct->setShortcut(tr("Ctrl+Alt+C"));
      calAct->setStatusTip(tr("Calendar"));
      calAct->setToolTip(tr("Calendar"));
      connect(calAct, SIGNAL(triggered()),this, SLOT(ChangeToCalendar())); 
      
      //save Action
      saveAct = new QAction(QIcon(":/images/save.png"),tr("Save"),this);
      saveAct->setShortcut(tr("Ctrl+Y"));
      saveAct->setStatusTip(tr("Save"));
      saveAct->setToolTip(tr("Save"));  
      connect(saveAct,SIGNAL(triggered()),this, SLOT(saveAll()));
      
      //save to database Action
      saveToDBAct = new QAction(QIcon(":/images/databasesqlite.png"),tr("&SQLite Database"),this);
      saveToDBAct->setShortcut(tr("Ctrl+D"));
      saveToDBAct->setStatusTip(tr("Save to SQLite database"));
      saveToDBAct->setToolTip(tr("Save to SQLite database"));  
      connect(saveToDBAct,SIGNAL(triggered()),this, SLOT(saveAlltoSQLiteDB()));

      saveToMySQLDBAct = new QAction(QIcon(":/images/databasemysql.png"),tr("&MySQL Database"),this);
      saveToMySQLDBAct->setShortcut(tr("Ctrl+M"));
      saveToMySQLDBAct->setStatusTip(tr("Save to MySQL database"));
      saveToMySQLDBAct->setToolTip(tr("Save to MySQL database"));  
      connect(saveToMySQLDBAct,SIGNAL(triggered()),this, SLOT(saveAlltoMySQLDB()));

      saveToTXTAct = new QAction(QIcon(":/images/save.png"),tr("&Text files"),this);
      saveToTXTAct->setShortcut(tr("Ctrl+T"));
      saveToTXTAct->setStatusTip(tr("Save to text files"));
      saveToTXTAct->setToolTip(tr("Save to text files"));  
      connect(saveToTXTAct,SIGNAL(triggered()),this, SLOT(saveAlltoTXT())); 
      
      //The action that changes to the to-do page
      toDoAct = new QAction(QIcon(":/images/todo.png"), tr("&To-do List"), this);
      toDoAct->setShortcut(tr("Ctrl+Alt+T"));
      toDoAct->setStatusTip(tr("Edit your to-do list"));
      toDoAct->setToolTip(tr("To-do list"));
      connect(toDoAct, SIGNAL(triggered()),this, SLOT(ChangetoToDoList())); 
      
      //The action that changes to the timetable page
      timeTableAct = new QAction(QIcon(":/images/timetable.png"), tr("&TimeTable"), this);
      timeTableAct->setShortcut(tr("Ctrl+Alt+L"));
      timeTableAct->setStatusTip(tr("Edit your timetable"));
      timeTableAct->setToolTip(tr("Timetable"));
      connect(timeTableAct, SIGNAL(triggered()),this, SLOT(ChangeToTimeTable())); 
 
      //The action that changes to the booklet actions
      bookletAct = new QAction(QIcon(":/images/booklet.png"), tr("&Booklet"), this);
      bookletAct->setShortcut(tr("Ctrl+Alt+B"));
      bookletAct->setStatusTip(tr("Keep track of your school marks and absences"));
      bookletAct->setToolTip(tr("Booklet"));
      connect(bookletAct, SIGNAL(triggered()),this, SLOT(ChangeToBooklet())); 

      //Print action
      printAct = new QAction(QIcon(":/images/print.png"), tr("&Print"), this);
      printAct->setShortcut(tr("Ctrl+P"));
      printAct->setStatusTip(tr("Print Page"));
      printAct->setToolTip(tr("Print Page"));
      connect(printAct, SIGNAL(triggered()),this, SLOT(printPage())); 

 
      //Exit action
      exitAct = new QAction(QIcon(":/images/exit.png"),tr("E&xit"), this);
      exitAct->setShortcut(tr("Ctrl+Q"));
      exitAct->setStatusTip(tr("Exit the application"));
      connect(exitAct, SIGNAL(triggered()), this, SLOT(exitApp()));

      //The action for the about box
      aboutAct = new QAction(QIcon(":/images/icon.png"),tr("&About"), this);
      aboutAct->setStatusTip(tr("Show the application's About box"));
      connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

      //About Qt
      aboutQtAct = new QAction(tr("About &Qt"), this);
      aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
      connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
        
      //Upload to FTP server
      uploadAct = new QAction(QIcon(":/images/upload.png"), tr("&Upload"), this);
      uploadAct ->setShortcut(tr("Ctrl+Alt+U"));
      uploadAct ->setStatusTip(tr("Upload data to FTP server"));
      uploadAct ->setToolTip(tr("Upload data to FTP server"));
      connect(uploadAct,SIGNAL(triggered()),this,SLOT(initUpload()));
      
      //Download from FTP server
      downloadAct = new QAction(QIcon(":/images/download.png"), tr("&Download"), this);
      downloadAct ->setShortcut(tr("Ctrl+Alt+W"));
      downloadAct ->setStatusTip(tr("Download data from FTP server"));
      downloadAct ->setToolTip(tr("Download data from FTP server"));
      connect(downloadAct,SIGNAL(triggered()),this,SLOT(getFromFTP()));
      
      //Settings dialog
      settingsAct = new QAction(QIcon(":/images/settings.png"),tr("&Settings"),this);
      settingsAct -> setShortcut(tr("Ctrl+Alt+S"));
      settingsAct -> setStatusTip(tr("Configure application"));
      connect(settingsAct, SIGNAL(triggered()), this ,SLOT(showSettings()));
      
}


//Set up menus
void qOrganizer::createMenus()
{
      fileMenu = menuBar()->addMenu(tr("&File"));
      saveToMenu = new QMenu(fileMenu);
      saveToMenu -> setTitle(tr("Save to"));      
      saveToMenu -> setIcon(QIcon(":/images/save.png"));
      saveToMenu -> addAction(saveToDBAct);
      saveToMenu -> addAction(saveToMySQLDBAct);
      saveToMenu -> addAction(saveToTXTAct);      

      fileMenu->addAction(saveAct);
      fileMenu->addAction(saveToMenu->menuAction());
      fileMenu->addAction(printAct);
      fileMenu->addAction(exitAct);
 
      viewMenu = menuBar()->addMenu(tr("&View"));
      viewMenu -> addAction(calAct);
      viewMenu -> addAction(toDoAct);
      viewMenu -> addAction(timeTableAct);
      viewMenu -> addAction(bookletAct);
    
      settingsMenu = menuBar()->addMenu(tr("&Settings"));      
      settingsMenu -> addAction(settingsAct);

      ftpMenu = menuBar() -> addMenu(tr("FT&P"));
      ftpMenu -> addAction(uploadAct);
      ftpMenu -> addAction(downloadAct);
 
      helpMenu = menuBar()->addMenu(tr("&Help"));
      helpMenu->addAction(aboutAct);
      //helpMenu->addAction(aboutQtAct);
           
      contextMenu = new QMenu();
      contextMenu -> addAction(settingsAct);
      contextMenu -> addAction(aboutAct);
      contextMenu -> addAction(exitAct);
      tray -> setContextMenu(contextMenu);
}

//Set up toolbars
void qOrganizer::createToolBars()
{
     
      fileToolBar = addToolBar(tr("File")); 
      fileToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
      fileToolBar->setIconSize(QSize(64,64));
      //fileToolBar->setContentsMargins(50,0,100,0);
      fileToolBar->addAction(calAct);
      fileToolBar->addAction(toDoAct);
      fileToolBar->addAction(timeTableAct);
      fileToolBar->addAction(bookletAct); 
      fileToolBar->addAction(printAct);
      fileToolBar->addAction(saveAct);
      if(C_AUTOSAVE_TOGGLE) saveAct->setVisible(false);
      fileToolBar->addAction(downloadAct);
      fileToolBar->addAction(uploadAct);  
      fileToolBar -> addAction(settingsAct);
      fileToolBar->addAction(saveToTXTAct);       
      fileToolBar->addAction(saveToDBAct);
      fileToolBar->addAction(saveToMySQLDBAct);
      
      if(C_STORINGMODE==0)
       {
        saveToTXTAct->setVisible(false);
       }
       else 
        if(C_STORINGMODE==1)
         {
          saveToDBAct->setVisible(false);
         }
         else 
          if(C_STORINGMODE==2) 
           {
            saveToMySQLDBAct->setVisible(false);
           }
 
       if(!C_SHOW_SAVE)
        {
         saveToTXTAct->setVisible(false);
         saveToDBAct->setVisible(false);
         saveToMySQLDBAct->setVisible(false);
        }

      if(C_SYNCFTP)
       { 
        downloadAct -> setVisible(true);
        uploadAct -> setVisible(true);
       } else 
       {
        downloadAct -> setVisible(false);
        uploadAct -> setVisible(false);
       }
     
}

void qOrganizer::createStatusBar()
{
      statusBar()->showMessage(tr("Ready"));
      statusProgress = new QProgressBar(statusBar());
      statusBar() -> addPermanentWidget(statusProgress,1);
      statusProgress -> setMaximumHeight(20);
      statusProgress -> setMaximumWidth(120);
      statusProgress -> setMinimum(0);
      statusProgress -> setMaximum(0);
      statusProgress -> hide();
     
}

void qOrganizer::addCalendarPageWidgets()
{
  //widgets in the page 
  calendar=new QCalendarWidget; //The Calendar
  calendar->setGridVisible(true);
  calendar-> setFirstDayOfWeek (Qt::DayOfWeek(C_FIRST_DAY_OF_WEEK)); 
  calendar->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
  
  QDate date = calendar->selectedDate();
  selDate = date.toString(C_DT_FORMAT);
  QString Date = "<center><font size=\"4\">"+tr("Schedule for ")+selDate+":</font></center>";
  day = new QLabel(Date);

  connect(calendar, SIGNAL(selectionChanged()), this, SLOT(updateDay()));
  
  tableWid = new CQTableWidget();
  tableWid->setRowCount(C_NRROWS);
  tableWid->setColumnCount(4);
  tableWid->setParent(this);
  tableWid -> verticalHeader()->hide();
//  tableWid -> setSortingEnabled(true);
  tableWid -> setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
  
  Clabels << tr("Event") << tr("From")<< tr("Until")<<tr("Reminder");
  
  
  tableWid ->setHorizontalHeaderLabels(Clabels);
  tableWid -> setColumnWidth(0,285);
  tableWid -> setColumnWidth(1,60);
  tableWid -> setColumnWidth(2,60);
  tableWid -> setColumnWidth(3,75);  

  tableWid -> horizontalHeader() -> setStretchLastSection(true);
  tableWid -> horizontalHeader() -> setMovable(true);
  tableWid -> setItemDelegate(&schDelegate);
  tableWid -> setObjectName("tableWid");

  newButton = new QPushButton(tr("New event"));
  newButton -> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
  connect(newButton, SIGNAL(clicked()),this, SLOT(insertRowToEnd()));
 
  deleteButton = new QPushButton(tr("Delete event"));
  deleteButton -> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
  connect(deleteButton, SIGNAL(clicked()),this, SLOT(deleteRow()));
 
  textField = new CQTextEdit;
  textField->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
  textField->setAcceptDrops(true);
  fontBox = new QFontComboBox;
  /*fontBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  fontBox->adjustSize();*/
  comboSize = new QComboBox();
  comboSize->setObjectName("comboSize");
  comboSize->setEditable(true);
  //Populate the font combo box
  QFontDatabase db;
  foreach(int size, db.standardSizes())
   comboSize->addItem(QString::number(size)); 
  //Set it to 12
  comboSize->setCurrentIndex(6);
  B=new QPushButton(tr("B"));
  I=new QPushButton(tr("I"));
  U=new QPushButton(tr("U"));
  
  B->setCheckable(true);
  U->setCheckable(true);
  I->setCheckable(true);
 
  B -> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
  I -> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
  U -> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
  
  //Set the selected Font
  this -> setFont();  
  
  QString note = "<center><font size=\"4\">"+tr("Important notes, journal for ")+selDate+":</font></center>";
  noteLabel = new QLabel(note); 
  
  searchLabel = new QLabel(tr("Search:"));
  searchField = new LineEdit(this);
  searchPrevButton = new QPushButton(tr("Previous"));
  searchPrevButton -> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
  searchNextButton = new QPushButton(tr("Next"));
  searchNextButton -> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
  searchLayout = new QHBoxLayout;
  searchLayout -> addWidget(searchLabel);
  searchLayout -> addWidget(searchField);
  searchLayout -> addWidget(searchPrevButton);
  searchLayout -> addWidget(searchNextButton);
  searchCurrentIndex=-1;
}

void qOrganizer::connectCallendarPageWidgets()
{
  //Make sure that the new Font is set if the boxes change
  connect(fontBox,SIGNAL(currentFontChanged(QFont)), this, SLOT(setFont()));
  connect(comboSize,SIGNAL(currentIndexChanged(int)), this, SLOT(setFont()));

  connect(B,SIGNAL(clicked()), this, SLOT(setFont()));
  connect(U,SIGNAL(clicked()), this, SLOT(setFont()));
  connect(I,SIGNAL(clicked()), this, SLOT(setFont())); 
  
  //Change the font for every new character
  connect(textField, SIGNAL(textChanged()), this, SLOT(modifyText())); 
  //Update if calendar changed 
  connect(calendar, SIGNAL(selectionChanged()), this, SLOT(updateCalendar()));
  //Automatically Save after every change :P
  if(C_AUTOSAVE_TOGGLE)
   connect(tableWid,SIGNAL(focusLost()),this,SLOT(saveSchedule()));
  //for error checking call remind if cell changed
   connect(tableWid,SIGNAL(cellChanged(int,int)),this,SLOT(remind()));
  
  if(C_AUTOSAVE_TOGGLE)
   connect(textField,SIGNAL(focusLost()),this,SLOT(saveJurnal()));
  //If an event gets deleted save
  if(C_AUTOSAVE_TOGGLE)
  connect(deleteButton, SIGNAL(clicked()), this , SLOT(saveSchedule()));
  //Search
  connect(searchPrevButton,SIGNAL(clicked()),this,SLOT(searchPrev()));
  connect(searchNextButton,SIGNAL(clicked()),this,SLOT(searchNext()));
}

void qOrganizer::layoutCalendarPageWidgets()
{
 //The layouts for the widgets
  
  buttonLayout = new QHBoxLayout;
  buttonLayout -> addWidget(newButton);
  buttonLayout -> addWidget(deleteButton);
  
  vl = new QVBoxLayout; //vertical layout
  vl -> addWidget(calendar);
  vl -> addWidget(day);
  vl -> addWidget(tableWid);
  vl -> addLayout(buttonLayout);
  vl -> setSizeConstraint(QLayout::SetNoConstraint); 
  
  QHBoxLayout *settingsLayout = new QHBoxLayout; 
  settingsLayout ->addWidget(fontBox);
  settingsLayout ->addWidget(comboSize);
  settingsLayout ->addWidget(B);
  settingsLayout ->addWidget(I);
  settingsLayout ->addWidget(U);
  settingsLayout ->setSizeConstraint(QLayout::SetNoConstraint);
  
  textLayout = new QVBoxLayout;
  textLayout ->addWidget(noteLabel);
  textLayout ->addLayout(settingsLayout);
  textLayout ->addWidget(textField);
  textLayout ->addLayout(searchLayout);

  mainLayout = new QHBoxLayout;;
  mainLayout ->addLayout(vl);
  mainLayout ->addLayout(textLayout);

  CalendarPage= new QWidget; //Page that contains the callendar 
  CalendarPage->setLayout(mainLayout);

  mainWid->addWidget(CalendarPage);
  mainWid->setCurrentIndex(0);
  
}



void qOrganizer::addCalendarPage()
{
  
  addCalendarPageWidgets();
  if(C_STORINGMODE==0) loadJurnal(); else loadJurnalDB();
  if(C_STORINGMODE==0) loadSchedule(); else loadScheduleDB();
  connectCallendarPageWidgets();
  layoutCalendarPageWidgets();
  timer = new QTimer();
  setReminders();
}

//---------------------- TO-DO PAGE -----------------------------------

void qOrganizer::addToDoPage()
{
 
 //Widgets:

  ToDoPage=new QWidget;
  QLabel *ToDoTitle= new QLabel ("<center><font size=\"4\">"+tr("General to-do list")+"</center>");
  
  list=new CQTableWidget;
  list->setRowCount(10);
  list->setColumnCount(5);
  list->setParent(this);
  list -> verticalHeader()->hide();
  list -> setObjectName("list");
 
  TDlabels <<tr("Start")<< tr("Task") << tr("Deadline") << tr("Priority")<<tr("Completed");
  
  list ->setHorizontalHeaderLabels(TDlabels);
  list -> setColumnWidth(0,80);
  list -> setColumnWidth(1,620);
  list -> setColumnWidth(2,80);
  list -> setColumnWidth(3,80);
  list -> setColumnWidth(4,90);
  list -> horizontalHeader() -> setStretchLastSection(true);
  list -> horizontalHeader() -> setMovable(true);
  list -> setItemDelegate(&tdlistDelegate);

  newTaskButton = new QPushButton(tr("New task"));
  deleteTaskButton = new QPushButton(tr("Delete task"));
  newTaskButton -> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
  deleteTaskButton-> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
  connect(newTaskButton,SIGNAL(clicked()),this,SLOT(newTask()));
  connect(deleteTaskButton,SIGNAL(clicked()),this,SLOT(deleteTask()));
  if(C_AUTOSAVE_TOGGLE)
  connect(list,SIGNAL(focusLost()),this,SLOT(saveToDoList()));

 //Layouts:

  ToDoVl=new QVBoxLayout;
  ToDoVl->addWidget(ToDoTitle);
  ToDoVl->addWidget(list); 
  ToDoVl -> setSizeConstraint(QLayout::SetNoConstraint);

  buttonVl=new QHBoxLayout;
  buttonVl->addWidget(newTaskButton);
  buttonVl->addWidget(deleteTaskButton);
 
  mainHl = new QVBoxLayout;
  mainHl ->addLayout(ToDoVl);
  mainHl ->addLayout(buttonVl);
 
  QHBoxLayout *layout= new QHBoxLayout;
  layout->addSpacing(10);
  layout->addLayout(mainHl);
  layout->addSpacing(10);

  ToDoPage->setLayout(layout);
  mainWid->addWidget(ToDoPage);

};

//----------------------------Timetable----------------------------------

void qOrganizer::addTimeTable()
{
  timeTablePage=new QWidget;
  QLabel *timeTableTitle= new QLabel ("<center><font size=\"4\">"+tr("Timetable")+":</font></center>");
  
  table=new CQTableWidget;
  table->setRowCount(9);
  table->setColumnCount(7);
  table->setParent(this);
  //table -> verticalHeader()->hide();
  table -> setSortingEnabled(false);
  table -> setItemDelegate(&ttDelegate);
  
  TTlabels << tr("From") <<tr("Until") << tr("Monday") << tr("Tuesday") << tr("Wednesday")<<tr("Thursday")<<tr("Friday");
  
  //If no labels are loaded from file then these labels are written to it and it is retranslated
  // to the current language.
  universalTTlabels<<"From"<<"Until"<<"Monday"<<"Tuesday"<<"Wednesday"<<"Thursday"<<"Friday";
 
  table ->setHorizontalHeaderLabels(TTlabels);
  for(int i=0;i<=2;i++)
  {
   table -> setColumnWidth(i,55);
  };
  for(int i=2;i<=table->columnCount()-1;i++)
  {
   table -> setColumnWidth(i,960/(table->columnCount()-1));
  }
  table -> horizontalHeader() -> setStretchLastSection(true);

  newTTRow = new QPushButton(tr("New row"));
  newTTCmn = new QPushButton(tr("New column"));
  delTTCmn = new QPushButton(tr("Delete column"));
  delTTRow = new QPushButton(tr("Delete row"));
  newTTRow -> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
  newTTCmn -> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
  delTTRow-> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
  delTTCmn-> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
  connect(newTTRow,SIGNAL(clicked()),this,SLOT(newRow()));
  connect(delTTRow,SIGNAL(clicked()),this,SLOT(delRow()));
  connect(newTTCmn,SIGNAL(clicked()),this,SLOT(newColumn()));
  connect(delTTCmn,SIGNAL(clicked()),this,SLOT(delColumn()));
  if(C_AUTOSAVE_TOGGLE)
  connect(table,SIGNAL(focusLost()),this,SLOT(saveTimeTable()));

 //Layouts:

  Vl=new QVBoxLayout;
  Vl->addWidget(timeTableTitle);
  Vl->addWidget(table); 
  Vl -> setSizeConstraint(QLayout::SetNoConstraint);

  ttLabel = new QLabel(tr("Timetable for:"));
  ttCombo = new QComboBox;
  
  ttCombo -> addItem(tr("Odd weeks"));
  ttCombo -> addItem(tr("Even weeks"));

  
  QDate cDate = QDate::currentDate();
  if(!C_TT_REVERSORDER)
   if(cDate.weekNumber() % 2 != 0) C_TIMETABLE_INDEX=0; else C_TIMETABLE_INDEX=1;
    
  if(C_TT_REVERSORDER)
   if(cDate.weekNumber() % 2 != 0) C_TIMETABLE_INDEX=1; else C_TIMETABLE_INDEX=0;
  
  if(!C_USE_ODDTT) C_TIMETABLE_INDEX=0;

  ttCombo -> setCurrentIndex(C_TIMETABLE_INDEX);
  
  ttCombo->setFixedWidth(160);
  ttCombo -> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed); 
  calledFromTTCombo=0;
  connect(ttCombo,SIGNAL(currentIndexChanged(int)),this,SLOT(changeTTIndex(int)));

  ttLayout = new QHBoxLayout;
  ttLayout -> addWidget(ttLabel);
  ttLayout -> addWidget(ttCombo);
  ttLayout -> setAlignment(Qt::AlignRight);
  if(!C_USE_ODDTT)
   {
    ttLabel->hide();
    ttCombo->hide();
   }

  centerLayout = new QHBoxLayout;
  centerLayout->addWidget(newTTCmn);
  centerLayout->addSpacing(50);
  centerLayout->addWidget(delTTCmn);
  centerLayout->addSpacing(50);
  centerLayout->addWidget(newTTRow);
  centerLayout->addSpacing(50);
  centerLayout->addWidget(delTTRow);
  centerLayout -> setAlignment(Qt::AlignHCenter);

  BHl=new QHBoxLayout;
  BHl->addLayout(centerLayout);
  BHl->addSpacing(50);
  BHl->addLayout(ttLayout);
  BHl-> setAlignment(Qt::AlignHCenter);
  Vl->addLayout(BHl);
 
  QHBoxLayout *layout= new QHBoxLayout;
  layout->addSpacing(10);
  layout->addLayout(Vl);
  layout->addSpacing(10);

  timeTablePage->setLayout(layout);
  mainWid->addWidget(timeTablePage);
};

//-----------------------------Booklet----------------------------


void qOrganizer::addBookletPage()
{
 bookletPage=new QWidget();
 //Widgets
 QLabel *bLabel=new QLabel("<center><font size=\"4\">"+tr("Booklet")+"</font></center>");
 QLabel *markLabel = new QLabel("<center>"+tr("Marks:")+"</center>");
 QLabel *absenceLabel = new QLabel("<center>"+tr("Absences:")+"</center>");
 markTable = new CQTableWidget();
 absenceTable = new CQTableWidget();
 markTable -> setRowCount(8);
 markTable -> setColumnCount(6);
 markTable -> setParent(this);
 markTable -> horizontalHeader()->hide();
 
 
 for(int i=0;i<=6;i++)
 {
  markTable -> setColumnWidth(i,50);
 }; 
 
 subjectList <<tr("English")<<tr("Math")<<tr("IT")<<tr("History")<<tr("Geography")<<tr("Biology")<<tr("Physics")<<tr("Chemistry");
 markTable -> setVerticalHeaderLabels(subjectList);
 
 absenceTable -> setRowCount(8);
 absenceTable -> setColumnCount(5);
 absenceTable -> setParent(this);
 absenceTable -> horizontalHeader()->hide();
 absenceTable -> verticalHeader()->hide();
 absenceTable -> setItemDelegate(&absDelegate);
 
 for(int i=0;i<=absenceTable->columnCount();i++)
 {
  absenceTable -> setColumnWidth(i,630/absenceTable->columnCount());
 };  
 
 //Neded to delete cells in this, by the custom delegate
 absenceTable->setObjectName(QString("absenceTable"));

 newMarkButton = new QPushButton(tr("New mark column"));
 newSubjectButton = new QPushButton(tr("New subject"));
 deleteSubjectButton = new QPushButton(tr("Delete subject"));
 deleteMarkButton = new QPushButton(tr("Delete mark column"));
 newAbsenceButton = new QPushButton(tr("New absence column"));
 deleteAbsenceButton = new QPushButton(tr("Delete absence column"));
  
 newMarkButton -> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
 newSubjectButton -> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
 deleteMarkButton -> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
 deleteSubjectButton -> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
 newAbsenceButton -> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
 deleteAbsenceButton -> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed); 
 
 
 QLabel *averageFieldLabel = new QLabel(tr("Subject average:"));
 QLabel *absenceFieldLabel = new QLabel(tr("Absences:"));
 QLabel *markDateLabel = new QLabel(tr("Date:"));
 averageField = new QLineEdit();
 averageField->setFixedWidth(45);
 averageField->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
 absenceNrField = new QLineEdit();
 markDateField = new QDateEdit(QDate::currentDate(),this);
 markDateField -> setDisplayFormat("dd/M/yyyy");
 markDateField->setCalendarPopup(true);
 markDateField->setFixedWidth(100);
 markDateField->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed); 
 averageField -> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed); 
 absenceNrField -> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed); 
 
 totalAverageField = new QLineEdit();
 totalAverageField -> setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed); 
 totalAverageField -> setFixedWidth(45);
 QLabel *totalAverageLabel = new QLabel(tr("Total average:"));
 QLabel *voidLabel = new QLabel("  ");
 //Layouts 
 
 QHBoxLayout *averageLayout = new QHBoxLayout;
 averageLayout -> addWidget(markDateLabel);
 averageLayout -> addWidget(markDateField);
 averageLayout -> addSpacing(20);
 averageLayout -> addWidget(averageFieldLabel); 
 averageLayout -> addWidget(averageField);
 averageLayout -> addWidget(voidLabel);
 averageLayout -> addSpacing(20);
 averageLayout -> addWidget(totalAverageLabel);
 averageLayout -> addWidget(totalAverageField);

 QHBoxLayout *absenceFieldLayout = new QHBoxLayout;
 absenceFieldLayout -> addWidget(absenceFieldLabel); 
 absenceFieldLayout -> addWidget(absenceNrField);

 QVBoxLayout *markLayout = new QVBoxLayout;
 markLayout -> addWidget(markLabel);
 markLayout -> addWidget(markTable);
 markLayout -> addLayout(averageLayout); 

 QVBoxLayout *absenceLayout = new QVBoxLayout;
 absenceLayout -> addWidget(absenceLabel);
 absenceLayout -> addWidget(absenceTable);
 absenceLayout -> addLayout(absenceFieldLayout);
 

 QHBoxLayout *tableLayout = new QHBoxLayout;
 tableLayout -> addLayout(markLayout);
 tableLayout -> addSpacing(30);
 tableLayout -> addLayout(absenceLayout);

 QHBoxLayout *buttonLayout = new QHBoxLayout;
 buttonLayout -> addWidget(newMarkButton); 
 buttonLayout -> addWidget(deleteMarkButton);
 buttonLayout -> addWidget(newSubjectButton);
 buttonLayout -> addWidget(deleteSubjectButton);
 buttonLayout -> addWidget(newAbsenceButton);
 buttonLayout -> addWidget(deleteAbsenceButton);
 
 QVBoxLayout *mainLayout = new QVBoxLayout;
 mainLayout -> addWidget(bLabel);
 mainLayout -> addLayout(tableLayout); 
 mainLayout -> addLayout(buttonLayout);
 
 bookletPage->setLayout(mainLayout);
 mainWid->addWidget(bookletPage);

 //Connect buttons 
 if(C_STORINGMODE==0) loadMarksTable(); else loadMarksTableDB();
 if(C_STORINGMODE==0) loadMarkDates(); else loadMarkDatesDB();
 if(C_STORINGMODE==0) loadAbsenceTable(); else loadAbsenceTableDB();
 markTable->setCurrentCell(0,0);
 updateAverage(0,0,1,0);
 updateAbsenceNr();
 updateTotalAverage();

 connect(newSubjectButton,SIGNAL(clicked()),this,SLOT(newSubject()));
 connect(deleteSubjectButton,SIGNAL(clicked()),this,SLOT(deleteSubject()));
 connect(newMarkButton,SIGNAL(clicked()),this,SLOT(newMark()));
 connect(deleteMarkButton,SIGNAL(clicked()),this,SLOT(deleteMark()));
 connect(newAbsenceButton,SIGNAL(clicked()),this,SLOT(newAbsence()));
 connect(deleteAbsenceButton,SIGNAL(clicked()),this,SLOT(deleteAbsence()));
 if(C_AUTOSAVE_TOGGLE)
 connect(markTable,SIGNAL(focusLost()),this,SLOT(saveMarksTable()));
 if(C_AUTOSAVE_TOGGLE)
 connect(absenceTable,SIGNAL(focusLost()),this,SLOT(saveAbsenceTable()));
 
 connect(markTable,SIGNAL(currentCellChanged(int,int,int,int)),this,SLOT(updateAverage(int,int,int,int)));
 connect(markTable,SIGNAL(cellChanged(int,int)),this,SLOT(updateTotalAverage()));
 connect(absenceTable,SIGNAL(cellChanged(int,int)),this,SLOT(updateAbsenceNr()));

 connect(markTable,SIGNAL(currentCellChanged(int,int,int,int)),this,SLOT(updateDate(int,int,int,int)));
 connect(markDateField,SIGNAL(dateChanged(QDate)),this,SLOT(saveDate(QDate)));
  
 //synch scrollbars
 connect(markTable->verticalScrollBar(),SIGNAL(valueChanged(int)),absenceTable->verticalScrollBar(),SLOT(setValue(int)));
 connect(absenceTable->verticalScrollBar(),SIGNAL(valueChanged(int)),markTable->verticalScrollBar(),SLOT(setValue(int)));
};


// Let's put these all together 

void qOrganizer::addItems()
{
  QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF8"));
  QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF8"));
  setSTray();
  mainWid= new QStackedWidget; //Let's set up the Main Widget
  setCentralWidget(mainWid);  //Let's make it the Central Widget
  addCalendarPage();
  addToDoPage();  
  addTimeTable();
  addBookletPage();
  if(C_STORINGMODE==0) loadToDoList(); else loadToDoListDB();
  if(C_STORINGMODE==0) loadTimeTable(); else loadTimeTableDB();
  //Resize timetable
  for(int i=2;i<=table->columnCount()-1;i++)
   table -> setColumnWidth(i,960/(table->columnCount()-1));
  downloadingFromFTP=0;
};

//-----------------------SLOTS---------------------------------

/*This function sets the QStackedWidgets current index to 0 therefore makes the 
calendar page active*/

void qOrganizer::ChangeToCalendar()
{
 if(C_LOAD_ON_CHANGE) 
  updateCalendar();
 
 if((C_AUTOSAVE_TOGGLE)&&(C_STORINGMODE==0)) saveAll();
 mainWid->setCurrentIndex(0);
}

/*This function sets the QStackedWidgets current index to 1 therefore makes the 
to-do list page active*/

void qOrganizer::ChangetoToDoList()
{ 
 if(C_LOAD_ON_CHANGE) 
  if(C_STORINGMODE==0) loadToDoList(); else loadToDoListDB();
 if((C_AUTOSAVE_TOGGLE)&&(C_STORINGMODE==0)) saveAll();
 mainWid->setCurrentIndex(1);
}

/*This function sets the QStackedWidgets current index to 2 therefore makes the 
timetable page active*/
 
void qOrganizer::ChangeToTimeTable()
{ 
 if(C_LOAD_ON_CHANGE) 
  if(C_STORINGMODE==0) loadTimeTable(); else loadTimeTableDB();
 if((C_AUTOSAVE_TOGGLE)&&(C_STORINGMODE==0)) saveAll();
 mainWid->setCurrentIndex(2);
}


/*This function sets the QStackedWidgets current index to 3 therefore makes the 
Booklet page active*/

void qOrganizer::ChangeToBooklet()
{
 if(C_LOAD_ON_CHANGE)
 {
  if(C_STORINGMODE==0) loadMarksTable(); else loadMarksTableDB();
  if(C_STORINGMODE==0) loadAbsenceTable(); else loadAbsenceTableDB();
 }
 if((C_AUTOSAVE_TOGGLE)&&(C_STORINGMODE==0)) saveAll();
 mainWid->setCurrentIndex(3);
}

//-----------------------CALENDAR SLOTS----------------------------

//adds a new row to the schedule table, which is named tableWid (from tableWidget)
void qOrganizer::insertRowToEnd()
{
 tableWid->setRowCount(tableWid->rowCount()+1);
}

//deletes a row from the schedule table
void qOrganizer::deleteRow()
{
 tableWid->removeRow(tableWid->currentRow());
}

//Updates the two labels when the callendar changes date
void qOrganizer::updateDay()
{
  QDate date = calendar->selectedDate();
  selDate = date.toString(C_DT_FORMAT);
  QString Date ="<center><font size=\"4\">"+tr("Schedule for ")+selDate+":</font></center>";
  day->setText(Date);
  QString note ="<center><font size=\"4\">"+tr("Important notes, journal for ")+selDate+":</font></center>";
  noteLabel->setText(note);
  
}

/*TODO: Fix the BUG: The first characters font after change
  Does not get changed.Don't know why.

*/


void qOrganizer::modifyText()
{
 /*When a new character apears select it and change it's font to the currently selected font*/

 
 cursor = textField->textCursor();
 if(cursor.selectedText()=="")
 {
  cursor.movePosition(QTextCursor::Left,QTextCursor::KeepAnchor);
  //cout << "Selected text is:";
  textField->setCurrentFont(font);
  /*cout << cursor.selectedText()<<"\n";
  cout <<"Font used:";
  cout << font.rawName() <<" "<<font.pointSize()<<"\n";
  */
  cursor.movePosition(QTextCursor::Right);
 };
};

//This function sets the font for the text written in the Jurnal 
void qOrganizer::setFont() //Set the currently selected font
{
 font=fontBox->currentFont();
 font.setPointSize((comboSize->currentText()).toInt());
 //fontBox->adjustSize();
 
 if(B->isChecked())
   font.setBold(true);
  
 if(I->isChecked()) 
   font.setItalic(true);

 if(U->isChecked()) 
   font.setUnderline(true);
 
 textField->setCurrentFont(font);

 //cout << font.rawName() <<" "<<font.pointSize()<<"\n";
};


//Empty the table
void qOrganizer::emptyTable()
{
    //cout <<"Blanking the table\n";
    tableWid->clearContents();  
    tableWid->setRowCount(C_NRROWS);
   
    tableWid -> setColumnWidth(0,285);
    tableWid -> setColumnWidth(1,60);
    tableWid -> setColumnWidth(2,60);
    tableWid -> setColumnWidth(3,75);  

}

//This function changes to the directory in which the callendar stores it's data
//By default this is ~/.qOrganizer/calendar but I plan to make a setting dialog for this
void qOrganizer::setCalDir()
{
 if(C_PATH=="home")
  QDir::setCurrent(QDir::homePath());
   else QDir::setCurrent(C_PATH);
 QDir currentDir = QDir::current();
 currentDir.mkdir(".qOrganizer");  
 currentDir.cd(".qOrganizer");
 currentDir.mkdir("calendar");
 currentDir.cd("calendar");
 QDir::setCurrent(currentDir.path()); 
 QDate date = calendar->selectedDate();
 selDate = date.toString("MM_dd_yyyy")+".txt";
}


void qOrganizer::loadSchedule() //Load the daily schedule into the schedule table named tableWid
{
 if(C_STORINGMODE!=0) loadScheduleDB(); else
 {
  setCalDir(); 
  QFile scheduleFile(selDate); 
  //For writing QStrings to stdout
  QTextStream cout(stdout, QIODevice::WriteOnly);

  cout <<"Loading file:"<< selDate <<"\n";
  emptyTable();
  if (scheduleFile.open(QFile::ReadOnly)) 
  {
   QTextStream in(&scheduleFile);
   in.setCodec("UTF-8");
   int i=-1;  
   while(!in.atEnd()) 
    {
     QString line=in.readLine(); 
     i++; 
     if(line!="\n")
     {
      QStringList list = line.split("|", QString::KeepEmptyParts);
      if(i>=tableWid->rowCount())
      {
       insertRowToEnd();
       //cout<<"Adding a new row:"<<tableWid->rowCount()<<"\n";
      }
      for (int j = 0; j < list.size(); j++)
       {
        item= new QTableWidgetItem(QTableWidgetItem::Type);
        if(list.at(j)==" ")
         item->setText(""); 
          else 
           item->setText(list.at(j)); 
        //This is the way to access an item from the QStringList
        if(tableWid->item(i,j)!=0) delete tableWid->takeItem(i,j);
        tableWid->setItem(i,j,item);
      }
     }
    }  
    tableWid->setRowCount(i+1);
    scheduleFile.close();
   }
   else 
    {
     emptyTable();  
    }
 }
}

//The tables are stored in a textfile, items are separated with | 

void qOrganizer::saveSchedule() //Save the daily schedule
{
 if(C_STORINGMODE!=0) saveScheduleDB(); else
 {
  setCalDir();
  QFile scheduleFile(selDate); 
  //For writing QStrings to stdout
  QTextStream cin(stdin, QIODevice::ReadOnly);
  QTextStream cout(stdout, QIODevice::WriteOnly);
  QTextStream cerr(stderr, QIODevice::WriteOnly); 

  cout << "Saving "<<selDate<<"\n";

  if (scheduleFile.open(QFile::WriteOnly)) 
  {
   QTextStream out(&scheduleFile);
   out.setCodec("UTF-8");
   int x=tableWid->currentRow();
   int y=tableWid->currentColumn();  

   for(int i=0;i<tableWid->rowCount();i++)
    { 
     for(int j=0;j<tableWid->columnCount();j++)
      {
       tableWid->setCurrentCell(i,j); 
       item=tableWid->currentItem();
       if(item!=0)
        out << item->text()+"|";
         else out << " |";
     }
      out <<"\n";
    }
   scheduleFile.close();
   tableWid->setCurrentCell(x,y);
  }
  else {
          QMessageBox::warning(NULL, "qOrganizer", "Unable to write schedule file.", "OK");
       }
 }
}
 

void qOrganizer::updateCalendar()
{
 saveCalendarColumnWidths();
 if(C_STORINGMODE==0)
  {
   loadSchedule();
   loadJurnal();
  } 
  else
   {
    loadScheduleDB();
    loadJurnalDB();
   }
  setCalendarColumnWidths();
}

//----------------------JURNALS SLOTS FOR CALENDAR--------------------

//This saves the journal into an html file named as jurnal_currentDate.html
void qOrganizer::saveJurnal()
{
 if(C_STORINGMODE!=0) saveJurnalDB(); else 
 {
  //For writing QStrings to stdout
  QTextStream cin(stdin, QIODevice::ReadOnly);
  QTextStream cout(stdout, QIODevice::WriteOnly);
  QTextStream cerr(stderr, QIODevice::WriteOnly);

  setCalDir();
  QDate date = calendar->selectedDate();
  QString fileName ="jurnal_"+date.toString("MM_dd_yyyy")+".html";
  QFile jurnalFile(fileName); 
  cout <<"Saving journal file:\n";
  cout <<fileName<<"\n";
  if (jurnalFile.open(QFile::WriteOnly)) 
   {
    
    QTextStream out(&jurnalFile);
    out.setCodec("UTF-8");
    QString text = textField->toHtml();
    out << text; 
   }
   else
    {
      QMessageBox::warning(NULL, "qOrganizer", "Unable to write journal file.", "OK");
    }; 
  jurnalFile.close();
 }
}

//This loads the jurnal file to the CQTextEdit object named textField which is the Jurnal editor
void qOrganizer::loadJurnal()
{
 if(C_STORINGMODE!=0) loadJurnalDB(); else 
 {
  //For writing QStrings to stdout
  QTextStream cin(stdin, QIODevice::ReadOnly);
  QTextStream cout(stdout, QIODevice::WriteOnly);
  QTextStream cerr(stderr, QIODevice::WriteOnly);

  setCalDir();
  cout <<"Loading journal file:\n";
  QDate date = calendar->selectedDate();
  QString fileName ="jurnal_"+date.toString("MM_dd_yyyy")+".html";
  QFile jurnalFile(fileName); 
  cout<<fileName<<"\n";

  if (jurnalFile.open(QFile::ReadOnly)) 
  {
   QTextStream in(&jurnalFile);
   in.setCodec("UTF-8");
   QString text=in.readAll();
   textField->setHtml(text);
  }
  else
   {
    textField->setHtml("");
   };
  jurnalFile.close();
 }
}

//This sets a timer to check for reminders every 30 seconds, this way we can still do the job in a 
//CPU friendly way
void qOrganizer::setReminders()
{
  connect(timer, SIGNAL(timeout()), this, SLOT(remind()));
  timer->start(C_TIMEOUT);
};

//We need this in the remind function
bool CQTableWidget::isBeingEdited()
{
 if(this->state()==QAbstractItemView::EditingState) 
  return 1;
   else return 0;
};

//This checks if it has to remind the user of an event
void qOrganizer::remind()
{
  //This is for debuging purposes only
  QTextStream cout(stdout, QIODevice::WriteOnly); 
  //We remember the coordinates of the current cell to set it back
  int x=tableWid->currentRow();
  int y=tableWid->currentColumn();  
  /*We need to set the calendar to the date of today to load all reminders for today  
   but this could be annoying for the user in a maximized state so we do it only if minimized
   just to be safe
   This is implemented in the reimplementation of hideEvent.
   Everytime the app gets minimized it returns to the current day so reminders apply only for that
   day and nothing is missed.
 */

 //We can't parse the table for data if the user is editing a cell because it would get it all //selected and when the user types a new character it would be overwritten, the reminder column is 
 //an exception because we need to check for errors and report them right away.
 if((!tableWid->isBeingEdited())||(y==3))
 {  
 //Parse the table for reminders
  for(int i=0;i<=tableWid->rowCount();i++)
   {    
       tableWid->setCurrentCell(i,3); 
       item=tableWid->currentItem();
       if(item!=0)
        {  
          QString timeString = item->text();
          tableWid->setCurrentCell(i,0); 
          item=tableWid->currentItem();
          QString eventString;
          if(item!=0)
            eventString = item->text();
             else eventString ="";
           
          //This is because there are to excepted formats for the timeString
          // Example: 12:00 is equal to 12
          if(timeString.indexOf(":")!=-1)
           {
             //Cut the string and make it into a QTime object
             if(timeString.size()==4)
              timeString="0"+timeString;
             QString hourString=timeString;
             hourString.remove(hourString.indexOf(":"),3); 
             
             QString minuteString=timeString;
             minuteString.remove(0,3);
             bool ok;
             bool ok2;
             int hour = hourString.toInt(&ok,10);
             int minute = minuteString.toInt(&ok2,10);
             if((ok&&ok2)&&(hour<24)&&(hour>=0)&&(minute>=0)&&(minute<=60))
             {
              QTime time = QTime(hour,minute,0,0);
              QDate date = calendar -> selectedDate();
              if((time<QTime::currentTime())&&(date==QDate::currentDate()))
               { 
                 if(C_SOUND) playSound(); 
                 tableWid->setCurrentCell(i,3); 
                 item=tableWid->currentItem();
                 item -> setText("");
                 //To avoid phantom reminders
                 if(C_STORINGMODE==0) saveSchedule(); else saveScheduleDB();
                 QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::MessageIcon(QSystemTrayIcon::Information);                
                /*Pops up a baloon in the system tray notifying the user about the event
                 This stays active for 2 hours (7200 seconds), I guess that's enough for the user to
                 notice it.*/
                 if(C_BALOON_SET)
                  tray->showMessage("qOrganizer",eventString,icon,7200000);      
                   else
                      {
                       if(!this->isVisible()) toggleVisibility();
                       QMessageBox::about(this,"qOrganizer", eventString); 
                      }
                      //This will look pretty and not block the sound since it's not modal.
                 tableWid->setCurrentCell(i,3); 
                 item=tableWid->currentItem();
                 item -> setText("");
                 //To avoid phantom reminders
                 if(C_STORINGMODE==0) saveSchedule(); else saveScheduleDB();
              }
             } 
             else 
              {
               QMessageBox::warning(NULL, "qOrganizer",tr("Couldn't set reminder!\nThere are two excepted formats for reminders.\nOne is hh:mm.\nExample:17:58.\nOr the other one is just writing the hour.\nExample: 3 is equal to 3:00 or to 03:00" ) , "OK");
               tableWid->setCurrentCell(i,3); 
               item=tableWid->currentItem();
               item -> setText("");
               //To avoid phantom reminders
               if(C_STORINGMODE==0) saveSchedule(); else saveScheduleDB();
             }
           }
           else
            {
              if((timeString.size()==2)||(timeString.size()==1))
               {
                 bool ok;
                 int hour = timeString.toInt(&ok,10);
                 if((ok)&&(hour<24)&&(hour>=0)) 
                  {
                     QTime time = QTime(hour,0,0,0);
                     QDate date = calendar -> selectedDate();
                     if((time<QTime::currentTime())&&(date==QDate::currentDate()))
                      {                      
                        if(C_SOUND) playSound(); 
                        tableWid->setCurrentCell(i,3); 
                        item=tableWid->currentItem();
                        item -> setText("");
                        //To avoid phantom reminders
                        if(C_STORINGMODE==0) saveSchedule(); else saveScheduleDB();
                        QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::MessageIcon(QSystemTrayIcon::Information);
                        if(C_BALOON_SET)
                         tray->showMessage("qOrganizer",eventString,icon,7200000);     
                         else
                           {
                            if(!this->isVisible()) toggleVisibility();
                            QMessageBox::about(this,"qOrganizer", eventString); 
                           }
                     
                          //This will look pretty and not block the sound since it's not modal.                 
                      }
                  } 
                   else 
                    {
                      if((timeString.size()!=0)&&(timeString!=" ")&&(timeString!="  ")
                                                            &&(timeString!="   ")) 
                       {
                        QMessageBox::warning(NULL, "qOrganizer",tr("Couldn't set reminder!\nThere are two excepted formats for reminders.\nOne is hh:mm.\nExample:17:58.\nOr the other one is just writing the hour.\nExample: 3 is equal to 3:00 or to 03:00" ) , "OK");
                        tableWid->setCurrentCell(i,3); 
                        item=tableWid->currentItem();
                        item -> setText("");          
                        //To avoid phantom reminders
                        if(C_STORINGMODE==0) saveSchedule(); else saveScheduleDB();
                       }
                    } 
                    
               }
                else 
                {
                 if((timeString.size()!=0)&&(timeString!=" ")&&(timeString!="  ")
                                                            &&(timeString!="   ")) 
                  {
                   QMessageBox::warning(NULL, "qOrganizer",tr("Couldn't set reminder!\nThere are two excepted formats for reminders.\nOne is hh:mm.\nExample:17:58.\nOr the other one is just writing the hour.\nExample: 3 is equal to 3:00 or to 03:00" ) , "OK");
                   tableWid->setCurrentCell(i,3); 
                   item=tableWid->currentItem();
                   item -> setText("");        
                   //To avoid phantom reminders
                   if(C_STORINGMODE==0) saveSchedule(); else saveScheduleDB();
                  }
                } 
            }
        };
   };
 tableWid -> setCurrentCell(x,y);
 };
};


//---------------------------------------SIGNALS---------------------------------------

//Reimplementation of focusOutEvent to emit a signal we can catch to know
//when to save
//AUTOSAVE works on focusOutEvents so it consumes less cpu then connecting it to 
//cellChanged(int,int)
void CQTableWidget::focusOutEvent( QFocusEvent * event )
{
 if((event->reason()==Qt::MouseFocusReason) ||
    (event->reason()==Qt::TabFocusReason)||
    (event->reason()==Qt::ActiveWindowFocusReason))
 {
  emit focusLost();
 }
 
}

void CQTextEdit::focusOutEvent( QFocusEvent * event )
{
 if((event->reason()==Qt::MouseFocusReason) ||      
    (event->reason()==Qt::TabFocusReason) ||
    (event->reason()==Qt::ActiveWindowFocusReason))
 {
  emit focusLost();
 }
}

bool CQTextEdit::canInsertFromMimeData( const QMimeData *source ) const
 {
     if (source->hasFormat("text/uri-list"))
         return true;
     else
         return QTextEdit::canInsertFromMimeData(source);
 }

void CQTextEdit::insertFromMimeData( const QMimeData *source )
 {

     if (source->hasFormat("text/uri-list"))
     {
         QTextCursor cursor = this->textCursor();
         QTextDocument *document = this->document();
         QString origin = source->urls()[0].toLocalFile();
         #ifdef Q_OS_LINUX  //toLocalFile() doesn't work
          origin = source->urls()[0].toString();
          origin.remove("file://");
         #endif
         #ifdef Q_OS_WIN32
          origin = source->urls()[0].toString();
          origin.remove("file:///");
         #endif
         QImage *image = new QImage(origin); 
         if(!image->isNull())
          {
           document->addResource(QTextDocument::ImageResource,origin,image);
           cursor.insertImage(origin);
          }
         delete image;
     } else QTextEdit::insertFromMimeData(source);
 }

//-----------------------------------------LINEEDIT WITH CLEAR BUTTON----------------------------------------
LineEdit::LineEdit(qOrganizer *parent) : QLineEdit(parent)
{
 clearButton = new QToolButton(this);
 Parent = parent;
 QPixmap pixmap(":/images/clear.png");
 clearButton->setIcon(QIcon(pixmap));
 clearButton->setIconSize(pixmap.size());
 clearButton->setCursor(Qt::ArrowCursor);
 clearButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
 clearButton->hide();
 connect(clearButton, SIGNAL(clicked()), this, SLOT(clearAndSetDateBack()));
 connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(updateCloseButton(const QString&)));
 int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
 setStyleSheet(QString("QLineEdit { padding-right: %1px; } ").arg(clearButton->sizeHint().width() + frameWidth + 1));
 QSize msz = minimumSizeHint();
 setMinimumSize(qMax(msz.width(), clearButton->sizeHint().height() + frameWidth * 2 + 2),
                qMax(msz.height(), clearButton->sizeHint().height() + frameWidth * 2 + 2));
}

void LineEdit::resizeEvent(QResizeEvent *)
{
 QSize sz = clearButton->sizeHint();
 int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
 clearButton->move(rect().right() - frameWidth - sz.width(),
                  (rect().bottom() + 1 - sz.height())/2);
}

void LineEdit::updateCloseButton(const QString& text)
{
 clearButton->setVisible(!text.isEmpty());
}

void LineEdit::clearAndSetDateBack()
{
 this->clear();
 Parent->ChangeToCalendar();
 Parent->calendar->setSelectedDate(QDate::currentDate());
 Parent->updateDay(); 
 if(Parent->C_STORINGMODE==0) Parent->loadJurnal(); else Parent->loadJurnalDB();
 if(Parent->C_STORINGMODE==0) Parent->loadSchedule(); else Parent->loadScheduleDB();
 Parent->setCalendarColumnWidths();
}

//------------------------TO-DO LIST SLOTS--------------------------

void qOrganizer::newTask()
{
 list->setRowCount(list->rowCount()+1);
}

void qOrganizer::deleteTask()
{
 list->removeRow(list->currentRow());
};

//Sets the todo list directory 
void qOrganizer::setTDDir()
{
 if(C_PATH=="home")
  QDir::setCurrent(QDir::homePath ());
   else QDir::setCurrent(C_PATH);
 QDir currentDir = QDir::current();
 currentDir.mkdir(".qOrganizer");  
 currentDir.cd(".qOrganizer");
 currentDir.mkdir("todo");
 currentDir.cd("todo");
 QDir::setCurrent(currentDir.path());
}

//Saves the to-do list
void qOrganizer::saveToDoList()
{ 
 if(C_STORINGMODE!=0) saveToDoListDB(); else
 {
  setTDDir();
  QFile todoFile("todo.txt"); 
  cout <<"Saving to-do list file:\n";

   if (todoFile.open(QFile::WriteOnly)) 
   {
    QTextStream out(&todoFile);
    out.setCodec("UTF-8");
    int x=list->currentRow();
    int y=list->currentColumn();  
    for(int i=0;i<list->rowCount();i++)
     { 
      for(int j=0;j<list->columnCount();j++)
       {
        list->setCurrentCell(i,j); 
        item=list->currentItem();
        if(item!=0)
         out << item->text()+"|";
          else out << " |";
       }
      out <<"\n";
      }
    todoFile.close();
    list->setCurrentCell(x,y); 
   }
   else 
    {
      QMessageBox::warning(NULL, "qOrganizer", "Unable to write To-Do list file.", "OK");
    }
 }
}

//Loads the to-do list
void qOrganizer::loadToDoList()
{
 if(C_STORINGMODE!=0) loadToDoListDB(); else 
 {
  list -> setSortingEnabled(false);
  list->clearContents();
  list->setRowCount(0);
  setTDDir();
  QFile todoFile("todo.txt"); 
  if (todoFile.open(QFile::ReadOnly)) 
  {
   cout << "Loading to-do list\n";
   QTextStream in(&todoFile);
   in.setCodec("UTF-8");
   int i=-1;
   while(!in.atEnd()) 
    {
     QString line=in.readLine(); 
     i++; 
     if(line!="\n")
     {
      QStringList slist = line.split("|", QString::KeepEmptyParts);
      if(i>=list->rowCount())
      {
       newTask();
       cout<<"TODO:Adding a new row:"<<list->rowCount()<<"\n";
      }; 
      for (int j = 0; j < slist.size(); j++)
       {
        item= new QTableWidgetItem(QTableWidgetItem::Type);
        item->setText(slist.at(j));
        if(list->item(i,j)!=0) delete list->takeItem(i,j);
        list->setItem(i,j,item);
       };
      }
     }  
   list->setRowCount(i+1);
   if(i==-1) list->setRowCount(10);
  }
   else 
    {
     cout << "Could not open to-do list file\n";
     list->setRowCount(10);
    }
  todoFile.close();
  list -> setSortingEnabled(true);
 }
}
  

//-------------------------TIMETABLE SLOTS---------------------

void qOrganizer::newRow()
{
 table->setRowCount(table->rowCount()+1);
};

void qOrganizer::delRow()
{
 table->removeRow(table->currentRow());
};

void qOrganizer::delColumn()
{ 
 TTlabels.removeAt(table->currentColumn());
 englishList.removeAt(table->currentColumn());
 table->removeColumn(table->currentColumn());
}

//This brings up a dialog and asks for a dayname to add as a column because in some countryes
//they have school even saturday.Doesn't that suck :( ?

void qOrganizer::newColumn()
{
  bool ok;
  QString text = QInputDialog::getText(this,
                                       "",
                                       tr("Column name:"), QLineEdit::Normal,
                                       tr("Saturday"), &ok,
                                       Qt::Dialog
                                      );
  if (!ok || text.isEmpty()) 
    { 
      text=tr("Saturday");
    };
 
 table->setColumnCount(table->columnCount()+1);
 TTlabels<<text;
 englishList << text;
 table->setHorizontalHeaderLabels(TTlabels);
 for(int i=2;i<=table->columnCount()-1;i++)
  table -> setColumnWidth(i,960/(table->columnCount()-1));
}

//Set's the directory for the Timetable
void qOrganizer::setTTDir()
{
 if(C_PATH=="home")
  QDir::setCurrent(QDir::homePath ());
   else QDir::setCurrent(C_PATH);
 QDir currentDir = QDir::current();
 currentDir.mkdir(".qOrganizer");  
 currentDir.cd(".qOrganizer");
 currentDir.mkdir("timetable");
 currentDir.cd("timetable");
 QDir::setCurrent(currentDir.path());
};

//Saves the timetable
void qOrganizer::saveTimeTable()
{
 if(C_STORINGMODE!=0) saveTimeTableDB(); else
 {
  setTTDir();
  QString ttFileName;
  if(C_TIMETABLE_INDEX==0) ttFileName="timetable.txt"; else ttFileName="timetable_even.txt";
  QFile timeTableFile(ttFileName); 
  cout <<"Saving timetable file\n";
  if (timeTableFile.open(QFile::WriteOnly)) 
  {
   QTextStream out(&timeTableFile);
   out.setCodec("UTF-8");
   int x=table->currentRow();
   int y=table->currentColumn();
   //Save column names
   if(!englishList.isEmpty())
    for(int i=0;i<englishList.size();i++)
      out << englishList.at(i)<<"|";
    else 
     {
      for(int i=0;i<universalTTlabels.size();i++)
       out << universalTTlabels.at(i)<<"|";
      englishList = universalTTlabels;
     }
   out<<"\n";  

   for(int i=0;i<table->rowCount();i++)
    { 
     for(int j=0;j<table->columnCount();j++)
      {
       table->setCurrentCell(i,j); 
       QTableWidgetItem *item=table->currentItem();
       if(item!=0)
        out << item->text()+"|";
         else out <<" |";
      }
     out <<"\n";
     }
   timeTableFile.close();
   table->setCurrentCell(x,y); 
  }
  else 
   {
     QMessageBox::warning(NULL, "qOrganizer", "Unable to write timetable file.", "OK");
   }
 }
}

//Loads the timetable
void qOrganizer::loadTimeTable()
{
 QTextStream cout(stdout, QIODevice::WriteOnly);

 if(C_STORINGMODE!=0) loadTimeTableDB(); else 
 {
  setTTDir();
  QString ttFileName;
  if(C_TIMETABLE_INDEX==0) ttFileName="timetable.txt"; else ttFileName="timetable_even.txt";
  QFile timeTableFile(ttFileName); 
  if (timeTableFile.open(QFile::ReadOnly)) 
  {
   cout << "Loading timetable:"<<ttFileName<<"\n";
   QTextStream in(&timeTableFile);
   in.setCodec("UTF-8");
   int i=-1;
   QString firstline=in.readLine(); 
   QStringList labelsList = firstline.split("|", QString::SkipEmptyParts);
   englishList = labelsList;
   for(int i=0;i<labelsList.size();i++)
    {
     QString temp = tr(labelsList.at(i).toUtf8().data());
     labelsList.replace(i,temp);
    }
   table->setColumnCount(labelsList.size()); 
   table->setHorizontalHeaderLabels(labelsList);
   TTlabels = labelsList;

   while(!in.atEnd()) 
    {
     QString line=in.readLine(); 
     i++; 
     if(line!="\n")
     {
      QStringList slist = line.split("|", QString::KeepEmptyParts);
      if(i>=table->rowCount())
      {
       newRow();
       cout<<"TimeTable:Adding a new row:"<<table->rowCount()<<"\n";
      }; 
      for (int j = 0; j < slist.size(); j++)
       {
        item= new QTableWidgetItem(QTableWidgetItem::Type);
        item->setText(slist.at(j));
        if(table->item(i,j)!=0) delete table->takeItem(i,j);
        table->setItem(i,j,item);
       }
      }
     }  
    table->setRowCount(i+1);
  }
  timeTableFile.close();
 }
}

//------------------------------------------BOOKLET SLOTS------------------------------------------

//New Column 
void qOrganizer::newMark()
{
 markTable->setColumnCount(markTable->columnCount()+1);
 markTable->setColumnWidth(markTable->columnCount()-1,50);
};

//New row with custom header
void qOrganizer::newSubject()
{
  bool ok;
  QString text = QInputDialog::getText(this,
                                       "",
                                       tr("Subject name:"), QLineEdit::Normal,
                                       tr(""), &ok,
                                       Qt::Dialog
                                      );
  if (ok || !text.isEmpty()) 
    { 
      markTable->setRowCount(markTable->rowCount()+1);
      absenceTable->setRowCount(markTable->rowCount());
      subjectList<<text;
      markTable->setVerticalHeaderLabels(subjectList);
    };

};

//Remove column
void qOrganizer::deleteMark()
{
  markTable -> removeColumn(markTable->currentColumn());
};

//Remove row, this also affects the absence table and also deletes it from the QStringList called
//subjectList
void qOrganizer::deleteSubject()
{
 int pos = markTable->currentRow();
 markTable -> removeRow(markTable->currentRow());
 absenceTable->removeRow(markTable->currentRow()+1);
 subjectList.removeAt(pos);
};

//New column in the absence table
void qOrganizer::newAbsence()
{
  absenceTable->setColumnCount(absenceTable->columnCount()+1);
  absenceTable->setColumnWidth(absenceTable->columnCount()-1,80);
};

//Remove column from the absence table
void qOrganizer::deleteAbsence()
{
 absenceTable->removeColumn(absenceTable->currentColumn());
}; 

void qOrganizer::setBDir() //set booklet directory
{
 if(C_PATH=="home")
  QDir::setCurrent(QDir::homePath ());
   else QDir::setCurrent(C_PATH);
 QDir currentDir = QDir::current();
 currentDir.mkdir(".qOrganizer");  
 currentDir.cd(".qOrganizer");
 currentDir.mkdir("booklet");
 currentDir.cd("booklet");
 QDir::setCurrent(currentDir.path());
};

//Saves the contents of the markTable into a textfile
//The first line contains the vertical headers (subjects) of the marktable separated with |
//The other lines contain the marks separated with |
void qOrganizer::saveMarksTable()
{
 if(C_STORINGMODE!=0) saveMarksTableDB(); else 
 {
  setBDir();
  QFile markFile("marks.txt"); 
  cout <<"Saving marks file\n";

  if (markFile.open(QFile::WriteOnly)) 
  {
   QTextStream out(&markFile);
   out.setCodec("UTF-8");
   int x=markTable->currentRow();
   int y=markTable->currentColumn();
   //Save row names
   for(int i=0;i<subjectList.size();i++)
    {
      out << subjectList.at(i)<<"|";
    };
   out<<"\n";  
 
   for(int i=0;i<markTable->rowCount();i++)
    { 
     for(int j=0;j<markTable->columnCount();j++)
      {
       markTable->setCurrentCell(i,j); 
       QTableWidgetItem *item=markTable->currentItem();
       if(item!=0)
        out << item->text()+"|";
         else out <<" |";
      }
     out <<"\n";
     }
   markFile.close();
   markTable->setCurrentCell(x,y); 
  }
  else 
   {
     QMessageBox::warning(NULL, "qOrganizer", "Unable to write marks into file.", "OK");
   }
 }
}

/*
  Here I kind of had a problem with QStringList::split, it worked buggy and I couldn't figure it out
  why so I wrote my own using stl strings and vector
*/

void qOrganizer::splitMyString(QString s) 
//According to a trace this had extreme memory problems under 64 bit, and therefore is not used anymore
{
    v.clear();
    string str = s.toStdString();
    string delim="|";
    unsigned int offset = 0;
    unsigned int delimIndex = 0;
    
    delimIndex = str.find(delim, offset);

    while (delimIndex != string::npos)
    {
        v.push_back(str.substr(offset, delimIndex - offset));
        offset += delimIndex - offset + delim.length();
        delimIndex = str.find(delim, offset);
    }

    v.push_back(str.substr(offset));
}


//Loads the marks in markTable
//v is the vector in which the string get's splitted
void qOrganizer::loadMarksTable()
{
 if(C_STORINGMODE!=0) loadMarksTableDB(); else
 {
  setBDir();
  QFile markFile("marks.txt"); 
  /* //For writing QStrings to stdout
  QTextStream cin(stdin, QIODevice::ReadOnly);
  QTextStream cout(stdout, QIODevice::WriteOnly);
  QTextStream cerr(stderr, QIODevice::WriteOnly);*/

  if (markFile.open(QFile::ReadOnly)) 
  {
   cout << "Loading marks!\n";
   QTextStream in(&markFile);
   in.setCodec("UTF-8");
   int i=-1;
   QString firstline=in.readLine(); 
   QStringList labelsList = firstline.split("|", QString::SkipEmptyParts); //Here it works, funny 
   markTable->setRowCount(labelsList.size()); 
   absenceTable->setRowCount(labelsList.size());
   markTable->setVerticalHeaderLabels(labelsList);
   subjectList=labelsList;
   QStringList slist;

   while(!in.atEnd()) 
    {
     QString line=in.readLine(); 
     i++;
     //splitMyString(line); Let's try something new because splitMyString is causing problems on 64bit
     QStringList v = line.split("|",QString::KeepEmptyParts);
     markTable->setColumnCount(v.size()-1); 
    //sets the Column count to the number of elements in a line of the textfile
     for(int j=0;j<v.size();j++)
      { 
       item= new QTableWidgetItem(QTableWidgetItem::Type);
       item->setText(v[j]);
       if(markTable->item(i,j)!=0) delete markTable->takeItem(i,j);
       markTable->setItem(i,j,item); 
      }
    }  
  }
  markFile.close();
  for(int a=0;a<markTable->columnCount();a++)
         markTable->setColumnWidth(a,50);
 }
}

//Save absences same format absence dates separated with |
//Example: 20.03.2007|13.01.2007|08.05.2007|
void qOrganizer::saveAbsenceTable()
{
 if(C_STORINGMODE!=0) saveAbsenceTableDB(); else 
 {
  setBDir();
  QFile absenceFile("absences.txt"); 
  cout <<"Saving absences into file\n";

  if (absenceFile.open(QFile::WriteOnly)) 
  {
   QTextStream out(&absenceFile);
   out.setCodec("UTF-8");
   int x=absenceTable->currentRow();
   int y=absenceTable->currentColumn();
      
   for(int i=0;i<absenceTable->rowCount();i++)
    { 
     for(int j=0;j<absenceTable->columnCount();j++)
      {
       absenceTable->setCurrentCell(i,j); 
       QTableWidgetItem *item=absenceTable->currentItem();
       if(item!=0)
        out << item->text()+"|";
         else out <<" |";
      }
     out <<"\n";
     }
   absenceFile.close();
   absenceTable->setCurrentCell(x,y); 
  }
  else 
   {
     QMessageBox::warning(NULL, "qOrganizer", "Unable to write absences into file.", "OK");
   }
 }
}

//Load the absences into absenceTable
void qOrganizer::loadAbsenceTable()
{ 
 if(C_STORINGMODE!=0) loadAbsenceTableDB(); else 
 {
  setBDir();
  QFile absenceFile("absences.txt"); 
  if (absenceFile.open(QFile::ReadOnly)) 
  {
   cout << "Loading absences!\n";
   QTextStream in(&absenceFile);
   in.setCodec("UTF-8");
   int i=-1;
   QStringList slist;
   while(!in.atEnd()) 
    {
     QString line=in.readLine(); 
     i++;
     //splitMyString();
     QStringList v = line.split("|",QString::KeepEmptyParts);
     absenceTable->setColumnCount(v.size()-1);
     for (int j = 0; j < v.size(); j++)
       {
        item= new QTableWidgetItem(QTableWidgetItem::Type);
        item->setText(v[j]);
        if(absenceTable->item(i,j)!=0) delete absenceTable->takeItem(i,j);
        absenceTable->setItem(i,j,item);
       };
     }
   }  
 
  absenceFile.close();
  for(int i=0;i<absenceTable->columnCount();i++)
        absenceTable->setColumnWidth(i,80);
 }   
}

double qOrganizer::calculateRowAverage(int row)
{
   double average; 
   double sum=0; 
   double nr=0;

   for(int j=0;j<markTable->columnCount();j++)
       {
        markTable->setCurrentCell(row,j); 
        QTableWidgetItem *item=markTable->currentItem();
        if(item!=0)
         {
          sum=sum+(item->text()).toFloat();
          if((item->text()).toFloat()!=0) nr++;
         }
       }
   if(nr!=0) average=sum/nr;
    else average=0;

 
 return average;
};

//This recalculates the average of the marks in markTable when the current row changes
//nr=new row, nc=new column, pr=previous row,pc=previous column

void qOrganizer::updateAverage(int nr,int nc,int pr,int pc)
{
 Q_UNUSED(nc);
 Q_UNUSED(pc);
 if(nr!=pr)
 {
  
  int x=markTable->currentRow();
  int y=markTable->currentColumn();
  double average = calculateRowAverage(x);
  markTable->setCurrentCell(x,y); 
  //cout<<"Average:"<<average<<"/"<<nr<<"\n";
  QString averageString = QString::number(average);
  if(averageString.size()>4) averageString.resize(4);
  averageField->setText(averageString);
 }
}

//This recalculates the total average based on all data
void qOrganizer::updateTotalAverage()
{
  int x=markTable->currentRow();
  int y=markTable->currentColumn();

  double sum=0;
  double nr = markTable->rowCount();

  for(int i=0;i<markTable->rowCount();i++)
  {
   double result = calculateRowAverage(i);
   if(C_ROUND_ON_AVERAGE) result=round(result);
   if(result==0) nr--;
   sum=sum+result;
  };
  
  double average;
  if(nr!=0) average=sum/nr;
   else average=0;
  cout << "Total average:"<<sum<<"/"<<nr<<"="<<sum/nr<<"\n"; 
  QString averageString = QString::number(average);
  if(averageString.size()>4) averageString.resize(4);
  totalAverageField->setText(averageString);
  markTable->setCurrentCell(x,y); 
};

//This recounts the absences in absenceTable when that looses focus
void qOrganizer::updateAbsenceNr()
{
 int nr=0;

 int x=absenceTable->currentRow();
 int y=absenceTable->currentColumn();
 
 for(int i=0;i<absenceTable->rowCount();i++)
    { 
     for(int j=0;j<absenceTable->columnCount();j++)
      {
       absenceTable->setCurrentCell(i,j); 
       QTableWidgetItem *item=absenceTable->currentItem();
       if(item!=0)
        {
         if(((item->text()).size()!=0)&&(item->text()!=" ")&&(item->text()!="  ")) nr++;
        }
      }
     }
 
 absenceTable->setCurrentCell(x,y); 
 absenceNrField->setText(QString::number(nr));
};

//Close event, save on close and quit

void qOrganizer::closeEvent(QCloseEvent *e)
{
 saveColumnWidths();
 if((C_AUTOSAVE_TOGGLE)&&(!downloadingFromFTP)) saveAll();
 writeSettings();
 
 if(C_SYSTRAY)
  { 
   e->ignore();
   hide();
   if(!downloadingFromFTP)
   {
    ChangeToCalendar();
    calendar->setSelectedDate(QDate::currentDate());
    updateDay(); 
    if(C_STORINGMODE==0) loadJurnal(); else loadJurnalDB();
    if(C_STORINGMODE==0) loadSchedule(); else loadScheduleDB();
    setCalendarColumnWidths();
   }
  }
  else
     {
      e->accept();
      tray->hide();
      qApp->quit();
     }

}

//---------------------------------SYSTEM TRAY------------------------


//Sets the system tray Icon
void qOrganizer::setSTray()
{
 tray = new QSystemTrayIcon( this );
 tray->setIcon ( QIcon(":/images/icon.png") );
 connect(tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
this,SLOT(trayActivated(QSystemTrayIcon::ActivationReason)));
 tray->show();
}

//reimplementation of event
void qOrganizer::trayActivated(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::Trigger)
		toggleVisibility();
}

//You guessed it toggles the visibility of the window
void qOrganizer::toggleVisibility()
{
	if(isHidden())
	{
		show();
		if(isMinimized())
		{
			if(isMaximized())
				showMaximized();
			else
				showNormal();
		}
		raise();
		activateWindow();
	}
	else
	{
		hide(); 
                saveColumnWidths();
                if(!downloadingFromFTP)
                {
                 cout << "Not Downloading so it's ok to load and save\n"; 
                 if(C_AUTOSAVE_TOGGLE) saveAll(); 
   		 ChangeToCalendar();
   		 calendar->setSelectedDate(QDate::currentDate());
   		 updateDay(); 
   		 if(C_STORINGMODE==0) loadJurnal(); else loadJurnalDB();
   		 if(C_STORINGMODE==0) loadSchedule(); else loadScheduleDB();
                 setCalendarColumnWidths();
                }
	}
};

//This catches minimize events and saves when the window gets minimized and also hides
//the window entry in the taskbar so the program sits in the system tray
void qOrganizer::hideEvent(QHideEvent * event)
{
 Q_UNUSED(event);
 if(this->isMinimized())
  {
   if(!downloadingFromFTP)
   {
    if(C_AUTOSAVE_TOGGLE) saveAll();
    //Set the calendar back to the current day to make sure reminders will work
    ChangeToCalendar();
    calendar->setSelectedDate(QDate::currentDate());
    updateDay(); 
    if(C_STORINGMODE==0) loadJurnal(); else loadJurnalDB();
    if(C_STORINGMODE==0) loadSchedule(); else loadScheduleDB();
    setCalendarColumnWidths();
   } 
    saveColumnWidths();
  }
}

//-------------------------Printing----------------------------

//formats a document with the content of the callendar using a hidden QTextEdit widget and prints it
void qOrganizer::printCalendar()
{
  QTextEdit *hidden=new QTextEdit;
  hidden->setParent(this);
  hidden->show();
  //We construct a string in html and we use setHtml with the hidden QTextEdit
  QString text=day->text();
  text.append("<br>");
  //Now for the schedule table 
  
  int x=tableWid->currentRow();
  int y=tableWid->currentColumn();  //We remember the current column to set it back 

  text.append("<table border=2>");
  text.append("<font size=7>");

  text.append("<tr>");

  for(int i=0;i<Clabels.size();i++)
    { 
     text.append("<td>");
     text.append(Clabels.at(i));
     text.append("</td>");
    };

  text.append("</tr>");

  for(int i=0;i<tableWid->rowCount();i++)
   { 
    text.append("<tr>");

    for(int j=0;j<tableWid->columnCount();j++)
     {
      tableWid->setCurrentCell(i,j); 
      item=tableWid->currentItem();
      if(item!=0)
        text.append("<td>"+item->text()+" </td>");
         else
          text.append("<td> </td>");
     }
    text.append(" </tr><br>");
   }

  tableWid->setCurrentCell(x,y);
  text.append("</font></table><br><br>");
  //Jurnal
  text.append(noteLabel->text()+"<br><br><br>");
  text.append(textField->toHtml());

  hidden->setHtml(text);
  QTextDocument *document = hidden->document();
  hidden->hide();
  document->print(printer);
  delete printer;
  delete hidden;
};

//Prints the to-do list under the same mechanism as in printCalendar()
void qOrganizer::printToDoList()
{ 
  QTextEdit *hidden=new QTextEdit;
  hidden->setParent(this);
  hidden->show();
  QString text;
  int x=list->currentRow();
  int y=list->currentColumn();  

  text="<center><font size=\"5\" >"+tr("General to-do list:")+"</center><br><br>";

  text.append("<center><table border=\"2\">");
  text.append("<font size=\"7\">");
  text.append("<tr>");

  for(int i=0;i<TDlabels.size();i++)
    { 
     text.append("<td>");
     text.append(TDlabels.at(i));
     text.append("</td>");
    };

  text.append("</tr>");

  for(int i=0;i<list->rowCount();i++)
   { 
    text.append("<tr>");

    for(int j=0;j<list->columnCount();j++)
     {
      list->setCurrentCell(i,j); 
      item=list->currentItem();
      if(item!=0)
        text.append("<td>"+item->text()+" </td>");
         else
          text.append("<td> </td>");
     }
    text.append(" </tr><br>");
   }

  list->setCurrentCell(x,y);
  text.append("</font></table></center><br><br>");  

  hidden -> setHtml(text);
  QTextDocument *document = hidden->document();
  hidden->hide();
  document->print(printer);
  delete printer;
  delete hidden;
};

//Prints the timetable
void qOrganizer::printTimeTable()
{
  QTextEdit *hidden=new QTextEdit;
  hidden->setParent(this);
  hidden->show();
  QString text;
  int x=table->currentRow();
  int y=table->currentColumn();  

  text = "<center><table border=\"0\"><tr><td>";
  text.append("<center><font size=\"5\">"+tr("Timetable")+":</font></center>");
  text.append("</td></tr><tr>");
  text.append("<br><table border=\"1\">");
  text.append("<font size=\"7\">");
  text.append("<tr>");

  for(int i=0;i<TTlabels.size();i++)
    { 
     text.append("<td>");
     text.append(TTlabels.at(i));
     text.append("</td>");
    };

  text.append("</tr>");

  for(int i=0;i<table->rowCount();i++)
   { 
     text.append("<tr>");

    for(int j=0;j<table->columnCount();j++)
     {
      table->setCurrentCell(i,j); 
      item=table->currentItem();
      if(item!=0)
        text.append("<td><b> "+item->text()+"</b></td>");
         else
          {
           text.append("<td> </td>");
          };
     }
    text.append("</tr><br>");
   }

  table->setCurrentCell(x,y);
  text.append("</font></table></center><br><br></td></tr></table>");  


  hidden ->setHtml(text);
  
  QTextDocument *document = hidden->document();
  hidden->hide();
  document->print(printer);
  delete printer;
  delete hidden;
};

//Prints the booklet
void qOrganizer::printBooklet()
{
  QTextEdit *hidden=new QTextEdit;
  hidden->setParent(this);
  hidden->show();
  QString text;
 
  //Marks
  int x=markTable->currentRow();
  int y=markTable->currentColumn();  

  text="<center><table border=\"0\"><tr><td>";
  text.append("<font size=\"5\"><center>"+tr("Marks:")+"</center></font><br>");

  
  text.append("<table border=\"1\">");
  text.append("<font size=\"7\">");


  for(int i=0;i<markTable->rowCount();i++)
   { 
    text.append("<tr><td>");
    text.append(subjectList.at(i));
    text.append("</td>");

    for(int j=0;j<markTable->columnCount();j++)
     {
      markTable->setCurrentCell(i,j); 
      item=markTable->currentItem();
      if(item!=0)
        text.append("<td><b> "+item->text()+"</b></td>");
         else
          {
           text.append("<td> </td>");
          };
     }
    text.append("</tr><br>");
   }

  markTable->setCurrentCell(x,y);
  text.append("</font></table><br></td>");  
 
 //Absences

  x=absenceTable->currentRow();
  y=absenceTable->currentColumn();  
 
  text.append("<td><font size=\"5\"><center>"+tr("Absences:")+"</center></font><br>");

  text.append("<table border=\"1\">");
  text.append("<font size=\"7\">");

  for(int i=0;i<absenceTable->rowCount();i++)
   { 
    text.append("<tr><td>");
    text.append(subjectList.at(i));
    text.append("</td>");

    for(int j=0;j<absenceTable->columnCount();j++)
     {
      absenceTable->setCurrentCell(i,j); 
      item=absenceTable->currentItem();
      if(item!=0)
        text.append("<td><b> "+item->text()+"</b></td>");
         else
          {
           text.append("<td> </td>");
          };
     }
    text.append("</tr><br>");
   }

  absenceTable->setCurrentCell(x,y);
  text.append("</font></table><br></td></tr></table></center>");  

  hidden ->setHtml(text);
  
  QTextDocument *document = hidden->document();
  hidden->hide();
  document->print(printer);
  delete printer;
  delete hidden;
};


//When the print button is hit on the toolbar the app checkes what page is active and prints the
//currently active page
void qOrganizer::printPage()
{
 printer = new QPrinter();
 QPrintDialog printDialog(printer, this);
 if (printDialog.exec() == QDialog::Accepted)
 { 
  if(mainWid->currentIndex()==0) 
   {
    printCalendar();
   }
   else
    if(mainWid->currentIndex()==1)
     { 
      printToDoList();
     } 
     else 
      if(mainWid->currentIndex()==2)
       {
        printTimeTable();
       }
       else
        if(mainWid->currentIndex()==3)
         {
          printBooklet();
         };
 };

};


//-----------------------------------CONFIG FILE ------------------------------------------------

//sets the home directory where the config file is located
void qOrganizer::setConfigDir()
{
 QDir::setCurrent(QDir::homePath ());
 QDir currentDir = QDir::current();
 currentDir.mkdir(".qOrganizer");
 currentDir.cd(".qOrganizer");
 QDir::setCurrent(currentDir.path());
};

//This is called if no config file is available, contains default settings
void qOrganizer::writeDefaultConfigFile() //this isn't used anymore!
{
 setConfigDir();
 QFile configFile("config.txt"); 
 cout <<"Writing default config file\n";

  if (configFile.open(QFile::WriteOnly)) 
  {
     QTextStream out(&configFile); 
     out.setCodec("UTF-8");
     out << "home" <<"\n"; //The default directory where the config file is located
     out << "30000" <<"\n"; //The value for the timer to check for reminders
     out << "1" <<"\n";  //First day of the week
     out << "ON" <<"\n"; //Autosave toggle
     out << "Baloon" <<"\n"; //Type of reminder
     out << "4" <<"\n";
     out << "dddd MMMM d. yyyy" << "\n";
     out << "OFF" <<"\n";
     out << "OFF" <<"\n";
  } 
   else
    { 
      QMessageBox::warning(NULL, "qOrganizer", "Unable to write config file.", "OK");
    }; 
 readConfigFile();
};

//Reads the config file and sets the config variables which are noted all upercase starting
//with C_ (from CONFIG)
void qOrganizer::readConfigFile()
{
 /*setConfigDir();
 QFile configFile("config.txt");
 
 if (configFile.open(QFile::ReadOnly))
 {
   QTextStream in(&configFile);
   in.setCodec("UTF-8");
   C_PATH = in.readLine();
   //QTextStream cout(stdout, QIODevice::WriteOnly);
   C_TIMEOUT=(in.readLine()).toInt();
   C_FIRST_DAY_OF_WEEK = (in.readLine()).toInt();
   QString autosaveString = in.readLine();
   if(autosaveString=="ON") 
    C_AUTOSAVE_TOGGLE=true; 
    else C_AUTOSAVE_TOGGLE = false; 
   QString messageString = in.readLine();
   if(messageString=="Baloon")
    C_BALOON_SET = true;
     else C_BALOON_SET = false;
   C_NRROWS = (in.readLine()).toInt();
   C_DT_FORMAT = in.readLine();
   QString ftpString = in.readLine();
   if(ftpString=="ON")
    C_SYNCFTP=1;
     else C_SYNCFTP=0;
   QString loadString = in.readLine(); 
   if(loadString=="ON")
    C_LOAD_ON_CHANGE=1;
     else C_LOAD_ON_CHANGE=0;
 } 
 else writeDefaultConfigFile();*/
 
 //C_PATH =settings -> value("pathtowdir","home").toString();
 
 C_TIMEOUT=settings -> value("rcheckinterval",20000).toInt();
 
 C_FIRST_DAY_OF_WEEK=settings -> value("firstdayindex",1).toInt();
  
 C_AUTOSAVE_TOGGLE=settings -> value("autosave",1).toBool();

 if(settings -> value("remindertype","Baloon").toString()=="Baloon")
  C_BALOON_SET = 1; else C_BALOON_SET=0;
 
 C_NRROWS = settings -> value("rownr",4).toInt();

 C_DT_FORMAT = settings -> value("dateformat","dddd MMMM d. yyyy").toString();

 C_SYNCFTP = settings -> value("ftpsynch",0).toBool(); 

 C_LOAD_ON_CHANGE = settings -> value("reloaddata",0).toBool(); 
} 

//Shows the setting dialog
void qOrganizer::showSettings()
{
 settingsDialog = new SettingsDialog(this);
 settingsDialog -> show();
};


//Saves the config file this is connected to the OK button of the settings dialog
void qOrganizer::saveConfigFile()
{
 /*setConfigDir();
 QFile configFile("config.txt");
 if (configFile.open(QFile::WriteOnly))
  { 
    QTextStream out(&configFile);
    out.setCodec("UTF-8");
    if(settingsDialog->pathEdit->text() == QDir::homePath())
     out << "home";
     else 
      if(!settingsDialog->pathEdit->text().endsWith(QDir::separator()))
       out << settingsDialog->pathEdit->text()+QDir::separator();
        else
         out << settingsDialog->pathEdit->text();
    out <<"\n";
    out << settingsDialog -> rBox -> value()<<"\n";
    out << settingsDialog -> firstDay -> currentIndex()+1<<"\n";
    if(settingsDialog->autoBox->isChecked())
     out <<"ON";
      else out << "OFF";
    out <<"\n";
    out << settingsDialog->messageCombo->currentText()<<"\n";
    out << settingsDialog->rownrBox->value()<<"\n";
    out << settingsDialog->dateFormatEdit->currentText()<<"\n";

    if(settingsDialog->ftpBox->isChecked())
     out <<"ON"<<"\n";
      else out <<"OFF"<<"\n";

    if(settingsDialog->loadBox->isChecked())
     out <<"ON"<<"\n";
      else out <<"OFF"<<"\n";
  }
  else 
   {
    QMessageBox::warning(NULL, "qOrganizer", "Unable to write config file.", "OK");
   };*/
 
 /*if(settingsDialog->pathEdit->text() == QDir::homePath())
   settings -> setValue("pathtowdir", "home");
    else 
     if(!settingsDialog->pathEdit->text().endsWith(QDir::separator()))
      settings -> setValue("pathtowdir",settingsDialog->pathEdit->text()+QDir::separator());
      else settings -> setValue("pathtowdir",settingsDialog->pathEdit->text());*/
        
  //Store it in the registry
  QSettings rsettings("qOrganizer","qOrganizer");
  if(settingsDialog->pathEdit->text() == QDir::homePath())
    rsettings.setValue("pathtowdir", "home");
     else 
      if(!settingsDialog->pathEdit->text().endsWith(QDir::separator()))
       rsettings.setValue("pathtowdir",settingsDialog->pathEdit->text()+QDir::separator());
       else rsettings.setValue("pathtowdir",settingsDialog->pathEdit->text());
 
  settings -> setValue("rcheckinterval",settingsDialog -> rBox -> value());
  settings -> setValue("firstdayindex",settingsDialog -> firstDay -> currentIndex()+1);
  settings -> setValue("autosave",settingsDialog->autoBox->isChecked()); 
  settings -> setValue("remindertype",settingsDialog->messageCombo->currentText());
  settings -> setValue("rownr",settingsDialog->rownrBox->value());
  settings -> setValue("dateformat",settingsDialog->dateFormatEdit->currentText());
  settings -> setValue("ftpsynch",settingsDialog->ftpBox->isChecked());
  settings -> setValue("reloaddata",settingsDialog->loadBox->isChecked());
  
  settings -> setValue("showsavebuttons",settingsDialog->saveAllBox->isChecked());
 //Store the selected language in the registry 
 //QSettings settings("qOrganizer", "qOrganizer");

 settings -> setValue("language", settingsDialog->langCombo->currentText());
 settings -> setValue("host",settingsDialog->hostEdit->text());
 settings -> setValue("port",settingsDialog->portBox->value());
 settings -> setValue("user",settingsDialog->userEdit->text());
 settings -> setValue("passwd",settingsDialog->passEdit->text());
 settings -> setValue("path",settingsDialog->ftppathEdit->text());
 if(settingsDialog->trayBox->isChecked())
  settings -> setValue("tosystrayonclose",1);
   else settings -> setValue("tosystrayonclose",0);
 
 if(settingsDialog->soundBox->isChecked())
  settings -> setValue("soundonremind",1);
   else settings -> setValue("soundonremind",0);
 
 settings -> setValue("storingmode", settingsDialog->storingBox->currentIndex());
 

 //MySQL stuff 
 settings -> setValue("mysqlhost",settingsDialog->mysqlHostEdit->text());
 settings -> setValue("mysqlport",settingsDialog->mysqlPortBox->value());
 settings -> setValue("mysqluser",settingsDialog->mysqlUserEdit->text());
 settings -> setValue("mysqlpasswd",settingsDialog->mysqlPassEdit->text());
 settings -> setValue("mysqldb",settingsDialog->mysqldbEdit->text());
 
 //Odd timetable
 if(settingsDialog->oddTTBox->isChecked())
  settings -> setValue("useoddtimetable",1);
   else settings -> setValue("useoddtimetable",0);
 
 if(settingsDialog->reverseTTBox->isChecked())
  settings -> setValue("reverseweekorder",1);
   else settings -> setValue("reverseweekorder",0);
 
 if(settingsDialog->roundBox->isChecked())
  settings -> setValue("roundonaverage",1);
   else settings -> setValue("roundonaverage",0); 

 QTextStream cout(stdout, QIODevice::WriteOnly);
 if(C_PATH!=settingsDialog->pathEdit->text())
  { 
   cout << "DirPath changed \n";
   if(C_AUTOSAVE_TOGGLE) toggleAutoSave(0);
   getWDirPath();
   cout << C_PATH <<"\n";
   if(db.isOpen()) db.close();
   connectToDB();
   loadAll();
   if(changedAutoSave) toggleAutoSave(1);
  }

 settingsDialog->close();
 readConfigFile(); //We re-read it only for the C_TIMEOUT variable to take effect instantly
 readSettings(); //For the ftp to take effect instantly
 
 if(C_OLD_STORINGMODE!=C_STORINGMODE)
  { 
   if(db.isOpen()) db.close();
   cout << C_PATH <<"\n";
   connectToDB();
   loadAll();
  }
 
 //If the user changed the storing mode then reload data
 if(C_USE_ODDTT)
 {
  if(!ttLabel->isVisible()) ttLabel->show();
  if(!ttCombo->isVisible()) ttCombo->show();
 }
  else 
   {
    ttLabel->hide();
    ttCombo->hide();
   }
 
 if(!C_AUTOSAVE_TOGGLE) saveAct -> setVisible(true);
  else saveAct->setVisible(false);
 
 if(C_SYNCFTP) 
  {
   uploadAct->setVisible(true); 
   downloadAct->setVisible(true);
  }
  else
   {
    uploadAct->setVisible(false); 
    downloadAct->setVisible(false);
   }

 if(!C_SHOW_SAVE)
  {
   saveToTXTAct->setVisible(false);
   saveToDBAct->setVisible(false);
   saveToMySQLDBAct->setVisible(false);
  }
  else
   if(C_STORINGMODE==0)
       {
        saveToTXTAct->setVisible(false);
        saveToDBAct->setVisible(true);
        saveToMySQLDBAct->setVisible(true);
       }
       else 
        if(C_STORINGMODE==1)
         {
          saveToDBAct->setVisible(false);
          saveToTXTAct->setVisible(true);
          saveToMySQLDBAct->setVisible(true);        
         }
         else 
          if(C_STORINGMODE==2) 
           {
            saveToMySQLDBAct->setVisible(false);
            saveToTXTAct->setVisible(true);
            saveToDBAct->setVisible(true);
           }
}


//------------------------------------FTP SYNC------------------------------------------------

/*Ok so this uploads a whole folder to a remote ftp server
The path is the folders name and the dir path is the folder where you keep it
Example: if the folder is /home/user/.qOrganizer then that is the path
and the dirPath is /home/user
This function is called from putToFTP() 

So QFtp is asynchronous, that's a good thing for the user, and bad for us, but we will deal with it.This is pretty readable and simple code:
*/
void qOrganizer::sendFolder(const QString &path,const QString &dirPath)
{
    QTextStream cout(stdout, QIODevice::WriteOnly);
    QFileInfo fileInfo(path);
    if (fileInfo.isDir())
    {
        QString dirName = path;
        dirName.remove(0,dirPath.size());
        if(!noFolders)
         {
          ftp->mkdir(dirName);
          cout <<"Creating directory named:"<<dirName<<"\n";
         }
        QDir dir(path);
        QStringList entries = dir.entryList(QDir::AllEntries|QDir::NoDotAndDotDot,QDir::DirsFirst);
        foreach (QString entry, entries)
        {
         statusBar()->showMessage(tr("Uploading..."),10000);
         sendFolder(dir.filePath(entry),dirPath);
        }
    }
    else
    {   
        QFile *file = new QFile(path);
        uploadedFileQueue.enqueue(file);
        if(file->open(QIODevice::ReadOnly))
         {
          QString filename =path;
          filename.remove(0,dirPath.size());
          putCommandVector.append(ftp->put(file,filename));
          cout << "put "<<path<<" "<<filename<<"\n";
          statusBar()->showMessage(tr("Uploading..."),10000);
          file->close();
         }
         else cout <<"Could not open file:"<<path<<"\n";
    }
}


/*
 This is where the fun begins and things get really asynchronous, if you thought the last one was hard to understand then prepare to read some docs.
 We managed to upload a folder. Now we need to download it.
 path is the folders name, and localPath the folder that contains our folder.
 ex: if the folder is /home/user/ftp/.qOrganizer then the path variable is .qOrganizer and the localPath is /home/user/ftp/
The list command emits a signal called listInfo which collects the entryes/
*/
void qOrganizer::getFolder(QString path,QString localPath)
{
 QTextStream cout(stdout, QIODevice::WriteOnly);
 QDir localDir(localPath);
 listCommandVector.append(ftp->list(path)); //we track the commands in vectors
 stack.push(path); 
 localDir.mkdir(path);
 connect(ftp,SIGNAL(listInfo(QUrlInfo)),this,SLOT(processFolder(QUrlInfo)));
 connect(ftp,SIGNAL(commandFinished(int,bool)),this,SLOT(processCommand(int,bool)));
 ftpEntryes = new QStringList; //this will contain the file list
 lpath=localPath;
}

//This makes us a QList with all entryes, folders first then files.

void qOrganizer::processFolder(const QUrlInfo &i)
{
 if(i.isDir())
  {
   entryList.push_front(i); 
  }
   else 
     if(i.isFile()) 
      entryList.push_back(i);     
}


static int nrcomfin=0; 
//nr of finished command, this counts how many times this function is called
void qOrganizer::processCommand(int com,bool err)
{
 Q_UNUSED(err);
 //Variables
 nrcomfin++;
 QString path;
 QTextStream cout(stdout, QIODevice::WriteOnly);
 bool containsSubfolders=0;
 bool entryExists=0;
 QDir localDir(lpath);
 //If the command was list() then we start procesing our QList
 if(listCommandVector.indexOf(com)!=-1)
  { 
   path = stack.top();
   foreach(QUrlInfo entry,entryList)
    {
      if((entry.isFile())&&(listedFiles.indexOf(entry)==-1))
       {
        QString foldPath = path+"/"+entry.name();
        ftpEntryes->append(foldPath);
        listedFiles.append(entry);
        //cout <<"File:"<<foldPath<<"\n";
        entryExists=1;
       }
       else
        if((entry.isDir())&&(listedFolders.indexOf(entry)==-1))
         {
          QString foldPath = path+"/"+entry.name();
          //cout <<"Folder:"<<foldPath<<"\n";
          stack.push(foldPath); 
          localDir.mkdir(foldPath);
          //cout << stack.top() <<"\n";
          listCommandVector.append(ftp->list(foldPath));
          listedFolders.append(entry);
          containsSubfolders=1;
          entryExists=1;
          break;
         }
    }
   if((stack.size()>1)&&(nrcomfin%2!=0)) stack.pop();
   entryList.clear();
   //cout << stack.size() << stack.top() <<" "<<nrcomfin<<"\n"; 
   if((!containsSubfolders)&&(entryExists)) 
      listCommandVector.append(ftp->list(stack.top()));
   if((!containsSubfolders) && (!entryExists)) downloadFiles();   
  }
  else //If the command was a get
   if(getCommandVector.indexOf(com)!=-1)
    {
     ftpFile=fileQueue.dequeue();
     ftpFile-> close();
     //cout <<"Completed:"<<ftpFile->fileName()<<" "<<"Errors:"<<err<<"\n";
     ftpFile->deleteLater();
    }
    else
     if(putCommandVector.indexOf(com)!=-1)
      {
       QFile *myFile = uploadedFileQueue.dequeue();
       myFile->deleteLater();
      };
}

void qOrganizer::downloadFiles()
{
 QTextStream cout(stdout, QIODevice::WriteOnly);
 //cout <<"Downloading"<<"\n";
 foreach(QString entry,*ftpEntryes)
 {
  QFile *file = new QFile(lpath+entry);
  cout << "get " << entry <<" "<<lpath+entry<<"\n";
  statusBar()->showMessage(tr("Downloading..."),10000);
  fileQueue.enqueue(file);
  file->open(QIODevice::WriteOnly);
  getCommandVector.append(ftp -> get(entry,file));
 }
}

void qOrganizer::connectToFTP()
{
 ftp = new QFtp(this);
 ftp -> connectToHost(C_HOST,C_PORT);
 ftp -> login(C_USER,C_PASSWD);
 ftp -> cd(C_FTPPATH);
}

//Two simplyfied aliases

void qOrganizer::initUpload()
{
 noFolders=false;
 putToFTP();
}

void qOrganizer::putToFTP()
{ 
 statusProgress -> show();
 connectToFTP();
 uploadAct->setEnabled(false);
 connect(ftp,SIGNAL(done(bool)),this,SLOT(updateStatusP(bool)));
 if(C_PATH=="home")  
  sendFolder(QDir::toNativeSeparators(QDir::homePath()+"/")+".qOrganizer",QDir::toNativeSeparators(QDir::homePath()+"/"));
   else 
     sendFolder(C_PATH+".qOrganizer",C_PATH);
}

void qOrganizer::getFromFTP()
{
 downloadingFromFTP=1;
 statusProgress -> show();
 connectToFTP();
 downloadAct->setEnabled(false);
 if(C_AUTOSAVE_TOGGLE) toggleAutoSave(0);
 connect(ftp,SIGNAL(done(bool)),this,SLOT(updateStatusG(bool)));
 if(C_PATH=="home")  
  getFolder(QString(".qOrganizer"),QDir::toNativeSeparators(QDir::homePath()+"/"));
   else 
    getFolder(QString(".qOrganizer"),C_PATH);
}

void qOrganizer::updateStatusG(bool error)
{
 downloadingFromFTP=0;
 downloadAct->setEnabled(true);
 statusProgress -> hide();
 if(!this->isVisible()) toggleVisibility();
 if(!error) 
  {
   statusBar()->showMessage(tr("Done!"),10000);
   QMessageBox::warning(NULL,tr("Done"),tr("Download finished!"));
   loadAll(); 
   ftp->deleteLater();
  }
   else
    {
      QMessageBox::warning(NULL, "Error",tr("Couldn't download files:\n")+ftp->errorString());
      ftp->deleteLater();
    }
  disconnect(ftp, SIGNAL(done(bool)),this,SLOT(updateStatusG(bool)));
  if(changedAutoSave) toggleAutoSave(1); 
  stack.clear();
  delete ftpEntryes;
  entryList.clear();
  listedFiles.clear();
  listedFolders.clear();
  listCommandVector.clear();
  getCommandVector.clear();
  nrcomfin=0;
}

void qOrganizer::updateStatusP(bool error)
{
 QTextStream cout(stdout, QIODevice::WriteOnly);
 statusProgress -> hide();
 cout << " Upload finished:"<<error<<" "<<ftp->errorString()<<"\n";
 uploadAct->setEnabled(true);
 if(!this->isVisible()) toggleVisibility();
 if(!error) 
  {
   statusBar()->showMessage(tr("Done!"),10000);
   QMessageBox::warning(NULL,tr("Done"),tr("Upload finished!"));
   ftp->deleteLater();
   noFolders=false;
  }
   else
    {
     if((ftp->errorString().indexOf(".qOrganizer")!=-1)&&(ftp->errorString().indexOf("exists")!=-1))
      {
       noFolders=true;
       ftp->deleteLater();
       disconnect(ftp, SIGNAL(done(bool)),this,SLOT(updateStatusP(bool)));
       cout <<"Error encountered:Folder exists, entering noFolder mode and reuploading\n";
       //Don't know why we need to call it twice, but it doesn't work if it is called only once.
       //Maybe one gets killed by deleteLater
       putToFTP();
       putToFTP();
      }
       else
        {
          QMessageBox::warning(NULL,tr("Error"),QString(tr("Couldn't upload files:\n")+ftp->errorString()));
          ftp->deleteLater();
        }
    }
 disconnect(ftp, SIGNAL(done(bool)),this,SLOT(updateStatusP(bool)));
}

//if ind is true turn it on else turn it off
void qOrganizer::toggleAutoSave(bool ind)
{
 if(!ind)
  {
   disconnect(tableWid,SIGNAL(focusLost()),this,SLOT(saveSchedule()));
   disconnect(textField,SIGNAL(focusLost()),this,SLOT(saveJurnal()));
   disconnect(list,SIGNAL(focusLost()),this,SLOT(saveToDoList()));
   disconnect(table,SIGNAL(focusLost()),this,SLOT(saveTimeTable()));
   disconnect(markTable,SIGNAL(focusLost()),this,SLOT(saveMarksTable()));
   disconnect(absenceTable,SIGNAL(focusLost()),this,SLOT(saveAbsenceTable()));
   changedAutoSave=1;
  }
  else
   {
    connect(tableWid,SIGNAL(focusLost()),this,SLOT(saveSchedule()));
    connect(textField,SIGNAL(focusLost()),this,SLOT(saveJurnal()));
    connect(list,SIGNAL(focusLost()),this,SLOT(saveToDoList()));
    connect(table,SIGNAL(focusLost()),this,SLOT(saveTimeTable()));
    connect(markTable,SIGNAL(focusLost()),this,SLOT(saveMarksTable()));
    connect(absenceTable,SIGNAL(focusLost()),this,SLOT(saveAbsenceTable()));
    changedAutoSave=0;
   }
};




//Needed by custom delegate to be able to delete from the absenceTable, all because there is no 
//way to create an empty QDateTimeEdit
//We previously called setObjectName for these tables
void CQTableWidget::keyPressEvent (QKeyEvent * event)
{
 if(this->objectName()==QString("absenceTable")) //The table with the absences
 {
   if(event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete)
   {
       this->currentItem()->setText(" ");
       return;
   }
 }
 else
  if(this->objectName()==QString("list")) //To-do list table
   { 
     if((this->currentColumn()==2)||(this->currentColumn()==0))
      {
       if(event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete)
       {
        this->currentItem()->setText(" ");
        return;
       }
      }
   }
   else
    if(this->objectName()==QString("tableWid")) //Schedule table
     {
      if(this->currentColumn()!=0)
      {
       if(event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete)
       {
        this->currentItem()->setText(" ");
        return;
       }
      } 
     }
 return QTableWidget::keyPressEvent(event);
}

void qOrganizer::saveAll()
{
 if(C_STORINGMODE==0)
  {
   saveToDoList();
   saveTimeTable();
   saveSchedule();
   saveJurnal();
   saveMarksTable();
   saveAbsenceTable();
   saveMarkDates();
  }
  else 
   {
    saveScheduleDB();
    saveJurnalDB();
    saveToDoListDB();
    saveTimeTableDB();
    saveMarksTableDB();
    saveAbsenceTableDB();
    saveMarkDatesDB();
   }
}

void qOrganizer::loadAll()
{
 //if(C_AUTOSAVE_TOGGLE) toggleAutoSave(0);
 if(C_STORINGMODE==0)
  {
   loadTimeTable();
   loadSchedule();
   loadJurnal();
   loadToDoList();
   loadMarksTable();
   loadAbsenceTable();
   loadMarkDates();
  }
  else 
   {
    loadScheduleDB();
    loadJurnalDB();
    loadTimeTableDB();
    loadToDoListDB();
    loadMarksTableDB();
    loadAbsenceTableDB();
    loadMarkDatesDB();
   }
 //if(changedAutoSave) toggleAutoSave(1);
}

void qOrganizer::exitApp()
{
 if(C_AUTOSAVE_TOGGLE)
 {
  saveAll();
 }
 writeSettings();
 saveColumnWidths();
 if(db.open()) db.close();
 qApp->quit();
}

void qOrganizer::playSound()
{
 if(QSound::isAvailable())
  {
   QFile soundFile(":images/sound.wav");
   if(C_PATH=="home")
    {
     soundFile.copy(QDir::homePath()+QDir::separator()+".qOrganizer"+QDir::separator()+"sound.wav");
     QSound::play(QDir::homePath()+QDir::separator()+".qOrganizer"+QDir::separator()+"sound.wav");
    }
     else   
      {
       soundFile.copy(C_PATH+".qOrganizer"+QDir::separator()+"sound.wav");
       QSound::play(C_PATH+".qOrganizer"+QDir::separator()+"sound.wav");
      }
  }
  else 
   {//Linux uses NAS, which kind of sucks with qt, so we have to do this primitively
    #ifdef Q_OS_LINUX
     QFile soundFile(":images/sound.wav");
     if(!soundFile.copy("/tmp/qremindersound.wav")) 
      cout << "Could not copy soundfile to /tmp\n";
     system("cat /tmp/qremindersound.wav >> /dev/dsp"); 
    #endif
   }
} 

//------------------------------------------------------DATABASE SLOTS--------------------------------------------------------------


void qOrganizer::setDBDir()
{
 if(C_PATH=="home")
  dbPath=QDir::homePath()+QDir::separator()+".qOrganizer";
   else
    dbPath=C_PATH+QDir::separator()+".qOrganizer"+QDir::separator();
 QDir::setCurrent(dbPath);
}

void qOrganizer::connectToDB()
{
 setDBDir();
 QTextStream cout(stdout, QIODevice::WriteOnly);
 if(C_STORINGMODE==1)
  {
   db = QSqlDatabase::addDatabase("QSQLITE");
   db.setDatabaseName("qorganizer.db");
  } 
   else
    if(C_STORINGMODE==2)
     { 
      db = QSqlDatabase::addDatabase("QMYSQL");
      db.setHostName(C_MYSQL_HOST);
      db.setPort(C_MYSQL_PORT);
      db.setDatabaseName(C_MYSQL_DB);
      db.setUserName(C_MYSQL_USER);
      db.setPassword(C_MYSQL_PASSWD);
     }
 
 if ((!db.open())&&(C_STORINGMODE!=0)) 
  {
   QMessageBox::critical(0,tr("Error"),
                        tr("Could not connect to database!")
                        ,QMessageBox::Ok); 
    C_STORINGMODE=0; //Revert to textfiles 
   }
   else cout << "Connected to database\n";
}

bool qOrganizer::tableExistsDB(QString &tablename)
{
  bool result; 
  QStringList tableList = db.tables(QSql::Tables);
  if(tableList.indexOf(tablename)!=-1) result=true; else result=false;
  return result;
}

//GOLDEN RULE: ALWAYS PREPARE YOUR SQL STATEMENTS BEFORE INSERT! 

void qOrganizer::loadScheduleDB()
{
 bool didntExist=0;
 if(!db.isOpen()) connectToDB();
 emptyTable();
 QTextStream cout(stdout, QIODevice::WriteOnly);
 QDate date = calendar->selectedDate();
 QString tableName ="schedule_"+date.toString("MM_dd_yyyy");
 cout<<"Loading schedule from database:"<<tableName<<"\n";
 QSqlQuery query(db); 
 if(!tableExistsDB(tableName)) didntExist=1;
 //Create table if it does not exist.
 query.exec("CREATE TABLE `"+tableName+"` (`_event_` TEXT,`_from_` TEXT,`_until_` TEXT,`_reminder_` TEXT)");
 //Select all from the table
 query.exec("SELECT _event_,_from_,_until_,_reminder_ FROM `"+tableName+"`");
 //Row counter, it will get incremented
 int i=-1;
 while(query.next())
  {
   i++;
   if(i>=tableWid->rowCount()) insertRowToEnd();;
   for(int j=0;j<tableWid->columnCount();j++)
    {
     QString text = query.value(j).toString();
     item= new QTableWidgetItem(QTableWidgetItem::Type);
     item->setText(text);
     if(tableWid->item(i,j)!=0) delete tableWid->takeItem(i,j);
     tableWid->setItem(i,j,item); 
    }
  }
  tableWid->setRowCount(i+1);
 //If the table didn't exist then set the preferred nr of rows
 if(didntExist) 
  {
   emptyTable();
   saveScheduleDB();
  }

} 

void qOrganizer::saveScheduleDB()
{
   QTextStream cout(stdout, QIODevice::WriteOnly);
   if(!db.isOpen()) connectToDB();
   //Get selected date
   QDate date = calendar->selectedDate();
   QString tableName ="schedule_"+date.toString("MM_dd_yyyy");
   cout << "Saving schedule to database:"<<tableName<<"\n";
   //Variables
   QSqlQuery query(db); 

   //Create database if it doesn't exist
   query.exec("CREATE TABLE `"+tableName+"` (`_event_` TEXT,`_from_` TEXT,`_until_` TEXT,`_reminder_` TEXT)");
   
   query.exec("DELETE FROM `"+tableName+"`");
   
   if(query.lastError().type()!=QSqlError::NoError) 
   {
    cout << " A database error occured when saving schedule:\n";
    cout << " "<<query.lastError().text()<<"\n";
    cout << " This could be caused by running several instances of qOrganizer\n";
    cout << " Saving aborted\n";
   }
   else
    {
     int x=tableWid->currentRow();
     int y=tableWid->currentColumn();
  
     for(int i=0;i<tableWid->rowCount();i++)
      { 
       textList.clear();
       for(int j=0;j<tableWid->columnCount();j++)
        {
         tableWid->setCurrentCell(i,j); 
         QTableWidgetItem *item=tableWid->currentItem();
         if(item!=0)
          textList.append(item->text());
           else textList.append("");
        }
       QString command = "INSERT INTO `"+tableName+"` VALUES"+
                          "(:myevent,:myfrom,:myuntil,:myreminder)";
       query.prepare(command);
       query.bindValue(":myevent",textList[0]);
       query.bindValue(":myfrom",textList[1]);
       query.bindValue(":myuntil",textList[2]);
       query.bindValue(":myreminder",textList[3]);
       query.exec();
      }
     tableWid->setCurrentCell(x,y); 
    }
}

void qOrganizer::loadJurnalDB() //I know it's Journal not Jurnal, but wth
{
 if(!db.isOpen()) connectToDB();
 QTextStream cout(stdout, QIODevice::WriteOnly);
 QDate date = calendar->selectedDate();
 QString tableName ="jurnal_"+date.toString("MM_dd_yyyy");
 cout << "Loading jurnal from database:"<<tableName<<"\n";
 QSqlQuery query(db); 
 //Create table if it does not exist.
 query.exec("CREATE TABLE `"+tableName+"` (`_text_` TEXT)");
 //Select all from the table
 query.exec("SELECT _text_ FROM `"+tableName+"`");
 QString text;
 while(query.next())
  {
   text = query.value(0).toString();
  }
  textField->setHtml(text);
}

void qOrganizer::saveJurnalDB()
{
 QString text;
 if(!db.isOpen()) connectToDB();
 QTextStream cout(stdout, QIODevice::WriteOnly);
 QDate date = calendar->selectedDate();
 QString tableName ="jurnal_"+date.toString("MM_dd_yyyy");
 cout << "Saving jurnal to database:"<<tableName<<"\n";
 QSqlQuery query(db); 
 //Create table if it does not exist.
 query.exec("CREATE TABLE `"+tableName+"` (`_text_` TEXT)");
 //Delete the table contents
 query.exec("DELETE FROM `"+tableName+"`");
 text = textField->toHtml();
 query.prepare("INSERT INTO `"+tableName+"` VALUES"+" (:mytext)");
 query.bindValue(":mytext",text);
 query.exec();
}

void qOrganizer::loadToDoListDB()
{
 list->setSortingEnabled(false);
 if(!db.isOpen()) connectToDB();
 QTextStream cout(stdout, QIODevice::WriteOnly);
 QString tableName ="todolist";
 cout << "Loading to-do list from database\n";
 list->clearContents();
 QSqlQuery query(db); 
 bool didntexist=0;
 if(!tableExistsDB(tableName)) didntexist=1;
 //Create table if it does not exist.
 query.exec("CREATE TABLE `"+tableName+"` (`_start_` TEXT,`_task_` TEXT,`_deadline_` TEXT,`_priority_` TEXT,`_completed_` TEXT)");
 //Select all from the table
 query.exec("SELECT _start_,_task_,_deadline_,_priority_,_completed_ FROM `"+tableName+"`");
 //Row counter, it will get incremented
 int i=-1;
 while(query.next())
  {
   i++;
   if(i>=list->rowCount()) newTask();
   for(int j=0;j<list->columnCount();j++)
    {
     QString text = query.value(j).toString();
     item= new QTableWidgetItem(QTableWidgetItem::Type);
     item->setText(text);
     if(list->item(i,j)!=0) delete list->takeItem(i,j);
     list->setItem(i,j,item); 
    }
  }
 if(didntexist) 
  {
   list->setRowCount(10); 
  }else list->setRowCount(i+1);
 list->setSortingEnabled(true);
}

void qOrganizer::saveToDoListDB()
{
 QTextStream cout(stdout, QIODevice::WriteOnly);
 if(!db.isOpen()) connectToDB();
 QString tableName ="todolist";
 cout << "Saving to-do list to database"<<"\n";
 //Variables
 QSqlQuery query(db); 
 //Create table if it does not exist.
 query.exec("CREATE TABLE `"+tableName+"` (`_start_` TEXT,`_task_` TEXT,`_deadline_` TEXT,`_priority_` TEXT,`_completed_` TEXT)");
 query.exec("DELETE FROM `"+tableName+"`");
 int x=list->currentRow();
 int y=list->currentColumn();
  
 for(int i=0;i<list->rowCount();i++)
 { 
  textList.clear();
  for(int j=0;j<list->columnCount();j++)
   {
    list->setCurrentCell(i,j); 
    QTableWidgetItem *item=list->currentItem();
    if(item!=0)
     textList.append(item->text());
      else textList.append("");
   }
  QString command = "INSERT INTO `"+tableName+"` VALUES"+
                    "(:mystart,:mytask,:mydeadline,:mypriority,:mycompleted)";
  query.prepare(command);
  query.bindValue(":mystart",textList[0]);
  query.bindValue(":mytask",textList[1]);
  query.bindValue(":mymydeadline",textList[2]);
  query.bindValue(":mypriority",textList[3]);
  query.bindValue(":mycompleted",textList[4]);
  query.exec();
 }
 list->setCurrentCell(x,y);  
}

void qOrganizer::setTTLabelsDB()
{
 QTextStream cout(stdout, QIODevice::WriteOnly);
 if(!db.isOpen()) connectToDB();
 bool didntExists=0;
 QString tableName="timetable_labels";
 QSqlQuery query(db); 
 QStringList timeTableLabels;
 timeTableLabels.clear();
 //create if it doesn't exist 
 if(!tableExistsDB(tableName)) didntExists=1;
 
 query.exec("CREATE TABLE `"+tableName+"` (`_label_` TEXT)");
 if(didntExists) 
  { 
   QStringList localLabelList;
   localLabelList <<"From"<<"Until"<<"Monday"<<"Tuesday"<<"Wednesday"<<"Thursday"<<"Friday";
   foreach(QString text,localLabelList)
    query.exec("INSERT INTO "+tableName+" VALUES"+"('"+text+"')");
  }
 
 query.exec("SELECT _label_ FROM `"+tableName+"`");
 while(query.next())
   timeTableLabels << query.value(0).toString();

 englishList = timeTableLabels; //the original
 //translate them if we can.
 for(int i=0;i<timeTableLabels.size();i++)
  {
    QString temp = tr(timeTableLabels.at(i).toUtf8().data());
    timeTableLabels.replace(i,temp);
  }
 table->setColumnCount(timeTableLabels.size()); 
 table->setHorizontalHeaderLabels(timeTableLabels);
 TTlabels = timeTableLabels;
}

void qOrganizer::loadTimeTableDB()
{
 QTextStream cout(stdout, QIODevice::WriteOnly);
 cout << "Loading timetable from database \n";
 table->clearContents();
 setTTLabelsDB();
 if(!db.isOpen()) connectToDB();
 QString tableName="timetable";
 if(C_TIMETABLE_INDEX==0) tableName="timetable"; else tableName="timetable_even";
 QSqlQuery query(db); 
 bool didntexist=0;
 if(!tableExistsDB(tableName)) didntexist=1;
 //Create table if it does not exist.
 query.exec("CREATE TABLE `"+tableName+
            "` (`_0_` TEXT,`_1_` TEXT,`_2_` TEXT,`_3_` TEXT"+
            ",`_4_` TEXT,`_5_` TEXT,`_6_` TEXT)");

 //We get the table column names.
 QSqlRecord record = db.record(tableName);
 QString columns;
 for(int i=0;i<record.count();i++)
  {
   //columns=columns + record.fieldName(i);
   columns+="_"+QString::number(i)+"_";
   if(i!=record.count()-1) columns=columns + ",";
  }
 query.exec("SELECT "+columns+" FROM `"+tableName+"`");
 //Load data into the table
 int i=-1;
 while(query.next())
  {
   i++;
   if(i>=table->rowCount()) newRow();
   for(int j=0;j<table->columnCount();j++)
    {
     QString text = query.value(j).toString();
     item= new QTableWidgetItem(QTableWidgetItem::Type);
     item->setText(text);
     if(table->item(i,j)!=0) delete table->takeItem(i,j);
     table->setItem(i,j,item); 
    }
  }
 if(didntexist)
 {
  table->setRowCount(9);
 } else table->setRowCount(i+1);
}

void qOrganizer::saveTTLabelsDB()
{
 QTextStream cout(stdout, QIODevice::WriteOnly);
 if(!db.isOpen()) connectToDB();
 QSqlQuery query(db); 

 QString tableName = "timetable_labels";
  //No we re create it with the new structure;
 query.exec("CREATE TABLE `"+tableName+"` (`_label_` TEXT)");
 query.exec("DELETE FROM `"+tableName+"`");

 for(int i=0;i<englishList.count();i++)
   {
    query.prepare("INSERT INTO `"+tableName+"` VALUES(:label"+QString::number(i)+")");
    query.bindValue(":label"+QString::number(i),englishList[i]);
    query.exec();
   }
 
}

void qOrganizer::saveTimeTableDB()
{
 QTextStream cout(stdout, QIODevice::WriteOnly);
 cout << "Saving timetable to database \n";
 saveTTLabelsDB();
 QString tableName="timetable";
 if(C_TIMETABLE_INDEX==0) tableName="timetable"; else tableName="timetable_even";
 if(!db.isOpen()) connectToDB();
 QSqlQuery query(db); 
 
 //Delete it's contents
 query.exec("DROP TABLE `"+tableName+"`");
 QString columns="(";
 for(int i=0;i<table->columnCount();i++)
  {
   columns+="`_"+QString::number(i)+"_`"+" TEXT";
   if(i!=table->columnCount()-1) columns=columns+",";
  } 
 columns+=")";

 //No we re create it with the new structure;
 query.exec("CREATE TABLE `"+tableName+"` "+columns);
 
 int x=table->currentRow();
 int y=table->currentColumn();
  
 for(int i=0;i<table->rowCount();i++)
 { 
  textList.clear();
  for(int j=0;j<table->columnCount();j++)
   {
    table->setCurrentCell(i,j); 
    QTableWidgetItem *item=table->currentItem();
    if(item!=0)
     textList.append(item->text());
      else textList.append("");
   }
 
  QStringList valueList;
  valueList.clear();
  QString values="";
  QString value="";
  for(int i=0;i<table->columnCount();i++)
  {
   value=":my"+QString::number(i);
   valueList.append(value);
   values += value;
   if(i!=table->columnCount()-1) values += ",";
  }
 
  query.prepare("INSERT INTO `"+tableName+"` VALUES("+values+")");
  for(int i=0;i<valueList.count();i++)
    query.bindValue(valueList[i],textList[i]);
  query.exec();
 } 
  table->setCurrentCell(x,y);  
}



void qOrganizer::setSubjectLabelsDB()
{
 QTextStream cout(stdout, QIODevice::WriteOnly);
 if(!db.isOpen()) connectToDB();
 bool didntExists=0;
 QString tableName="subjects";
 QSqlQuery query(db); 
 QStringList subjects;
 subjects.clear();
 //create if it doesn't exist 
 if(!tableExistsDB(tableName)) didntExists=1; 
 query.exec("CREATE TABLE `"+tableName+"` (`_subject_` TEXT)");
 if(didntExists) 
  for(int i=0;i<subjectList.count();i++)
   query.exec("INSERT INTO "+tableName+" VALUES ('"+subjectList[i]+"')");
 //Extract the labels from those records.
 query.exec("SELECT _subject_ FROM `"+tableName+"`");
 while(query.next())
   subjects.append(query.value(0).toString());
 //now we have all subjects in the QStringList subjects 
 markTable->setRowCount(subjects.size()); 
 absenceTable->setRowCount(subjects.size());
 markTable->setVerticalHeaderLabels(subjects);
 subjectList = subjects;
}

void qOrganizer::loadMarksTableDB()
{
 QTextStream cout(stdout, QIODevice::WriteOnly);
 cout << "Loading marks from database \n";
 markTable->clearContents();
 setSubjectLabelsDB();
 if(!db.isOpen()) connectToDB();
 QString tableName="marks";
 QSqlQuery query(db); 
 bool didntexist=0;
 if(!tableExistsDB(tableName)) didntexist=1;
 //Create table if it does not exist.
 query.exec("CREATE TABLE `"+tableName+
            "` (`_0_` TEXT,`_1_` TEXT,`_2_` TEXT,`_3_` TEXT"+
            ",`_4_` TEXT,`_5_` TEXT)");

 //We get the table column names.
 QSqlRecord record = db.record(tableName);
 QString columns;
 for(int i=0;i<record.count();i++)
  {
   columns+="_"+QString::number(i)+"_";
   if(i!=record.count()-1) columns=columns + ",";
  }
 markTable->setColumnCount(record.count());
 query.exec("SELECT "+columns+" FROM `"+tableName+"`");
 //Load data into the table
 int i=-1;
 while(query.next())
  {
   i++;
   for(int j=0;j<markTable->columnCount();j++)
    {
     QString text = query.value(j).toString();
     item= new QTableWidgetItem(QTableWidgetItem::Type);
     item->setText(text);
     if(markTable->item(i,j)!=0) delete markTable->takeItem(i,j);
     markTable->setItem(i,j,item); 
    }
  }
 for(int a=0;a<markTable->columnCount();a++) markTable->setColumnWidth(a,50);
 
}

void qOrganizer::saveSubjectLabelsDB()
{
 QTextStream cout(stdout, QIODevice::WriteOnly);
 if(!db.isOpen()) connectToDB();
 QString tableName="subjects";
 QSqlQuery query(db); 
 //create if it doesn't exist 
 query.exec("CREATE TABLE `"+tableName+"` (`_subject_` TEXT)");
 query.exec("DELETE FROM `"+tableName+"`");
 foreach(QString text,subjectList)
  {
   query.prepare("INSERT INTO "+tableName+" VALUES (:subject)");
   query.bindValue(":subject",text);
   query.exec();
  }
}

void qOrganizer::saveMarksTableDB()
{ 
 QTextStream cout(stdout, QIODevice::WriteOnly);
 cout << "Saving marks to database \n";
 saveSubjectLabelsDB();
 QString tableName="marks";
 if(!db.isOpen()) connectToDB();
 QSqlQuery query(db); 
 //Delete it's contents
 query.exec("DROP TABLE `"+tableName+"`");
 QString columns="(";
 for(int i=0;i<markTable->columnCount();i++)
  {
   columns+="`_"+QString::number(i)+"_`"+" TEXT";
   if(i!=markTable->columnCount()-1) columns=columns+",";
  } 
 columns+=")";
 //No we re create it with the new structure;
 query.exec("CREATE TABLE `"+tableName+"` "+columns);
 int x=markTable->currentRow();
 int y=markTable->currentColumn();
 for(int i=0;i<markTable->rowCount();i++)
 { 
  textList.clear();
  for(int j=0;j<markTable->columnCount();j++)
   {
    markTable->setCurrentCell(i,j); 
    QTableWidgetItem *item=markTable->currentItem();
    if(item!=0)
     textList.append(item->text());
      else textList.append("");
   }
  QStringList valueList;
  valueList.clear();
  QString values="";
  QString value="";
  for(int i=0;i<markTable->columnCount();i++)
  {
   value=":my"+QString::number(i);
   valueList.append(value);
   values += value;
   if(i!=markTable->columnCount()-1) values += ",";
  }
  query.prepare("INSERT INTO `"+tableName+"` VALUES("+values+")");
  for(int i=0;i<valueList.count();i++)
    query.bindValue(valueList[i],textList[i]);
  query.exec();
 } 
  markTable->setCurrentCell(x,y);  
}

void qOrganizer::loadAbsenceTableDB()
{
 QTextStream cout(stdout, QIODevice::WriteOnly);
 cout << "Loading absences from database \n";
 absenceTable->clearContents();
 if(!db.isOpen()) connectToDB();
 QString tableName="absences";
 QSqlQuery query(db); 
 bool didntexist=0;
 if(!tableExistsDB(tableName)) didntexist=1;
 //Create table if it does not exist.
 query.exec("CREATE TABLE `"+tableName+
            "` (`_0_` TEXT,`_1_` TEXT,`_2_` TEXT,`_3_` TEXT"+
            ",`_4_` TEXT,`_5_` TEXT)");

 //We get the table column names.
 QSqlRecord record = db.record(tableName);
 QString columns;
 for(int i=0;i<record.count();i++)
  {
   columns+="_"+QString::number(i)+"_";
   if(i!=record.count()-1) columns=columns + ",";
  }
 absenceTable->setColumnCount(record.count());
 query.exec("SELECT "+columns+" FROM `"+tableName+"`");
 //Load data into the table
 int i=-1;
 while(query.next())
  {
   i++;
   for(int j=0;j<absenceTable->columnCount();j++)
    {
     QString text = query.value(j).toString();
     item= new QTableWidgetItem(QTableWidgetItem::Type);
     item->setText(text);
     if(absenceTable->item(i,j)!=0) delete absenceTable->takeItem(i,j);
     absenceTable->setItem(i,j,item); 
    }
  }
 for(int a=0;a<absenceTable->columnCount();a++) absenceTable->setColumnWidth(a,80);
 
}

void qOrganizer::saveAbsenceTableDB()
{
 QTextStream cout(stdout, QIODevice::WriteOnly);
 cout << "Saving absences to database \n";
 saveSubjectLabelsDB(); //save them again
 QString tableName="absences";
 if(!db.isOpen()) connectToDB();
 QSqlQuery query(db); 
 //Delete it's contents
 query.exec("DROP TABLE `"+tableName+"`");
 QString columns="(";
 for(int i=0;i<absenceTable->columnCount();i++)
  {
   columns+="`_"+QString::number(i)+"_`"+" TEXT";
   if(i!=absenceTable->columnCount()-1) columns=columns+",";
  } 
 columns+=")";
 //No we re create it with the new structure;
 query.exec("CREATE TABLE `"+tableName+"` "+columns);
 int x=absenceTable->currentRow();
 int y=absenceTable->currentColumn();
 for(int i=0;i<absenceTable->rowCount();i++)
 { 
  textList.clear();
  for(int j=0;j<absenceTable->columnCount();j++)
   {
    absenceTable->setCurrentCell(i,j); 
    QTableWidgetItem *item=absenceTable->currentItem();
    if(item!=0)
     textList.append(item->text());
      else textList.append("");
   }
  QStringList valueList;
  valueList.clear();
  QString values="";
  QString value="";
  for(int i=0;i<absenceTable->columnCount();i++)
  {
   value=":my"+QString::number(i);
   valueList.append(value);
   values += value;
   if(i!=absenceTable->columnCount()-1) values += ",";
  }
  query.prepare("INSERT INTO `"+tableName+"` VALUES("+values+")");
  for(int i=0;i<valueList.count();i++)
    query.bindValue(valueList[i],textList[i]);
  query.exec();
 } 
  absenceTable->setCurrentCell(x,y);  
}

//Saving between txt and DB
void qOrganizer::saveAlltoTXT()
{
 int temp=C_STORINGMODE; 
 /*Quick and dirty hack to keep me from 
  getting those ifs out of the loading functions.
  I know, I know, lazyness is bliss :D */
 C_STORINGMODE=0;
 saveToDoList();
 saveTimeTable();
 saveSchedule();
 saveJurnal();
 saveMarksTable();
 saveAbsenceTable();
 saveMarkDates();
 C_STORINGMODE=temp;
}

void qOrganizer::saveAlltoSQLiteDB()
{ 
 bool changedAS=0;
 if(C_AUTOSAVE_TOGGLE)
  {
   toggleAutoSave(0);
   changedAS=1;
   C_AUTOSAVE_TOGGLE=0;
  }
 int temp=C_STORINGMODE; 
 if((!db.isOpen())||(C_STORINGMODE!=1))
 {
  if(db.isOpen()) db.close();
  C_STORINGMODE=1;
  connectToDB();
 }
 if(C_STORINGMODE==1)
  {
   saveToDoListDB();
   saveTimeTableDB();
   saveScheduleDB();
   saveJurnalDB();
   saveMarksTableDB();
   saveAbsenceTableDB();
   saveMarkDatesDB();
  }
 C_STORINGMODE=temp;
 if(changedAS)
  { 
   toggleAutoSave(1);
   C_AUTOSAVE_TOGGLE=1;
  }
}

void qOrganizer::saveAlltoMySQLDB()
{
 bool changedAS=0;
 if(C_AUTOSAVE_TOGGLE)
  {
   toggleAutoSave(0);
   changedAS=1;
   C_AUTOSAVE_TOGGLE=0;
  }
 int temp=C_STORINGMODE; 
 if((!db.isOpen())||(C_STORINGMODE!=2))
 {
  if(db.isOpen()) db.close();
  C_STORINGMODE=2;
  connectToDB();
 }
 if(C_STORINGMODE==2)
  {
   saveToDoListDB();
   saveTimeTableDB();
   saveScheduleDB();
   saveJurnalDB();
   saveMarksTableDB();
   saveAbsenceTableDB();
   saveMarkDatesDB();
  }
 C_STORINGMODE=temp;
 if(changedAS)
  { 
   toggleAutoSave(1);
   C_AUTOSAVE_TOGGLE=1;
  }
}


//-------------------------------------------------------SEARCH---------------------------------------------------------------
//This is responsible for searching through events

bool caseInsensitiveLessThan(const QString &s1, const QString &s2)
{
 return s1.toLower() < s2.toLower();
}

void qOrganizer::search()
{
 QTextStream cout(stdout, QIODevice::WriteOnly);
 QString searchString = searchField->text();
 QList<QString> searchResultList;
 QString tempText;
 dateList.clear();
 if(C_STORINGMODE==0)
  {
   setCalDir();
   QDir dir = QDir::current();
   QStringList entries = dir.entryList(QDir::Files|QDir::NoDotAndDotDot,QDir::Name);
   foreach (QString entry, entries)
    {
     QFile file(entry);
     if(file.open(QFile::ReadOnly))
      {
       QTextStream in(&file);
       in.setCodec("UTF-8");
       QString text=in.readAll();
       if(text.indexOf(searchString,0,Qt::CaseInsensitive)!=-1)
        { 
         cout <<"Found:"<< entry <<"\n";
         searchResultList.append(entry);
        }
       file.close();
      }
    }
  }
  else
   {
     if(!db.isOpen()) connectToDB();
     QSqlQuery query(db); 
     QStringList tableList = db.tables(QSql::Tables);
     QStringList scheduleList = tableList.filter("schedule");
     QStringList jurnalList = tableList.filter("jurnal");
     QStringList entryList = scheduleList + jurnalList;
     qSort(entryList.begin(), entryList.end(), caseInsensitiveLessThan);
     foreach(QString entry,entryList)
      {  
       if(entry.indexOf("schedule")!=-1)
        {
         tempText="";
         query.exec("SELECT _event_,_from_,_until_,_reminder_ FROM `"+entry+"`");
         while(query.next())
          {
           tempText+=query.value(0).toString()+"|"+query.value(1).toString()+"|"+
                     query.value(2).toString()+"|"+query.value(3).toString()+"|"+"\n";
          }
         if(tempText.indexOf(searchString,0,Qt::CaseInsensitive)!=-1)
          { 
           cout <<"Found:"<< entry <<"\n";
           searchResultList.append(entry);
          }
        }
        else //jurnal
         {
          query.exec("SELECT _text_ FROM `"+entry+"`");
          while(query.next())
           {
            QString text = query.value(0).toString();
            if(text.indexOf(searchString,0,Qt::CaseInsensitive)!=-1)
             {
              cout << "Found:" << entry<<"\n";
              searchResultList.append(entry);
             }
           }
         }
      }
   }
 foreach(QString entry,searchResultList)
  {
   entry.remove(QString(".txt"),Qt::CaseSensitive);
   entry.remove(QString(".html"),Qt::CaseSensitive);
   entry.remove(QString("schedule_"),Qt::CaseSensitive);
   entry.remove(QString("jurnal_"),Qt::CaseSensitive);    
   QDate date = QDate::fromString(entry,"MM_dd_yyyy");
   dateList.append(date);
  }  
}

void qOrganizer::searchPrev()
{
 QString searchedString = searchField->text();
 if(searchedString!=oldSearchString)
  {
   searchCurrentIndex=0;
   search();
  }
 if(searchCurrentIndex<0) searchCurrentIndex=0;
 if(searchCurrentIndex==0) searchCurrentIndex=dateList.count();
 if((dateList.isEmpty())&&(!searchedString.isEmpty())) 
  {
   QMessageBox::warning(NULL, "qOrganizer",tr("No entries found!"), "OK");
  }
   else
    if((!searchedString.isEmpty())&&(searchedString!="|"))
     {
      if(searchCurrentIndex>0) 
       {
        searchCurrentIndex--;
        calendar->setSelectedDate(dateList[searchCurrentIndex]);
        updateDay(); 
        if(C_STORINGMODE==0) loadJurnal(); else loadJurnalDB();
        if(C_STORINGMODE==0) loadSchedule(); else loadScheduleDB();
        setCalendarColumnWidths();
       }
     }
 
 oldSearchString=searchedString;
}

void qOrganizer::searchNext()
{
 QString searchedString = searchField->text();
 if(searchedString!=oldSearchString)
  {
   searchCurrentIndex=-1;
   search();
  }
 if(searchCurrentIndex>=dateList.count()-1) searchCurrentIndex=-1;
 if((dateList.isEmpty())&&(!searchedString.isEmpty())) 
  {
   QMessageBox::warning(NULL, "qOrganizer",tr("No entries found!"), "OK");
  }
  else
   {
    if((!searchedString.isEmpty())&&(searchedString!="|"))
     {
      if(searchCurrentIndex<dateList.count()-1)
       {
        searchCurrentIndex++;
        calendar->setSelectedDate(dateList[searchCurrentIndex]);
        updateDay(); 
        if(C_STORINGMODE==0) loadJurnal(); else loadJurnalDB();
        if(C_STORINGMODE==0) loadSchedule(); else loadScheduleDB();
        setCalendarColumnWidths();
       }
     }
    }
 oldSearchString=searchedString;
}

//-------------------------------------------REMEMBER COLUMN WIDTHS------------------------------------------------------------

void qOrganizer::saveColumnWidths()
{
 cout << "Saving column widths\n";
 //QSettings settings("qOrganizer", "qOrganizer");
 //Schedule table
 for(int i=0;i<tableWid->columnCount();i++)
  settings -> setValue("schedule"+QString::number(i),tableWid->columnWidth(i));
 
 //To-Do List
 for(int i=0;i<list->columnCount();i++)
  settings -> setValue("todo"+QString::number(i),list->columnWidth(i));
 
 //TimeTable
 for(int i=0;i<table->columnCount();i++)
  settings -> setValue("timetable"+QString::number(i),table->columnWidth(i));
 
 //Mark table
 for(int i=0;i<markTable->columnCount();i++)
  settings -> setValue("marktable"+QString::number(i),markTable->columnWidth(i));
 
 //Absence table
 for(int i=0;i<absenceTable->columnCount();i++)
  settings -> setValue("absencetable"+QString::number(i),absenceTable->columnWidth(i));
 
}

void qOrganizer::setColumnWidths()
{
 cout << "Setting column widths\n";
 //QSettings settings("qOrganizer", "qOrganizer");
 //Schedule table
 for(int i=0;i<tableWid->columnCount();i++)
  if(settings -> value("schedule"+QString::number(i)).toInt()!=0)
   tableWid->setColumnWidth(i,settings -> value("schedule"+QString::number(i)).toInt());
 
 //To-Do List
 for(int i=0;i<list->columnCount();i++)
  if(settings -> value("todo"+QString::number(i)).toInt()!=0)
   list->setColumnWidth(i,settings -> value("todo"+QString::number(i)).toInt());
 
 //TimeTable
 for(int i=0;i<table->columnCount();i++)
  if(settings -> value("timetable"+QString::number(i)).toInt()!=0)
   table->setColumnWidth(i,settings -> value("timetable"+QString::number(i)).toInt());
 
 //Mark table
 for(int i=0;i<markTable->columnCount();i++)
  if(settings -> value("marktable"+QString::number(i)).toInt()!=0)
   markTable->setColumnWidth(i,settings -> value("marktable"+QString::number(i)).toInt());
 
 //Absence table
 for(int i=0;i<absenceTable->columnCount();i++)
  if(settings -> value("absencetable"+QString::number(i)).toInt()!=0)
   absenceTable->setColumnWidth(i,settings -> value("absencetable"+QString::number(i)).toInt());
}

void qOrganizer::saveCalendarColumnWidths()
{
 cout << "Saving calendar column widths\n";
 //QSettings settings("qOrganizer", "qOrganizer");
 //Schedule table
 for(int i=0;i<tableWid->columnCount();i++)
  settings -> setValue("schedule"+QString::number(i),tableWid->columnWidth(i));
}

void qOrganizer::setCalendarColumnWidths()
{
 cout << "Setting calendar column widths\n";
 //QSettings settings("qOrganizer", "qOrganizer");
 //Schedule table
 for(int i=0;i<tableWid->columnCount();i++)
  if(settings -> value("schedule"+QString::number(i)).toInt()!=0)
   tableWid->setColumnWidth(i,settings -> value("schedule"+QString::number(i)).toInt());
}

//-----------------------------------------ODD or EVEN TIMETABLE-----------------------------------------------

void qOrganizer::changeTTIndex(int ttindex)
{
 if(C_AUTOSAVE_TOGGLE) 
  if(C_STORINGMODE==0) saveTimeTable(); else saveTimeTableDB();
 calledFromTTCombo=1;
 C_TIMETABLE_INDEX=ttindex;
 table->clearContents();
 if(C_STORINGMODE==0) loadTimeTable(); else loadTimeTableDB();
 calledFromTTCombo=0;
}

//-------------------------------------DATE STORING FOR BOOKLET ----------------------------------------------------

inline bool operator<(const CoordinatePair &p1, const CoordinatePair &p2)
{
 if((p1.x!=p2.x)&&(p1.y!=p2.y))
  { 
   return p1.x < p2.x;
  } 
  else if(p1.x==p2.x) return p1.y < p2.y; 
   else if(p1.y==p2.y) return p1.x < p2.x;
  return 0;
}

void qOrganizer::loadMarkDates()
{
 if(C_STORINGMODE!=0) loadMarkDatesDB(); else
 {
  cout << "Loading mark dates\n";
  setBDir();
  QFile markDateFile("markdates.txt"); 
  map.clear(); //clear it
  QTextStream cout(stdout, QIODevice::WriteOnly);
  //cout << "Entered loadMarkDate()\n";
  if (markDateFile.open(QFile::ReadOnly)) 
  {
   //cout << "File opened\n";
   QTextStream in(&markDateFile);
   in.setCodec("UTF-8");
   int i=-1;
   while(!in.atEnd()) 
    {
     QString line=in.readLine(); 
     i++;
     QStringList slist = line.split(" ", QString::KeepEmptyParts);
     bool ok;
     //cout << slist[0]<<" "<<slist[1]<<" "<<slist[2]<<"\n";
     int a = slist[0].toInt(&ok,10);
     int b = slist[1].toInt(&ok,10);
     CoordinatePair pair(a,b);
     QDate date = QDate::fromString(slist[2],"d/M/yyyy");
     map.insert(pair,date);
     //cout << "Inserted into map\n";
    }  
  } else cout << "Could not open mark dates file\n";
  markDateFile.close();
 }
}

void qOrganizer::loadMarkDatesDB()
{
 QTextStream cout(stdout, QIODevice::WriteOnly);
 cout << "Saving mark dates to database \n";
 QString tableName="markdates";
 if(!db.isOpen()) connectToDB();
 map.clear();
 QSqlQuery query(db); 
 query.exec("CREATE TABLE `"+tableName+"` (`_x_` TEXT,`_y_` TEXT,`_date_` TEXT)");
 query.exec("SELECT _x_,_y_,_date_ FROM `"+tableName+"`");
 while(query.next())
  {
   int a = query.value(0).toInt();
   int b = query.value(1).toInt();
   CoordinatePair pair(a,b);
   QString dateString = query.value(2).toString(); 
   QDate date = QDate::fromString(dateString,"d/M/yyyy");
   map.insert(pair,date);
  }
}

void qOrganizer::updateDate(int currentX,int currentY,int prevX,int prevY)
{
 Q_UNUSED(prevX);
 Q_UNUSED(prevY);
 CoordinatePair pair(currentX,currentY);
 QDate returnedDate = map.value(pair,QDate::currentDate());
 markDateField -> setDate(returnedDate);
}

void qOrganizer::saveDate(QDate date)
{
 int a=markTable->currentRow();
 int b=markTable->currentColumn();
 CoordinatePair pair(a,b);
 map.remove(pair);
 map.insert(pair,date);
 //saveMarkDates(); to CPU hungry specially with database
}

void qOrganizer::saveMarkDates()
{
 if(C_STORINGMODE!=0) saveMarkDatesDB(); else 
 {
  setBDir();
  QFile markDatesFile("markdates.txt"); 
  cout <<"Saving mark dates to file\n";

  if (markDatesFile.open(QFile::WriteOnly)) 
  {
   QTextStream out(&markDatesFile);
   out.setCodec("UTF-8");
   QMapIterator<CoordinatePair,QDate> i(map);
   while (i.hasNext()) 
    {
     i.next();
     out << i.key().x <<" "<< i.key().y <<" "<< i.value().toString("d/M/yyyy")<<"\n";
    }
   markDatesFile.close();
  }
  else 
   {
     QMessageBox::warning(NULL, "qOrganizer", "Unable to write mark dates into file.", "OK");
   }
 }
}

void qOrganizer::saveMarkDatesDB()
{
 QTextStream cout(stdout, QIODevice::WriteOnly);
 cout << "Saving mark dates to database \n";
 QString tableName="markdates";
 if(!db.isOpen()) connectToDB();
 QSqlQuery query(db); 
 query.exec("CREATE TABLE `"+tableName+"` (`_x_` TEXT,`_y_` TEXT,`_date_` TEXT)");
 query.exec("DELETE FROM `"+tableName+"`");
 QMapIterator<CoordinatePair,QDate> i(map);
 while (i.hasNext()) 
  {
   i.next();
   //out << i.key().x <<" "<< i.key().y <<" "<< i.value().toString("d/M/yyyy")<<"\n";
   query.exec("INSERT INTO `"+tableName+"` VALUES ('"+QString::number(i.key().x)+"','"+QString::number(i.key().y)+"','"+i.value().toString("d/M/yyyy")+"')");
   //I wont prepare it now since the values have a strict format and the user can't modify it to cause problems
  }
}

qOrganizer::~qOrganizer()
{
}
