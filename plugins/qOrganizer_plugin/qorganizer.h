/***************************************************************************
 *   Copyright (C) 2007 by Balázs Béla                                     *
 *   balazsbela@gmail.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2               
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


#ifndef QORGANIZER_H
#define QORGANIZER_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QStackedWidget>
#include <QCalendarWidget>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QToolButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QFontComboBox>
#include <QTextCursor>
#include <QDir>
#include <QFtp>
#include <QFile>
#include <iostream>
#include <QSystemTrayIcon>
#include <string>
#include <vector>
#include <QMenu>
#include <QStack>
#include <QQueue>
#include <QVector>
#include <QProgressDialog>
#include <QKeyEvent>
#include <QSound>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QSettings>
#include <QUrl>
#include <QPixmap>
#include "settings.h"
#include "delegates.h"

class QApplication;

//Custom QTableWidget
/*
  This is a subclass of QTableWidget so we can emit a signal every time in looses focus.
  To this signal are connected the saving functions for the data container widgets
*/
class CQTableWidget:public QTableWidget
{
   Q_OBJECT

 public:
  bool isBeingEdited();

 public: signals:
   void focusLost();
 
 protected:
    void focusOutEvent(QFocusEvent * event);
    //We need to reimplement this to be able to delete absences,
    void keyPressEvent (QKeyEvent * event);
 
};


//Custom QTableWidget for the same thing.

class CQTextEdit:public QTextEdit
{
 Q_OBJECT
  
 signals:    
   void focusLost();
 
 protected:
 void focusOutEvent(QFocusEvent * event);
 /*void dragEnterEvent(QDragEnterEvent *event);
 //void dropEvent(QDropEvent *e);*/
 void insertFromMimeData( const QMimeData *source );
 bool canInsertFromMimeData( const QMimeData *source ) const;
 
 
};

//LineEdit with clear button on the right.

class LineEdit : public QLineEdit
{
    Q_OBJECT

public:
    LineEdit(qOrganizer *parent = 0);

protected:
    void resizeEvent(QResizeEvent *);

private slots:
    void updateCloseButton(const QString &text);
    void clearAndSetDateBack();
private:
    QToolButton *clearButton;
    qOrganizer *Parent;
};


//coordinate class for storing dates in a QMap

class CoordinatePair
{
 public:
  CoordinatePair(int a, int b)
   {
    x=a;
    y=b;
   }

  int x;
  int y;
   
};

//about Dialog
class aboutDialog : public QDialog
{
  Q_OBJECT
  public :
   aboutDialog();
   ~aboutDialog();
  public slots:
  void close();
  private:
   QTabWidget *tabWidget;
   QWidget *textWidget;
   QWidget *thanksWidget;
   QLabel *aboutLabel;
   QLabel *thanksToLabel;
   QLayout *thanksLayout;
   QHBoxLayout *textLayout;
   QVBoxLayout *layout;
   QHBoxLayout *okLayout;
   QPushButton *okButton; 
};


//Our class


