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

#ifdef UNFINISHED
#include "unfinished/ApplicationWindow.h"
#endif

#include "NetworkDialog.h"
#include "PeersDialog.h"
#include "SearchDialog.h"
#include "TransfersDialog.h"
#include "MessagesDialog.h"
#include "SharedFilesDialog.h"
#include "MessengerWindow.h"
#include "PluginsPage.h"

#include "bwgraph/bwgraph.h"
#include "help/browser/helpbrowser.h"

#include "ui_MainWindow.h"
#include "gui/common/RWindow.h"

class PeerStatus;
class NATStatus;
class RatesStatus;

void openFile(std::string path);

class MainWindow : public RWindow
{
  Q_OBJECT

public:
    /** Main dialog pages. */
    enum Page {
        Network            = 0,  		/** Network page. */
        Friends,              	 		/** Peers page. */
        Search,					/** Search page. */
        Transfers, 				/** Transfers page. */
        SharedDirectories,   			/** Shared Directories page. */
        Messages,  		           	/** Messages page. */
        Links,  				/** Links page. */
        Channels,  				/** Channels page. */
        Forums  				/** Forums page. */


    };

    /** Default Constructor */
    MainWindow(QWidget *parent = 0, Qt::WFlags flags = 0);

    /** Destructor. */
    ~MainWindow();

    /* A Bit of a Hack... but public variables for
    * the dialogs, so we can add them to the
    * Notify Class...
    */

    NetworkDialog     *networkDialog;
    PeersDialog       *peersDialog;
    SearchDialog      *searchDialog;
    TransfersDialog   *transfersDialog;
    ChatDialog        *chatDialog;
    MessagesDialog    *messagesDialog;
    SharedFilesDialog *sharedfilesDialog;
    MessengerWindow   *messengerWindow;
#ifdef UNFINISHED    
    ApplicationWindow   *applicationWindow;
#endif
    PluginsPage*   pluginsPage ;


public slots:
    /** Called when this dialog is to be displayed */
    //void show();
    /** Shows the config dialog with focus set to the given page. */
    void showWindow(Page page);

    void playFiles(QStringList files);
    void updateHashingInfo(const QString&) ;
	
protected:
    void closeEvent(QCloseEvent *);
    
    /** Called when the user changes the UI translation. */
    virtual void retranslateUi();

private slots:

    void updateMenu();
    void updateStatus();

    void toggleVisibility(QSystemTrayIcon::ActivationReason e);
    void toggleVisibilitycontextmenu();


    /** Toolbar fns. */
    void addFriend();
    void showMessengerWindow();
#ifdef UNFINISHED    
    void showApplWindow();
#endif

    void showabout();
    void openShareManager();
    void displaySystrayMsg(const QString&,const QString&) ;

    /** Displays the help browser and displays the most recently viewed help
    * topic. */
    void showHelpDialog();
    /** Called when a child window requests the given help <b>topic</b>. */
    void showHelpDialog(const QString &topic);

    void showMess(MainWindow::Page page = MainWindow::Messages);
    void showSettings();
    void setStyle();

    /** Called when user attempts to quit via quit button*/
    void doQuit();


private:

    /** Create the actions on the tray menu or menubar */
    void createActions();
    
    void createTrayIcon();

    /** Defines the actions for the tray menu */
    QAction* _settingsAct;
    QAction* _bandwidthAct;
    QAction* _messengerwindowAct;
    QAction* _messagesAct;
    QAction* _smplayerAct;
    QAction* _helpAct;
#ifdef UNFINISHED   
    QAction* _appAct;
#endif    

    /** A BandwidthGraph object which handles monitoring RetroShare bandwidth usage */
    BandwidthGraph* _bandwidthGraph;

    /** A RetroShareSettings object used for saving/loading settings */
    RshareSettings* _settings;

    /** Creates a new action for a Main page. */
    QAction* createPageAction(QIcon img, QString text, QActionGroup *group);
    /** Adds a new action to the toolbar. */
    void addAction(QAction *action, const char *slot = 0);

    void loadStyleSheet(const QString &sheetName);

    QSystemTrayIcon *trayIcon;
    QAction *toggleVisibilityAction, *toolAct;
    QMenu *menu;

    PeerStatus *peerstatus;
    NATStatus *natstatus;
    RatesStatus *ratesstatus;

    QLabel *_hashing_info_label ;

    /** Qt Designer generated object */
    Ui::MainWindow ui;
};

#endif

