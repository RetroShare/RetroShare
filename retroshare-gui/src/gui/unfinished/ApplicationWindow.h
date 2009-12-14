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

#ifndef _ApplicationWindow_H
#define _ApplicationWindow_H

#include <QtGui>
#include <QMainWindow>
#include <QFileDialog>

#include "ExampleDialog.h"


#include "ui_ApplicationWindow.h"


class ApplicationWindow : public QMainWindow
{
  Q_OBJECT

public:
    /** Main dialog pages. */
    enum Page {
        Graph              = 0,  /** Network Graph */
        Channels           = 1,  /** Channels page. */
        SharedDirectories  = 2,  /** Shared Directories page. */
        Search 	           = 3,  /** Search page. */
        Transfers          = 4,  /** Transfers page. */
        Chat               = 5,  /** Chat page. */
        Messages           = 6,  /** Messages page. */
        Statistics         = 8  /** Statistic page. */
        
    };

    /** Default Constructor */
    ApplicationWindow(QWidget *parent = 0, Qt::WFlags flags = 0);
    
    /** Destructor. */
    ~ApplicationWindow();

    /* A Bit of a Hack... but public variables for 
    * the dialogs, so we can add them to the 
    * Notify Class...
    */
    
    ExampleDialog    *exampleDialog;
    //ChannelsDialog    *channelsDialog;
    //GroupsDialog      *groupsDialog;
    //StatisticDialog   *statisticDialog;

public slots:
    /** Called when this dialog is to be displayed */
    void show();
    /** Shows the config dialog with focus set to the given page. */
    void show(Page page);
  

private slots:

    void updateMenu();
    
    void toggleVisibility(QSystemTrayIcon::ActivationReason e);
    void toggleVisibilitycontextmenu();

    void showsmplayer();

protected:
    void closeEvent(QCloseEvent *);


private slots:



private:

    /** Create the actions on the tray menu or menubar */
    void createActions();
    
    /** Creates a new action for a config page. */
    QAction* createPageAction(QIcon img, QString text, QActionGroup *group);
    /** Adds a new action to the toolbar. */
    void addAction(QAction *action, const char *slot = 0);
    
    void loadStyleSheet(const QString &sheetName);
    
    QSystemTrayIcon *trayIcon;
    QAction *toggleVisibilityAction;
    QMenu *menu;
       
    /** Qt Designer generated object */
    Ui::ApplicationWindow ui;
};

#endif

