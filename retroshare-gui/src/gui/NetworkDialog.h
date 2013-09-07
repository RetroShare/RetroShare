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

#include "ui_NetworkDialog.h"
#include "RsAutoUpdatePage.h"

class NetworkDialog : public RsAutoUpdatePage 
{
  Q_OBJECT

  Q_PROPERTY(QColor backgroundColorSelf READ backgroundColorSelf WRITE setBackgroundColorSelf)
  Q_PROPERTY(QColor backgroundColorOwnSign READ backgroundColorOwnSign WRITE setBackgroundColorOwnSign)
  Q_PROPERTY(QColor backgroundColorAcceptConnection READ backgroundColorAcceptConnection WRITE setBackgroundColorAcceptConnection)
  Q_PROPERTY(QColor backgroundColorHasSignedMe READ backgroundColorHasSignedMe WRITE setBackgroundColorHasSignedMe)
  Q_PROPERTY(QColor backgroundColorDenied READ backgroundColorDenied WRITE setBackgroundColorDenied)

public:
  /** Default Constructor */
  NetworkDialog(QWidget *parent = 0);

  //void load();
  virtual void updateDisplay() ; // overloaded from RsAutoUpdatePage
  
  QColor backgroundColorSelf() const { return mBackgroundColorSelf; }
  QColor backgroundColorOwnSign() const { return mBackgroundColorOwnSign; }
  QColor backgroundColorAcceptConnection() const { return mBackgroundColorAcceptConnection; }
  QColor backgroundColorHasSignedMe() const { return mBackgroundColorHasSignedMe; }
  QColor backgroundColorDenied() const { return mBackgroundColorDenied; }

  void setBackgroundColorSelf(QColor color) { mBackgroundColorSelf = color; }
  void setBackgroundColorOwnSign(QColor color) { mBackgroundColorOwnSign = color; }
  void setBackgroundColorAcceptConnection(QColor color) { mBackgroundColorAcceptConnection = color; }
  void setBackgroundColorHasSignedMe(QColor color) { mBackgroundColorHasSignedMe = color; }
  void setBackgroundColorDenied(QColor color) { mBackgroundColorDenied = color; }

private:
  void  insertConnect();
//  std::string loadneighbour();
  /* void loadneighbour(); */
  //void updateNewDiscoveryInfo() ;

protected:
  void changeEvent(QEvent *e);

private slots:

	void removeUnusedKeys() ;
  void makeFriend() ;
  void denyFriend() ;
//  void deleteCert() ;
  void peerdetails();
  void copyLink();
  void sendDistantMessage();
  /** Create the context popup menu and it's submenus */
  void connectTreeWidgetCostumPopupMenu( QPoint point );
  //void unvalidGPGKeyWidgetCostumPopupMenu( QPoint point );

  /** Called when user clicks "Load Cert" to choose location of a Cert file */
//  void loadcert();

//  void authneighbour();
//  void addneighbour();

  void on_actionAddFriend_activated();
  //void on_actionCopyKey_activated();
  void on_actionExportKey_activated();

  void on_actionCreate_New_Profile_activated();
    
  //void updateNetworkStatus();
  
//  void loadtabsettings();
  
//  void on_actionTabsright_activated();
//  void on_actionTabsnorth_activated();
//  void on_actionTabssouth_activated();
//  void on_actionTabswest_activated();
//
//  void on_actionTabsRounded_activated();
//  void on_actionTabsTriangular_activated();
  
  void filterColumnChanged(int);
  void filterItems(const QString &text);

private:
  QTreeWidgetItem *getCurrentNeighbour();

//  class NetworkView *networkview;
  
  bool filterItem(QTreeWidgetItem *item, const QString &text, int filterColumn);

  /* Color definitions (for standard see qss.default) */
  QColor mBackgroundColorSelf;
  QColor mBackgroundColorOwnSign;
  QColor mBackgroundColorAcceptConnection;
  QColor mBackgroundColorHasSignedMe;
  QColor mBackgroundColorDenied;

  /** Qt Designer generated object */
  Ui::NetworkDialog ui;
};

#endif

