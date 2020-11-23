/*******************************************************************************
 * gui/settings/PostedPage.cpp                                                 *
 *                                                                             *
 * Copyright 2014 Retroshare Team  <retroshare.project@gmail.com>              *
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
	ui->groupFrameSettingsWidget->setOpenAllInNewTabText(tr("Open each board in a new tab"));
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
