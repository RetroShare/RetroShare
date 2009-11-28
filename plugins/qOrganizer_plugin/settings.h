/***************************************************************************
 *   Copyright (C) 2007 by Balázs Béla                                     *
 *   balazsbela@gmail.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation version 2 of the License                 *
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
#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QDir>
#include <QComboBox>

class qOrganizer;

class SettingsDialog : public QDialog
{
  Q_OBJECT

  public :
   SettingsDialog(qOrganizer *parent);
   ~SettingsDialog();

   QLabel *warnLabel;
   QLabel *pathLabel;
   QLineEdit *pathEdit;
   QPushButton *pathButton;
   QLabel *rLabel;
   QSpinBox *rBox;
   QLabel *firstDayLabel;
   QComboBox *firstDay;
   QLabel *dateFormatLabel;
   QComboBox *dateFormatEdit;
   QLabel *autoLabel;
   QCheckBox  *autoBox;
   QLabel *messageLabel;
   QComboBox *messageCombo;
   QLabel *langLabel;
   QComboBox *langCombo;
   QLabel *rownrLabel;
   QSpinBox *rownrBox;
   //Labels for ftp 
   QLabel *portLabel;
   QLabel *hostLabel;
   QLabel *userLabel;
   QLabel *passLabel;
   QLabel *ftpSyncLabel;
   QLabel *ftppathLabel;
   
   QLabel *soundLabel;
   QCheckBox *soundBox; 
  
   QCheckBox *ftpBox;
   QLineEdit *hostEdit;
   QSpinBox *portBox;
   QLineEdit *userEdit;
   QLineEdit *passEdit;
   QLineEdit *ftppathEdit;
   
   QCheckBox *loadBox;
   QLabel *trayLabel;
   QCheckBox *trayBox;

   QComboBox *storingBox;
   
   QTabWidget *tabWidget;
   QWidget *General;
   QWidget *Calendar;
   QWidget *Ftp;
   QWidget *Storing;

   QCheckBox *mysqlBox;
   QLineEdit *mysqlHostEdit;
   QSpinBox  *mysqlPortBox;
   QLineEdit *mysqlUserEdit;
   QLineEdit *mysqlPassEdit;
   QLineEdit *mysqldbEdit;

   QCheckBox *oddTTBox;
   QCheckBox *reverseTTBox;
   QCheckBox *roundBox;
   QCheckBox *saveAllBox;
   void readLangDir();
  public slots :
   void setDir();
   void toggleFTP(int state);
   void toggleMySQL(int state);
   void close();
};

#endif


