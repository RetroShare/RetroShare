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


#include <QTime>

#include <sstream>
#include <iomanip>


/** Default constructor */
StatusMessage::StatusMessage(QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);
  
  /* Create RshareSettings object */
  _settings = new RshareSettings();
  
  connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(close()));
  connect(ui.okButton, SIGNAL(clicked()), this, SLOT(save()));
  
  load();
  
}

/** Destructor. */
StatusMessage::~StatusMessage()
{
  delete _settings;

}

void StatusMessage::closeEvent (QCloseEvent * event)
{
 QDialog::closeEvent(event);
}


/** Saves the changes on this page */
void StatusMessage::save()
{
  _settings->beginGroup("Profile");

			_settings->setValue("StatusMessage",ui.txt_StatusMessage->text());

	_settings->endGroup();
	
	rsMsgs->setCustomStateString(ui.txt_StatusMessage->text().toStdString());

	close();
}


/** Loads the settings for this page */
void StatusMessage::load()
{	
  ui.txt_StatusMessage->setText(QString::fromStdString(rsMsgs->getCustomStateString()));
}



