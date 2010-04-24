/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 -2009 RetroShare Team
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

#include "GeneralPage.h"
#include "DirectoriesPage.h"
#include "ServerPage.h"
#include "NetworkPage.h"
#include "NotifyPage.h"
#include "CryptoPage.h"
#include "AppearancePage.h"
#include "FileAssociationsPage.h"
#include "SoundPage.h"
#include "TransferPage.h"

#define IMAGE_GENERAL       ":/images/kcmsystem24.png"


#include "rsettingswin.h"

RSettingsWin *RSettingsWin::_instance = NULL;
int RSettingsWin::lastPage = 0;

RSettingsWin::RSettingsWin(QWidget * parent, Qt::WFlags flags)
                            : QDialog(parent, flags)
{
    setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setModal(false);

    initStackedWidget();

    connect(listWidget, SIGNAL(currentRowChanged(int)), this, SLOT(setNewPage(int)));
    connect(okButton, SIGNAL(clicked( bool )), this, SLOT( saveChanges()) );
}

RSettingsWin::~RSettingsWin()
{
    lastPage = stackedWidget->currentIndex ();
    _instance = NULL;
}

void
RSettingsWin::closeEvent (QCloseEvent * event)
{
    QWidget::closeEvent(event);
}

/*static*/ void RSettingsWin::showYourself(QWidget *parent)
{
    if(_instance == NULL) {
        _instance = new RSettingsWin(parent);
    }

    if (_instance->isHidden()) {
        _instance->setNewPage(lastPage);
    }
    _instance->show();
    _instance->activateWindow();
}

/*static*/ void RSettingsWin::postModDirectories(bool update_local)
{
    if (_instance == NULL || _instance->isHidden() || _instance->stackedWidget == NULL) {
       return;
    }

    if (update_local) {
        if (_instance->stackedWidget->currentIndex() == Directories) {
            ConfigPage *Page = (ConfigPage*) _instance->stackedWidget->currentWidget();
            if (Page) {
                Page->load();
            }
        }
    }
}

void
RSettingsWin::initStackedWidget()
{
    stackedWidget->setCurrentIndex(-1);
    stackedWidget->removeWidget(stackedWidget->widget(0));

    stackedWidget->addWidget(new GeneralPage(false));
    stackedWidget->addWidget(new ServerPage());
    stackedWidget->addWidget(new TransferPage());
    stackedWidget->addWidget(new DirectoriesPage());
    stackedWidget->addWidget(new NotifyPage());
    stackedWidget->addWidget(new CryptoPage());
    stackedWidget->addWidget(new AppearancePage());
    #ifndef RS_RELEASE_VERSION
    stackedWidget->addWidget(new FileAssociationsPage() );
    stackedWidget->addWidget(new SoundPage() );
    #endif

    setNewPage(General);
}

void
RSettingsWin::setNewPage(int page)
{
    QString text;

    switch (page)
    {
        case General:
            text = tr("General");
	    pageicon->setPixmap(QPixmap(":/images/kcmsystem24.png"));
            break;
        case Directories:
            text = tr("Directories");
	    pageicon->setPixmap(QPixmap(":/images/folder_doments.png"));
            break;
        case Server:
            text = tr("Server");
	    pageicon->setPixmap(QPixmap(":/images/server_24x24.png"));
            break;
        case Transfer:
            text = tr("Transfer");
	    pageicon->setPixmap(QPixmap(":/images/ktorrent32.png"));
            break;    
        case Notify:
            text = tr("Notify");
	    pageicon->setPixmap(QPixmap(":/images/status_unknown.png"));
            break;
        case Security:
            text = tr("Security");
	    pageicon->setPixmap(QPixmap(":/images/encrypted32.png"));
            break;
        case Appearance:
            text = tr("Appearance");
	    pageicon->setPixmap(QPixmap(":/images/looknfeel.png"));
            break;
        #ifndef RS_RELEASE_VERSION
        case Fileassociations:
            text = tr("File Associations");
	    pageicon->setPixmap(QPixmap(":/images/filetype-association.png"));
            break;
        case Sound:
            text = tr("Sound");
	    pageicon->setPixmap(QPixmap(":/images/sound.png"));
            break;
        #endif
        default:
            text = tr("UnknownPage");// impossible case
    }

    pageName->setText(text);
    loadSettings();	/* load saved settings */
    stackedWidget->setCurrentIndex(page);
    listWidget->setCurrentRow(page);
}

void
RSettingsWin::loadSettings()
{
  /* Call each config page's load() method to load its data */
  int i, count = stackedWidget->count();
  for (i = 0; i < count; i++) {
	  ConfigPage *page = (ConfigPage *) stackedWidget->widget(i);
	  page->load();
  }
}

/** Saves changes made to settings. */
void
RSettingsWin::saveChanges()
{
	QString errmsg;

	/* Call each config page's save() method to save its data */
	int i, count = stackedWidget->count();
	for (i = 0; i < count; i++) 
	{
		ConfigPage *page = dynamic_cast<ConfigPage *>(stackedWidget->widget(i));

		if(!page->save(errmsg)) 
		{
			/* Display the offending page */
			stackedWidget->setCurrentWidget(page);

			/* Show the user what went wrong */
			QMessageBox::warning(this,
			tr("Error Saving Configuration on page ")+QString::number(i), errmsg,
			QMessageBox::Ok, QMessageBox::NoButton);

			/* Don't process the rest of the pages */
			return;
		}
	}

	/* call to RsIface save function.... */
	//rsicontrol -> ConfigSave();

	QDialog::close();
}

