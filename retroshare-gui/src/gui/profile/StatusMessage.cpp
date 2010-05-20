/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2009,  RetroShre Team
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
#include "StatusMessage.h"

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsdisc.h"
#include "rsiface/rsmsgs.h"
#include "gui/settings/rsharesettings.h"


#include <QTime>

#include <sstream>
#include <iomanip>


/** Default constructor */
StatusMessage::StatusMessage(QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);
  
  connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(close()));
  connect(ui.okButton, SIGNAL(clicked()), this, SLOT(save()));
  
  load();
  
}

/** Destructor. */
StatusMessage::~StatusMessage()
{
}

void StatusMessage::closeEvent (QCloseEvent * event)
{
 QDialog::closeEvent(event);
}


/** Saves the changes on this page */
void StatusMessage::save()
{
    Settings->setValueToGroup("Profile", "StatusMessage",ui.txt_StatusMessage->text());

	rsMsgs->setCustomStateString(ui.txt_StatusMessage->text().toStdString());

	close();
}


/** Loads the settings for this page */
void StatusMessage::load()
{	
  ui.txt_StatusMessage->setText(QString::fromStdString(rsMsgs->getCustomStateString()));
}



