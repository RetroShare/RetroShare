/*******************************************************************************
 * gui/settings/rsettingswin.cpp                                               *
 *                                                                             *
 * Copyright (c) 2008, Retroshare Team <retroshare.project@gmail.com>          *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include <QMessageBox>

#include <retroshare/rsplugin.h>
#include <rshare.h>
#include "GeneralPage.h"
#include "ServerPage.h"
#include "NotifyPage.h"
#include "CryptoPage.h"
#include "AppearancePage.h"
#include "FileAssociationsPage.h"
#include "SoundPage.h"
#include "TransferPage.h"
#include "ChatPage.h"
#include "ChannelPage.h"
#include "PeoplePage.h"
#include "MessagePage.h"
#include "ForumPage.h"
#include "AboutPage.h"
#include "PostedPage.h"
#include "PluginsPage.h"
#include "ServicePermissionsPage.h"
#include "rsharesettings.h"
#include "gui/notifyqt.h"
#include "gui/common/FloatingHelpBrowser.h"
#include "gui/common/RSElidedItemDelegate.h"

#ifdef RS_WEBUI
#	include "WebuiPage.h"
#endif

#ifdef RS_JSONAPI
#	include "JsonApiPage.h"
#endif

#define IMAGE_GENERAL       ":/images/kcmsystem24.png"

#define ITEM_SPACING 2

#include "rsettingswin.h"

//RSettingsWin *RSettingsWin::_instance = NULL;
int SettingsPage::lastPage = 0;

SettingsPage::SettingsPage(QWidget *parent)
    : MainPage(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint)
{
    ui.setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose, true);

    /* Initialize help browser */
    mHelpBrowser = new FloatingHelpBrowser(this, ui.helpButton);

    /* set initial size the splitter */
    ui.splitter->setStretchFactor(0, 0);
    ui.splitter->setStretchFactor(1, 1);

    QList<int> sizes;
    sizes << 200 << width(); // Qt calculates the right sizes
    ui.splitter->setSizes(sizes);

    /* Add own item delegate to get item width*/
    RSElidedItemDelegate *itemDelegate = new RSElidedItemDelegate(this);
    itemDelegate->setSpacing(QSize(0, ITEM_SPACING));
    ui.listWidget->setItemDelegate(itemDelegate);

    initStackedWidget();

    /* Load window position */
    QByteArray geometry = Settings->valueFromGroup("SettingDialog", "Geometry", QByteArray()).toByteArray();
    if (geometry.isEmpty() == false) {
      restoreGeometry(geometry);
    }
    // state of splitter
    ui.splitter->restoreState(Settings->valueFromGroup("SettingDialog", "Splitter", QByteArray()).toByteArray());

    connect(ui.listWidget, SIGNAL(currentRowChanged(int)), this, SLOT(setNewPage(int)));
    connect(this, SIGNAL(finished(int)), this, SLOT(dialogFinished(int)));
}

SettingsPage::~SettingsPage()
{
    /* Save window position */
    Settings->setValueToGroup("SettingDialog", "Geometry", saveGeometry());
    // state of splitter
    Settings->setValueToGroup("SettingDialog", "Splitter", ui.splitter->saveState());
    //lastPage = ui.stackedWidget->currentIndex ();
    //_instance = NULL;
}

//void RSettingsPage::dialogFinished(int result)
//{
//	if (result == Rejected) {
//		/* reaload style sheet */
//		Rshare::loadStyleSheet(::Settings->getSheetName());
//	}
//}

/*static*/ void SettingsPage::showYourself(QWidget */*parent*/, PageType /*page = LastPage*/)
{
#ifdef TODO
    if(_instance == NULL) {
        _instance = new RSettingsPage(parent);
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
#else
    std::cerr << "(EE) unimplemented call to RSettingsPage::showYourself" << std::endl;
#endif
}

/*static*/ void SettingsPage::postModDirectories(bool update_local)
{
    //if (_instance == NULL || _instance->isHidden() || _instance->ui.stackedWidget == NULL) {
    if (ui.stackedWidget == NULL) {
       return;
    }

    if (update_local) {
        if (ui.stackedWidget->currentIndex() == Directories) {
            ConfigPage *Page = dynamic_cast<ConfigPage *> (ui.stackedWidget->currentWidget());
            if (Page) {
                Page->load();
            }
        }
    }
}

void
SettingsPage::initStackedWidget()
{
    ui.stackedWidget->setCurrentIndex(-1);
    ui.stackedWidget->removeWidget(ui.stackedWidget->widget(0));

    addPage(new GeneralPage()); // GENERAL
    addPage(new CryptoPage()); // NODE
    addPage(new ServerPage()); // NETWORK
    addPage(new PeoplePage()); // PEOLPE
    addPage(new ChatPage()); // CHAT
    addPage(new MessagePage()); //MESSGE RENAME TO MAIL
    addPage(new TransferPage()); //FILE TRANSFER
    addPage(new ChannelPage()); // CHANNELS
    addPage(new ForumPage()); // FORUMS
    addPage(new PostedPage()); // POSTED RENAME TO LINKS
    addPage(new NotifyPage()); // NOTIFY
    addPage(new settings::PluginsPage() ); // PLUGINS
    addPage(new AppearancePage()); // APPEARENCE
    addPage(new SoundPage() ); // SOUND
    addPage(new ServicePermissionsPage() ); // PERMISSIONS
#ifdef RS_JSONAPI
    JsonApiPage *jsonapi_p = new JsonApiPage() ;
	addPage(new JsonApiPage());
#ifdef RS_WEBUI
    WebuiPage *webui_p = new WebuiPage() ;
    addPage(new WebuiPage() );

    QObject::connect(webui_p,SIGNAL(passwordChanged()),jsonapi_p,SLOT(load()));
#endif
#endif

	 // add widgets from plugins
	for(int i=0;i<rsPlugins->nbPlugins();++i)
	{
		RsPlugin *pl = rsPlugins->plugin(i) ;
		if(pl != NULL)
		{
			ConfigPage* cp = pl->qt_config_page();
			if(cp != NULL)
				addPage(cp) ;
		}
	}
    addPage(new AboutPage() );

	 // make the first page the default.

    setNewPage(General);
}

void SettingsPage::addPage(ConfigPage *page)
{
	ui.stackedWidget->addWidget(page) ;

	QListWidgetItem *item = new QListWidgetItem(QIcon(page->iconPixmap()),page->pageName());
	ui.listWidget->addItem(item);
}

void
SettingsPage::setNewPage(int page)
{
	ConfigPage *pagew = dynamic_cast<ConfigPage*>(ui.stackedWidget->widget(page)) ;

	mHelpBrowser->hide();

	if(pagew == NULL)
	{
		std::cerr << "Error in RSettingsPage::setNewPage(): widget is not a ConfigPage!" << std::endl;
		mHelpBrowser->clear();
		return ;
	}
	ui.pageNameLabel->setText(pagew->pageName());
	ui.pageIconLabel->setPixmap(pagew->iconPixmap()) ;

	ui.stackedWidget->setCurrentIndex(page);
	ui.listWidget->setCurrentRow(page);

	mHelpBrowser->setHelpText(pagew->helpText());
}

/** Saves changes made to settings. */
void SettingsPage::notifySettingsChanged()
{
	/* call to RsIface save function.... */
	//rsicontrol -> ConfigSave();

	if (NotifyQt::getInstance())
		NotifyQt::getInstance()->notifySettingsChanged();
}
