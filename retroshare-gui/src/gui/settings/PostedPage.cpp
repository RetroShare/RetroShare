/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2014, RetroShare Team
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

#include "PostedPage.h"
#include "ui_PostedPage.h"
#include "rsharesettings.h"

PostedPage::PostedPage(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags),
      ui(new Ui::PostedPage)
{
	ui->setupUi(this);
	setAttribute(Qt::WA_QuitOnClose, false);

	/* Initialize GroupFrameSettingsWidget */
	ui->groupFrameSettingsWidget->setOpenAllInNewTabText(tr("Open each topic in a new tab"));
	ui->groupFrameSettingsWidget->setType(GroupFrameSettings::Posted);
}

PostedPage::~PostedPage()
{
}

/** Loads the settings for this page */
void PostedPage::load()
{
	ui->groupFrameSettingsWidget->loadSettings(GroupFrameSettings::Posted);
}
