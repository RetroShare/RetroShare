/*******************************************************************************
 * gui/settings/ChannelPage.cpp                                                *
 *                                                                             *
 * Copyright 2006, Crypton         <retroshare.project@gmail.com>              *
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

#include "ChannelPage.h"
#include "rsharesettings.h"
#include "util/misc.h"
#include "gui/notifyqt.h"

ChannelPage::ChannelPage(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_QuitOnClose, false);

	/* Initialize GroupFrameSettingsWidget */
	ui.groupFrameSettingsWidget->setOpenAllInNewTabText(tr("Open each channel in a new tab"));
    ui.groupFrameSettingsWidget->setType(GroupFrameSettings::Channel) ;

	connect(ui.emoteicon_checkBox,SIGNAL(toggled(bool)),this,SLOT(updateEmotes())) ;

}

ChannelPage::~ChannelPage()
{
}

/** Loads the settings for this page */
void ChannelPage::load()
{
	ui.groupFrameSettingsWidget->loadSettings(GroupFrameSettings::Channel);
	
	Settings->beginGroup(QString("ChannelPostsWidget"));
    whileBlocking(ui.emoteicon_checkBox)->setChecked(Settings->value("Emoteicons_ChannelDecription", true).toBool());
    Settings->endGroup();
}

void ChannelPage::updateEmotes()
{
    Settings->beginGroup(QString("ChannelPostsWidget"));
    Settings->setValue("Emoteicons_ChannelDecription", ui.emoteicon_checkBox->isChecked());
    Settings->endGroup();
}
