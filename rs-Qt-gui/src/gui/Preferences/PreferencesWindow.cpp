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



#include <QMessageBox>
#include <rshare.h>
#include "PreferencesWindow.h"

#include "rsiface/rsiface.h"


#define FONT        QFont(tr("Arial"), 8)

/* Images for toolbar icons */
#define IMAGE_PREFERENCES       ":/images/locale.png"
#define IMAGE_WEBCACHES        	":/images/webcache_24x24.png"
#define IMAGE_DIRECTORIES    	":/images/folder_doments.png"
#define IMAGE_CRYPTOGRAPHY      ":/images/cryptography_24x24.png"
#define IMAGE_LOG   			":/images/log_24x24.png"
#define IMAGE_ABOUT 			":/images/informations_24x24.png"
#define IMAGE_SAVE			    ":/images/media-floppy.png"


/** Constructor */
PreferencesWindow::PreferencesWindow(QWidget *parent, Qt::WFlags flags)
: QMainWindow(parent, flags)
{
  /* Invoke the Qt Designer generated QObject setup routine */
  ui.setupUi(this);

  /* Create the config pages and actions */
  QActionGroup *grp = new QActionGroup(this);
  ui.stackPages->add(new GeneralDialog(ui.stackPages),
                     createPageAction(QIcon(IMAGE_PREFERENCES), tr("General"), grp));
                     
  ui.stackPages->add(new ServerDialog(ui.stackPages),
                    createPageAction(QIcon(IMAGE_WEBCACHES), tr("Server"), grp));
  
  ui.stackPages->add(new DirectoriesDialog(ui.stackPages),
                     createPageAction(QIcon(IMAGE_DIRECTORIES), tr("Directories"), grp)); 
                     
  //ui.stackPages->add(new CryptographyDialog(ui.stackPages),
  //                   createPageAction(QIcon(IMAGE_CRYPTOGRAPHY), tr("Cryptography"), grp));
                     
  ui.stackPages->add(new LogDialog(ui.stackPages),
                     createPageAction(QIcon(IMAGE_ABOUT), tr("About"), grp));
                     
  
  /* Create the toolbar */
  ui.toolBar->addActions(grp->actions());
  ui.toolBar->addSeparator();
  connect(grp, SIGNAL(triggered(QAction *)), ui.stackPages, SLOT(showPage(QAction *)));
 
   /* Create and bind the Save button */  
  //addAction(new QAction(QIcon(IMAGE_SAVE), tr("Save"), ui.toolBar), 
  //          SLOT(saveChanges()));

  /* Select the first action */
  grp->actions()[0]->setChecked(true);
  setFixedSize(QSize(480, 450));
  
   connect(ui.okButton, SIGNAL(clicked( bool )), this, SLOT( saveChanges()) );
   connect(ui.cancelprefButton, SIGNAL(clicked( bool )), this, SLOT( cancelpreferences()) );

}

/** Creates a new action associated with a config page. */
QAction*
PreferencesWindow::createPageAction(QIcon img, QString text, QActionGroup *group)
{
  QAction *action = new QAction(img, text, group);
  action->setCheckable(true);
  action->setFont(FONT);
  return action;
}

/** Adds the given action to the toolbar and hooks its triggered() signal to
 * the specified slot (if given). */
void
PreferencesWindow::addAction(QAction *action, const char *slot)
{
  action->setFont(FONT);
  ui.toolBar->addAction(action);
  connect(action, SIGNAL(triggered()), this, slot);
}

/** Overloads the default show so we can load settings */
void
PreferencesWindow::show()
{
  /* Load saved settings */
  loadSettings();

  if (!this->isVisible()) {
    QMainWindow::show();
  } else {
    QMainWindow::activateWindow();
    setWindowState(windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
    QMainWindow::raise();
  }
}

/** Shows the config dialog with focus set to the given page. */
void
PreferencesWindow::show(Page page)
{
  /* Show the dialog. */
  show();

  /* Set the focus to the specified page. */
  ui.stackPages->setCurrentIndex((int)page);
}


/** Loads the saved PreferencesWindow settings. */
void
PreferencesWindow::loadSettings()
{
  /* Call each config page's load() method to load its data */
  foreach (ConfigPage *page, ui.stackPages->pages()) {
    page->load();
  }
}


/** Saves changes made to settings. */
void
PreferencesWindow::saveChanges()
{
  QString errmsg;
  
  /* Call each config page's save() method to save its data */
  foreach (ConfigPage *page, ui.stackPages->pages()) {
    if (!page->save(errmsg)) {
      /* Display the offending page */
      ui.stackPages->setCurrentPage(page);
      
      /* Show the user what went wrong */
      QMessageBox::warning(this, 
        tr("Error Saving Configuration"), errmsg,
        QMessageBox::Ok, QMessageBox::NoButton);

      /* Don't process the rest of the pages */
      return;
    }
  }

  /* call to RsIface save function.... */
  rsicontrol -> ConfigSave();

  QMainWindow::close();
}

/** Cancel and close the Preferences Window. */
void
PreferencesWindow::cancelpreferences()
{

  QMainWindow::close();
}

void PreferencesWindow::closeEvent (QCloseEvent * event)
{
    hide();
    event->ignore();
}





