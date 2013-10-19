/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2010 Christopher Evi-Parker
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
#include "EditChanDetails.h"

#include <retroshare/rschannels.h>

#include "util/misc.h"

#include <list>
#include <iostream>
#include <string>

#define CHAN_DEFAULT_IMAGE ":/images/channels.png"


/** Default constructor */
EditChanDetails::EditChanDetails(QWidget *parent, std::string cId)
  : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint), mChannelId(cId)
{
    /* Invoke Qt Designer generated QObject setup routine */
    ui.setupUi(this);

    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(applyDialog()));
    connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(close()));
  
    connect( ui.logoButton, SIGNAL(clicked() ), this , SLOT(addChannelLogo()));
    
    ui.logoButton->setDisabled(true);
    ui.logoLabel->setDisabled(true);

    loadChannel();
}

void EditChanDetails::loadChannel()
{
    if (!rsChannels)
    {
        return;
    }

    ChannelInfo ci;
    rsChannels->getChannelInfo(mChannelId, ci);

    // Set Channel Name
    ui.nameline->setText(QString::fromStdWString(ci.channelName));

    // Set Channel Description
    ui.DescriptiontextEdit->setText(QString::fromStdWString(ci.channelDesc));
    
	// Set Channel Logo
	QPixmap chanImage;
    if(ci.pngImageLen != 0)
    {
        chanImage.loadFromData(ci.pngChanImage, ci.pngImageLen, "PNG");
    }
    else
    {
        chanImage = QPixmap(CHAN_DEFAULT_IMAGE);
    }
    ui.logoLabel->setPixmap(chanImage);

}

void EditChanDetails::applyDialog()
{
    if(!rsChannels)
        return;

    // if text boxes have not been edited leave alone
    if(!ui.nameline->isModified() && !ui.DescriptiontextEdit->document()->isModified())
        return;

    ChannelInfo ci;

    ci.channelName = misc::removeNewLine(ui.nameline->text()).toStdWString();
    ci.channelDesc = ui.DescriptiontextEdit->document()->toPlainText().toStdWString();

    /* reload now */
    rsChannels->channelEditInfo(mChannelId, ci);

    /* close the Dialog after the Changes applied */
    close();

    return;
}

void EditChanDetails::addChannelLogo() // the same function as in CreateChannel
{
	QPixmap img = misc::getOpenThumbnailedPicture(this, tr("Load channel logo"), 64, 64);

	if (img.isNull())
		return;

	picture = img;

	// to show the selected
	ui.logoLabel->setPixmap(picture);
}
