/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, 2007 crypton
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


#ifndef _STARTDIALOG_H
#define _STARTDIALOG_H

#include <gui/Preferences/rsharesettings.h>
#include "rsiface/rsiface.h"
/********
#if (QT_VERSION >= 040300)
#include "qskinobject/qskinobject.h"
#endif
*******/

#include "ui_StartDialog.h"

class LogoBar;

class StartDialog : public QMainWindow
{
  Q_OBJECT

public:
  /** Default constructor */
  StartDialog(RsInit *config, QWidget *parent = 0, Qt::WFlags flags = 0);
  /** Default destructor */
  //~StartDialog();

  bool  requestedNewCert();

public slots:
  /** Overloaded QWidget.show */
  void show();
  
  LogoBar & getLogoBar() const;

protected:
  void closeEvent (QCloseEvent * event);
  
private slots:

	void closeinfodlg();
	void loadPerson();
    void createnewaccount();
  
private:

  /** Loads the saved connectidialog settings */
//  void loadSettings();
  void loadCertificates();


  /** A VidaliaSettings object that handles getting/saving settings */
  RshareSettings* _settings;
  
  LogoBar * _rsLogoBar;
  
  /** Qt Designer generated object */
  Ui::StartDialog ui;
 
/************** 
#if (QT_VERSION >= 040300)
  QSkinObject *skinobject;
#endif
**************/

  RsInit *rsConfig;

  bool reqNewCert;
};

#endif

