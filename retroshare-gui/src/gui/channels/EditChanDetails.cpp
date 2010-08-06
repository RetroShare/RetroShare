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

#include <list>
#include <iostream>
#include <string>


/** Default constructor */
EditChanDetails::EditChanDetails(QWidget *parent, Qt::WFlags flags, std::string cId)
  : QDialog(parent, flags), mChannelId(cId)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);

  connect(ui.applyButton, SIGNAL(clicked()), this, SLOT(applyDialog()));
  connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(closeinfodlg()));

  ui.ChannelLogoButton->setDisabled(true);
  ui.ChannelLogoButton->hide();
  ui.LogoButton->setDisabled(true);
  ui.LogoButton->hide();

  loadChannel();

}


/**
 Overloads the default show() slot so we can set opacity*/

void EditChanDetails::show()
{

  if(!this->isVisible()) {
    QDialog::show();

  }
}

void EditChanDetails::closeEvent (QCloseEvent * event)
{
 QWidget::closeEvent(event);
}

void EditChanDetails::closeinfodlg()
{
	close();
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

}

void EditChanDetails::applyDialog()
{

    if(!rsChannels)
        return;

    // if text boxes have not been edited leave alone
    if(!ui.nameline->isModified() && !ui.DescriptiontextEdit->document()->isModified())
        return;

    ChannelInfo ci;

    ci.channelName = ui.nameline->text().toStdWString();
    ci.channelDesc = ui.DescriptiontextEdit->document()->toPlainText().toStdWString();


    /* reload now */
    rsChannels->channelEditInfo(mChannelId, ci);

    /* close the Dialog after the Changes applied */
    closeinfodlg();

    return;
}



