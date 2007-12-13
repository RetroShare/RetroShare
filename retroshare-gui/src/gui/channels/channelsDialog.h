/****************************************************************
*  RetroShare is distributed under the following license:
*
*  Copyright (C) 2006, 2007 The RetroShare Team
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
#ifndef _ChannelsDialog_h_
#define _ChannelsDialog_h_

#include "../mainpage.h"
#include "ui_ChannelsDialog.h"

#include <QFileDialog>

class QTreeWidgetItem;

class ChannelsDialog : public MainPage, public Ui::ChannelsDialog 
{
    Q_OBJECT
        
public:
    ChannelsDialog(QWidget * parent = 0 );


    void insertChannels();
    void insertMsgTxtAndFiles();


private slots:

    /** Create the context popup menu and it's submenus */
    void messageslistWidgetCostumPopupMenu( QPoint point ); 
    void msgfilelistWidgetCostumPopupMenu(QPoint);  
  
    void updateChannels ( QTreeWidgetItem * item, int column );
  
          // Fns for MsgList
    void newmessage();
    void newchannel();
    void subscribechannel();
    void unsubscribechannel();
    void deletechannel();
  
          // Fns for recommend List
    void getcurrentrecommended();
    void getallrecommended();

private:
  
    /** Define the popup menus for the Context menu */
    QMenu* contextMnu;
    
    /** Defines the actions for the context menu */
    QAction* newMsgAct;
    QAction* newChanAct;
    QAction* subChanAct;
    QAction* unsubChanAct;
    QAction* delChanAct;
  
    QAction* getRecAct;
    QAction* getAllRecAct;
};


#endif
