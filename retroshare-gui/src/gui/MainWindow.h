/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,2007 The RetroShare Team
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

#ifndef _MainWindow_H
#define _MainWindow_H

#include <QtGui>
#include <QMainWindow>
#include <QFileDialog>
#include <QSystemTrayIcon>


#include "NetworkDialog.h"
#include "PeersDialog.h"
#include "SearchDialog.h"
#include "TransfersDialog.h"
#include "MessagesDialog.h"
#include "ChatDialog.h"
#include "SharedFilesDialog.h"
#include "StatisticDialog.h"
#include "MessengerWindow.h"
#include "ApplicationWindow.h"

#include "Preferences/PreferencesWindow.h"
#include "Settings/gsettingswin.h"
#include "bwgraph/bwgraph.h"
#include "games/qbackgammon/bgwindow.h"
#include "toplevel.h"

#include "Preferences/ConfirmQuitDialog.h"

#include "channels/channelsDialog.h"


#include "ui_MainWindow.h"

#include "../config/rsharesettings.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
    /** Main dialog pages. */
    enum Page {
        Network            = 0,  /** Network page. */
        Friends            = 1,  /** Peers page. */
        SharedDirectories  = 2,  /** Shared Directories page. */
        Search 		       = 3,  /** Search page. */
        Transfers          = 4,  /** Transfers page. */
        Chat               = 5,  /** Chat page. */
        Messages           = 6,  /** Messages page. */
        Channels           = 7,  /** Channels page. */
        Statistics         = 8  /** Statistic page. */
        
    };

    /** Default Constructor */
    MainWindow(QWidget *parent = 0, Qt::WFlags flags = 0);
    
    /** Destructor. */
    ~MainWindow();

    /* A Bit of a Hack... but public variables for 
    * the dialogs, so we can add them to the 
    * Notify Class...
    */
    
    NetworkDialog *networkDialog;
    PeersDialog       *peersDialog;
    SearchDialog      *searchDialog;
    TransfersDialog   *transfersDialog;
    ChatDialog        *chatDialog;
    MessagesDialog    *messagesDialog;
    ChannelsDialog   *channelsDialog;
    SharedFilesDialog *sharedfilesDialog;
    //GroupsDialog      *groupsDialog;
    //StatisticDialog   *statisticDialog;
    MessengerWindow   *messengerWindow;
    ApplicationWindow   *applicationWindow;


public slots:
    /** Called when this dialog is to be displayed */
    void show();
    /** Shows the config dialog with focus set to the given page. */
    void show(Page page);
  
    void startgammon();
    void startqcheckers();


private slots:

    void updateMenu();
    
    void toggleVisibility(QSystemTrayIcon::ActivationReason e);
    void toggleVisibilitycontextmenu();


    /** Toolbar fns. */
    void addFriend();
    void inviteFriend();
    void addSharedDirectory();
    void showPreferencesWindow();
    void showMessengerWindow();
    void showApplWindow();
	void showsmplayer();

    void showSettings();
    void setStyle();
    
    /** Called when console button is toggled */
    void showConsoleFrame(bool show);
	
    /** Called when console button is toggled */
    //void showToolboxFrame(bool show);
    
    /** Called when user attempts to quit via quit button*/
    void doQuit();


protected:
    void closeEvent(QCloseEvent *);


private slots:



private:

    /** Create the actions on the tray menu or menubar */
    void createActions();
    
    
    /** Defines the actions for the tray menu */
    QAction* _prefsAct;
    QAction* _bandwidthAct;
    QAction* _messengerwindowAct;
    QAction* _smplayerAct;
    
    /** A BandwidthGraph object which handles monitoring RetroShare bandwidth usage */
    BandwidthGraph* _bandwidthGraph;
    
    PreferencesWindow* _preferencesWindow;
    
    
    /** Creates a new action for a Main page. */
    QAction* createPageAction(QIcon img, QString text, QActionGroup *group);
    /** Adds a new action to the toolbar. */
    void addAction(QAction *action, const char *slot = 0);
    
    /** Creates a new action for a Service pages. */
    QAction* createPageActionservice(QIcon img, QString text, QActionGroup *groupservice);
    /** Adds a new action to the toolbar. */
    void addActionservice(QAction *actionservice, const char *slot = 0);
    
    void loadStyleSheet(const QString &sheetName);
    
    QSystemTrayIcon *trayIcon;
    QAction *toggleVisibilityAction;
    QMenu *menu;
       
    /** Qt Designer generated object */
    Ui::MainWindow ui;
};

#endif