class qOrganizer:public QMainWindow
{
      Q_OBJECT

public:
      qOrganizer();
      ~qOrganizer();
      void createActions();
      void createMenus();
      void createToolBars();
      void createStatusBar();
      void addItems();
      void addCalendarPage();
      void addToDoPage(); 
      void addCalendarPageWidgets();
      void addTimeTable();
      void addBookletPage();
      void connectCallendarPageWidgets();
      void layoutCalendarPageWidgets();     
      void closeEvent(QCloseEvent *e);
      void emptyTable();
      void setCalDir();
      void setTDDir();
      void setTTDir();
      void setBDir();
      void setConfigDir();
      void setSTray();
      void hideEvent(QHideEvent * event);
      void printCalendar();
      void printToDoList();
      void printTimeTable();
      void printBooklet();
      void splitMyString(QString s);
      void writeDefaultConfigFile();
      void writeSettings();
      void readSettings();
      double calculateRowAverage(int row);
      void toggleAutoSave(bool ind); 
      void playSound();
      //DATABASE STUFF: 
      void connectToDB(); 
      void setDBDir();
      bool tableExistsDB(QString &tablename);
      
  
      QLabel *noteLabel;
      QString selDate;
      QComboBox *comboSize;
      QLabel *day;
      QHBoxLayout *buttonLayout;
      QPushButton *deleteButton;
      QPushButton *newButton;
      QTextEdit *textField;
      QHBoxLayout *mainLayout;
      QVBoxLayout *textLayout;  
      CQTableWidget *tableWid;
      QStackedWidget *mainWid;
      QMenu *fileMenu;
      QMenu *viewMenu;
      QMenu *settingsMenu;
      QMenu *helpMenu;
      QMenu *ftpMenu; 
      QMenu *saveToMenu;
      QToolBar *fileToolBar;
      QToolBar *editToolBar;
      QAction *calAct;
      QAction *bookletAct;
      QAction *toDoAct;
      QAction *saveAct;
      QAction *timeTableAct;
      QAction *exitAct;
      QAction *aboutAct;
      QAction *aboutQtAct;
      QAction *printAct;
      QAction *uploadAct;
      QAction *downloadAct;
      QAction *settingsAct;
      QAction *saveToDBAct;
      QAction *saveToTXTAct;
      QAction *saveToMySQLDBAct;
      QWidget *CallWid;
      QVBoxLayout *vl;
      QCalendarWidget *calendar;
      QWidget *CalendarPage;
      QFontComboBox *fontBox;
      QVBoxLayout *ToDoVl;
      QWidget *ToDoPage;
      QStringList englishList;
      QStringList universalTTlabels;
      QWidget *timeTablePage;
      CQTableWidget *list;
      QTextCursor cursor;
      QFont font;
      QPushButton *B;
      QPushButton *I;
      QPushButton *U;
      QTableWidgetItem* item;
      QDir *dataDir;
      
      QPushButton *newTaskButton;
      QPushButton *deleteTaskButton;
      QVBoxLayout *mainHl;
      QHBoxLayout *buttonVl;
 
      CQTableWidget *table;
      QPushButton *newTTRow;
      QPushButton *delTTRow;
      QPushButton *newTTCmn;
      QPushButton *delTTCmn;
      QVBoxLayout *Vl;
      QHBoxLayout *Hl;
      QHBoxLayout *BHl;
      QStringList TDlabels;
      QStringList TTlabels;
      QStringList Clabels;
      QHBoxLayout *ttLayout;
      QHBoxLayout *centerLayout;
      QLabel *ttLabel;
      QComboBox *ttCombo;
 
      QSystemTrayIcon *tray;
      QPrinter *printer; 
      
      QWidget *bookletPage;
      CQTableWidget *markTable;
      CQTableWidget *absenceTable;
      QPushButton *newMarkButton;
      QPushButton *deleteMarkButton;
      QPushButton *newSubjectButton;
      QPushButton *deleteSubjectButton;
      QPushButton *newAbsenceButton;
      QPushButton *deleteAbsenceButton;
      QStringList subjectList;

      QLineEdit *averageField;
      QLineEdit *totalAverageField;
      QLineEdit *absenceNrField;
      QDateEdit *markDateField;

      QTimer *timer;
      
      std::vector<std::string> v;
      QMenu *contextMenu;

      //CONFIG VARIABLES
      QString C_PATH;
      QString C_DT_FORMAT;   //date time format
      unsigned long long int C_TIMEOUT;
      unsigned int C_FIRST_DAY_OF_WEEK;
      bool C_AUTOSAVE_TOGGLE;
      bool C_BALOON_SET;
      unsigned int C_NRROWS;
      //Ftp settings
      bool C_SYNCFTP;
      QString C_HOST;
      unsigned int C_PORT;
      QString C_USER;
      QString C_PASSWD;
      QString C_FTPPATH;
      QString C_LANGUAGETEXT;
      bool C_LOAD_ON_CHANGE;
      bool C_SYSTRAY;
      bool C_SOUND;
      bool C_SHOW_SAVE;
      unsigned int C_STORINGMODE;
      unsigned int C_OLD_STORINGMODE;
      //MySQL stuff
      QString C_MYSQL_HOST; 
      unsigned int C_MYSQL_PORT;
      QString C_MYSQL_USER;
      QString C_MYSQL_PASSWD;
      QString C_MYSQL_DB;
      unsigned int C_TIMETABLE_INDEX;
      bool calledFromTTCombo;
      bool C_USE_ODDTT;
      bool C_TT_REVERSORDER;
      bool C_ROUND_ON_AVERAGE;
      //Settings dialog
      SettingsDialog *settingsDialog;
      //global settings object
      QSettings *settings;
      QString globalWDirPath;
       
