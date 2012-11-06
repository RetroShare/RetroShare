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

#include <QMessageBox>

#include <retroshare/rsplugin.h>
#include <rshare.h>
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
#include "RelayPage.h"
#include "ChatPage.h"
#include "MessagePage.h"
#include "ForumPage.h"
#include "PluginsPage.h"
#include "rsharesettings.h"

#define IMAGE_GENERAL       ":/images/kcmsystem24.png"

#include "rsettingswin.h"

RSettingsWin *RSettingsWin::_instance = NULL;
int RSettingsWin::lastPage = 0;

RSettingsWin::RSettingsWin(QWidget *parent)
    : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
    setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setModal(false);

    initStackedWidget();

    connect(listWidget, SIGNAL(currentRowChanged(int)), this, SLOT(setNewPage(int)));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(saveChanges()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(close()));
    connect(this, SIGNAL(finished(int)), this, SLOT(dialogFinished(int)));
}

RSettingsWin::~RSettingsWin()
{
    lastPage = stackedWidget->currentIndex ();
    _instance = NULL;
}

void RSettingsWin::dialogFinished(int result)
{
	if (result == Rejected) {
		/* reaload style sheet */
		Rshare::loadStyleSheet(::Settings->getSheetName());
	}
}

/*static*/ void RSettingsWin::showYourself(QWidget *parent, PageType page /* = LastPage*/)
{
    if(_instance == NULL) {
        _instance = new RSettingsWin(parent);
    }

    if (page != LastPage) {
        /* show given page */
        _instance->setNewPage(page);
    } else {
        if (_instance->isHidden()) {
            _instance->setNewPage(lastPage);
        }
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
            ConfigPage *Page = dynamic_cast<ConfigPage *> (_instance->stackedWidget->currentWidget());
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

    addPage(new GeneralPage(0));
    addPage(new ServerPage());
    addPage(new TransferPage());
    addPage(new RelayPage() );
    addPage(new DirectoriesPage());
    addPage(new PluginsPage() );
    addPage(new NotifyPage());
    addPage(new CryptoPage());
    addPage(new MessagePage());
    addPage(new ForumPage());
    addPage(new ChatPage());
    addPage(new AppearancePage());
    addPage(new SoundPage() );

	 // add widgets from plugins

	 for(int i=0;i<rsPlugins->nbPlugins();++i)
	 {
		 RsPlugin *pl = rsPlugins->plugin(i) ;

		 if(pl != NULL && pl->qt_config_page() != NULL)
			 addPage(pl->qt_config_page()) ;
	 }

	 // make the first page the default.

    setNewPage(General);
}

void RSettingsWin::addPage(ConfigPage *page)
{
	stackedWidget->addWidget(page) ;

	QListWidgetItem *item = new QListWidgetItem(QIcon(page->iconPixmap()),page->pageName()) ;
	listWidget->addItem(item) ;
}

void
RSettingsWin::setNewPage(int page)
{
	ConfigPage *pagew = dynamic_cast<ConfigPage*>(stackedWidget->widget(page)) ;

	if(pagew == NULL)
	{
		std::cerr << "Error in RSettingsWin::setNewPage(): widget is not a ConfigPage!" << std::endl;
		return ;
	}
	pageName->setText(pagew->pageName());
	pageicon->setPixmap(pagew->iconPixmap()) ;

	stackedWidget->setCurrentIndex(page);
	listWidget->setCurrentRow(page);
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
		if (page && page->wasLoaded()) {
			if (!page->save(errmsg))
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
	}

	/* call to RsIface save function.... */
	//rsicontrol -> ConfigSave();

    close();
}
