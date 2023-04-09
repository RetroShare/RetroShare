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
#include "../../libretroshare/src/retroshare/rsgxschannels.h"
#include "../../libretroshare/src/services/p3gxschannels.h"

ChannelPage::ChannelPage(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_QuitOnClose, false);

	/* Initialize GroupFrameSettingsWidget */
	ui.groupFrameSettingsWidget->setOpenAllInNewTabText(tr("Open each channel in a new tab"));
    ui.groupFrameSettingsWidget->setType(GroupFrameSettings::Channel) ;

    connect(ui.emoteicon_checkBox,SIGNAL(toggled(bool)),this,SLOT(updateEmotes())) ;

    // Connecting the spin box with the maximum auto download size in channels
    connect(ui.autoDownloadSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateMaxAutoDownloadSizeLimit(int)));

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

    // Getting  the maximum auto download size from the configuration
    uint64_t maxAutoDownloadSize;
    rsGxsChannels->getMaxAutoDownloadSizeLimit(maxAutoDownloadSize);
    int temp=(maxAutoDownloadSize/(Size_Of_1_GB));
    whileBlocking(ui.autoDownloadSpinBox)->setValue(temp);

}

void ChannelPage::updateEmotes()
{
    Settings->beginGroup(QString("ChannelPostsWidget"));
    Settings->setValue("Emoteicons_ChannelDecription", ui.emoteicon_checkBox->isChecked());
    Settings->endGroup();
}

// Function to update the maximum size allowed for auto download in channels
void ChannelPage::updateMaxAutoDownloadSizeLimit(int value)
{
    uint64_t temp=(static_cast<uint64_t>(value)*Size_Of_1_GB);
    rsGxsChannels->setMaxAutoDownloadSizeLimit(temp);
}