      QFtp *ftp;
      QStringList *ftpEntryes;
      QStack<QString> stack;
      QVector<int> listCommandVector;
      QVector<int> getCommandVector;
      QVector<int> putCommandVector;
      QList<QUrlInfo> entryList;
      QList<QUrlInfo> listedFiles;
      QList<QUrlInfo> listedFolders;
      int listCommand; 
      QString lpath;
      QFile *ftpFile;
      QQueue<QFile*> fileQueue;
      QQueue<QFile*> uploadedFileQueue;
      bool noFolders;
      bool changedAutoSave;

     //Delegates:
      absenceDelegate absDelegate;
      markDelegate marksDelegate;
      todoDelegate tdlistDelegate;
      scheduleDelegate schDelegate;
      timeTableDelegate ttDelegate;
     //Status progress bar
      QProgressBar *statusProgress;
      
      QSound *sound; //reminder sound
      //Database stuff:
      QList<QString> textList;
      QSqlDatabase db;
      QString dbPath;
      bool connectionExists;
      
      //Search stuff
      QList<QDate> dateList;
      QLabel *searchLabel;
      LineEdit *searchField;
      QPushButton *searchPrevButton;
      QPushButton *searchNextButton;
      QHBoxLayout *searchLayout;
      int searchCurrentIndex;
      QString oldSearchString;

      bool downloadingFromFTP;
   
      //Dates in booklet 
      QMap<CoordinatePair,QDate> map;
      
      //about dialog;
      aboutDialog *aboutD;
      public slots:
      void about();
      void ChangeToCalendar();
      void ChangetoToDoList(); 
      void ChangeToTimeTable();
      void ChangeToBooklet();
      void insertRowToEnd();  
      void deleteRow();
      void updateDay();
      void modifyText();
      void setFont();
      void loadSchedule();
      void saveSchedule(); 
      void saveJurnal();
      void loadJurnal();
       
      void newTask();
      void deleteTask();
      void saveToDoList();
      void loadToDoList();

      void newRow();
      void newColumn();
      void delColumn();
      void delRow();
      void saveTimeTable(); 
      void loadTimeTable();

      void newMark(); 
      void newSubject();
      void deleteMark();
      void deleteSubject();
      void newAbsence();
      void deleteAbsence(); 
      void saveMarksTable();
      void saveAbsenceTable();
      void loadMarksTable();
      void loadAbsenceTable();

      void trayActivated(QSystemTrayIcon::ActivationReason reason);
      void toggleVisibility();
      void printPage();
 
      void updateAbsenceNr();
      void updateAverage(int nx,int ny,int px,int py);
      void updateTotalAverage();
      void setReminders();
      void remind();
     
      void readConfigFile();
      void showSettings();
      void saveConfigFile();
       
      void initUpload();
      void putToFTP();
      void getFromFTP();
      void connectToFTP();
      void processFolder(const QUrlInfo &i);
      void processCommand(int com,bool err);
      void getFolder(QString path,QString localPath);
      void sendFolder(const QString& path,const QString &dirPath);
      void downloadFiles();
      void updateStatusG(bool error);
      void updateStatusP(bool error);
      
      //DATABASE SLOTS:
      void saveScheduleDB();      
      void loadScheduleDB();
      void loadJurnalDB();
      void saveJurnalDB();
      void loadToDoListDB(); 
      void saveToDoListDB();
      void loadTimeTableDB();
      void saveTimeTableDB();        
      void setTTLabelsDB();
      void saveTTLabelsDB();
      void setSubjectLabelsDB();
      void saveSubjectLabelsDB();
      void loadMarksTableDB();
      void saveMarksTableDB();
      void loadAbsenceTableDB();
      void saveAbsenceTableDB();
      void saveAlltoSQLiteDB();
      void saveAlltoTXT();
      void saveAlltoMySQLDB();

      void saveAll();     
      void loadAll();
      void updateCalendar();

      //Search:
      void search();
      void searchPrev();
      void searchNext();
      //Saving column widths
      void saveColumnWidths();
      void setColumnWidths();
      void saveCalendarColumnWidths();
      void setCalendarColumnWidths();
      //Odd or even in timetable
      void changeTTIndex(int);
      //settings
      void getWDirPath();
      
      //marks date     
      void loadMarkDates();
      void loadMarkDatesDB();
      void saveMarkDates();
      void saveMarkDatesDB();

      void updateDate(int currentX,int currentY,int prevX,int prevY);
      void saveDate(QDate date);
      void exitApp();
      

};

#endif
