/****************************************************************
 *  RShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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


#ifndef _CONNECTIONSDIALOG_H
#define _CONNECTIONSDIALOG_H

#include <QFileDialog>
#include <QtGui>

#include "Preferences/rsharesettings.h"

#include "mainpage.h"
#include "ui_NetworkDialog.h"
#include "RsAutoUpdatePage.h"

#include "connect/ConnectDialog.h"


class NetworkDialog : public RsAutoUpdatePage 
{
  Q_OBJECT

public:
  /** Default Constructor */
  NetworkDialog(QWidget *parent = 0);
  /** Default Destructor */

  void  showpeerdetails(std::string id);

  //void load();
  virtual void updateDisplay() ; // overloaded from RsAutoUpdatePage
  
public slots:
  void  insertConnect();
  std::string loadneighbour();
  /* void loadneighbour(); */
  void setLogInfo(QString info, QColor color=QApplication::palette().color(QPalette::WindowText));


private slots:

  void makeFriend() ;
  void denyFriend() ;
  void deleteCert() ;
  void peerdetails();
  /** Create the context popup menu and it's submenus */
  void connecttreeWidgetCostumPopupMenu( QPoint point );
  
  /** Called when user clicks "Load Cert" to choose location of a Cert file */
  void loadcert();

  void authneighbour();
  void addneighbour();

  void on_actionAddFriend_activated();
  //void on_actionCopyKey_activated();
  void on_actionExportKey_activated();

  void on_actionCreate_New_Profile_activated();

  
  void on_actionClearLog_triggered();
  void displayInfoLogMenu(const QPoint& pos);
  
  void getNetworkStatus();
  void updateNetworkStatus();
  
  void loadtabsettings();
  
  void on_actionTabsright_activated();
  void on_actionTabsnorth_activated();
  void on_actionTabssouth_activated();
  void on_actionTabswest_activated();

  void on_actionTabsRounded_activated();
  void on_actionTabsTriangular_activated();


private:
  /** A RetroShare Settings object used for saving/loading settings */
  RshareSettings *_settings;

  QTreeWidgetItem *getCurrentNeighbour();

  /** Define the popup menus for the Context menu */
  QMenu* contextMnu;
  /** Defines the actions for the context menu */
  QAction* peerdetailsAct;
  QAction* denyFriendAct;
  QAction* deleteCertAct;
  QAction* makefriendAct;
  QAction* authAct;
  QAction* loadcertAct;
  QAction* exportcertAct;

  /* connection dialog */
  ConnectDialog *connectdialog;

  QTreeWidget *connecttreeWidget;
  
  class NetworkView *networkview;

  /** Qt Designer generated object */
  Ui::NetworkDialog ui;
};

#endif

