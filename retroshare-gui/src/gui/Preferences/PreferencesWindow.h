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

#ifndef _PreferencesWindow_H
#define _PreferencesWindow_H

#include <QMainWindow>
#include <QFileDialog>

#include "GeneralDialog.h"
#include "DirectoriesDialog.h"
#include "ServerDialog.h"
#include "CryptographyDialog.h"
#include "AppearanceDialog.h"
#include "gui/help/browser/helpbrowser.h"
#include <gui/common/rwindow.h>


#include "ui_PreferencesWindow.h"

class PreferencesWindow : public RWindow
{
  Q_OBJECT

public:
  /** Preferences dialog pages. */
  enum Page {
    General 	  	= 0,  /** Preferences page. */
    Server,  			  /** Server page. */
    Directories,           /** Directories page. */
    Appearance				/** Appearance page. */

  };

  /** Default Constructor */
  PreferencesWindow(QWidget *parent = 0, Qt::WFlags flags = 0);
    /** Default destructor */
  //~PreferencesWindow();

protected:
	void closeEvent (QCloseEvent * event);

public slots:
	/** Called when this dialog is to be displayed */
	//void show();
	/** Shows the Preferences dialog with focus set to the given page. */
	void showWindow(Page page);

private slots:

    /** Called when user clicks "Save Settings" */
    void saveChanges();
	/**void preferences();*/
  
	void cancelpreferences();
  
	/** Called when a ConfigPage in the dialog requests help on a specific
	* <b>topic</b>. */
	//void help(const QString &topic);
	/** Shows general help information for whichever settings page the user is
	* currently viewing. */
	//void help();
  	
  	/** Displays the help browser and displays the most recently viewed help
    * topic. */
    void showHelp();
    /** Called when a child window requests the given help <b>topic</b>. */
    void showHelp(const QString &topic);

private:
	/** Loads the current configuration settings */
	void loadSettings();
	/** Creates a new action for a config page. */
	QAction* createPageAction(QIcon img, QString text, QActionGroup *group);
	/** Adds a new action to the toolbar. */
	void addAction(QAction *action, const char *slot = 0);

  /** Qt Designer generated object */
  Ui::PreferencesWindow ui;
};

#endif

