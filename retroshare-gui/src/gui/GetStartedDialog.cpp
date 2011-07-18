/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2011, drbob
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

#include "gui/GetStartedDialog.h"
#include "retroshare/rsconfig.h"
#include "gui/RsAutoUpdatePage.h"

#include <iostream>

/** Constructor */
GetStartedDialog::GetStartedDialog(QWidget *parent)
: MainPage(parent)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

	/* we use a flag to setup the GettingStarted Flags, so that RS has a bit of time to initialise itself
	 */

	mFirstShow = true;

	connect(ui.inviteCheckBox, SIGNAL(stateChanged( int )), this, SLOT(tickInviteChanged()));
	connect(ui.addCheckBox, SIGNAL(stateChanged( int )), this, SLOT(tickAddChanged()));
	connect(ui.connectCheckBox, SIGNAL(stateChanged( int )), this, SLOT(tickConnectChanged()));
	connect(ui.firewallCheckBox, SIGNAL(stateChanged( int )), this, SLOT(tickFirewallChanged()));

/* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

GetStartedDialog::~GetStartedDialog()
{

}

void GetStartedDialog::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui.retranslateUi(this);
        break;
    default:
        break;
    }

}

void GetStartedDialog::showEvent ( QShowEvent * event ) 
{
        /* do nothing if locked, or not visible */
        if (RsAutoUpdatePage::eventsLocked() == true)
        {
                std::cerr << "GetStartedDialog::showEvent() events Are Locked" << std::endl;
                return;
        }

	if ((mFirstShow) && (rsConfig))
	{
        	RsAutoUpdatePage::lockAllEvents();

		updateFromUserLevel();
		mFirstShow = false;

        	RsAutoUpdatePage::unlockAllEvents() ;
	}

}

void GetStartedDialog::updateFromUserLevel()
{
	uint32_t userLevel = RSCONFIG_USER_LEVEL_NEW;
	userLevel = rsConfig->getUserLevel();

	ui.inviteCheckBox->setChecked(false);
	ui.addCheckBox->setChecked(false);
	ui.connectCheckBox->setChecked(false);
	ui.firewallCheckBox->setChecked(false);

	switch(userLevel)
	{
		// FALLS THROUGH EVERYWHERE.
		case RSCONFIG_USER_LEVEL_POWER:
		case RSCONFIG_USER_LEVEL_OVERRIDE:
			ui.firewallCheckBox->setChecked(true);

		case RSCONFIG_USER_LEVEL_CASUAL:
			ui.connectCheckBox->setChecked(true);

		case RSCONFIG_USER_LEVEL_BASIC:
			ui.addCheckBox->setChecked(true);
			ui.inviteCheckBox->setChecked(true);

		default:
		case RSCONFIG_USER_LEVEL_NEW:

			break;
	}

	/* will this auto trigger changes? */

}

void GetStartedDialog::tickInviteChanged()
{
	if (ui.inviteCheckBox->isChecked())
	{
		ui.inviteTextBrowser->setVisible(false);
	}
	else
	{
		ui.inviteTextBrowser->setVisible(true);
	}
}

void GetStartedDialog::tickAddChanged()
{
	if (ui.addCheckBox->isChecked())
	{
		ui.addTextBrowser->setVisible(false);
	}
	else
	{
		ui.addTextBrowser->setVisible(true);
	}
}

void GetStartedDialog::tickConnectChanged()
{
	if (ui.connectCheckBox->isChecked())
	{
		ui.connectTextBrowser->setVisible(false);
	}
	else
	{
		ui.connectTextBrowser->setVisible(true);
	}
}

void GetStartedDialog::tickFirewallChanged()
{
	if (ui.firewallCheckBox->isChecked())
	{
		ui.firewallTextBrowser->setVisible(false);
	}
	else
	{
		ui.firewallTextBrowser->setVisible(true);
	}
}


