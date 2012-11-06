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

#include <retroshare/rsmsgs.h>

/** Default constructor */
StatusMessage::StatusMessage(QWidget *parent)
  : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);
  
  connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(save()));
  connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(close()));
  
  ui.txt_StatusMessage->setText(QString::fromUtf8(rsMsgs->getCustomStateString().c_str()));
}

/** Saves the changes on this page */
void StatusMessage::save()
{
    rsMsgs->setCustomStateString(ui.txt_StatusMessage->text().toUtf8().constData());

    accept();
}
