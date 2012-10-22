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

#include <QMainWindow>

//#include "ExampleDialog.h"
#include "ui_ApplicationWindow.h"

class ApplicationWindow : public QMainWindow
{
  Q_OBJECT

public:
    /** Default Constructor */
    ApplicationWindow(QWidget *parent = 0, Qt::WFlags flags = 0);
    
    /** Destructor. */
    ~ApplicationWindow();

    /* A Bit of a Hack... but public variables for 
    * the dialogs, so we can add them to the 
    * Notify Class...
    */

    //ExampleDialog    *exampleDialog;
    //ChannelsDialog    *channelsDialog;
    //GroupsDialog      *groupsDialog;
    //StatisticDialog   *statisticDialog;

protected:
    void closeEvent(QCloseEvent *);

private:
    /** Creates a new action for a config page. */
    QAction* createPageAction(QIcon img, QString text, QActionGroup *group);
    /** Adds a new action to the toolbar. */
    void addAction(QAction *action, const char *slot = 0);

    /** Qt Designer generated object */
    Ui::ApplicationWindow ui;
};

#endif
