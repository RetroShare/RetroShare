/*******************************************************************************
 * retroshare-gui/src/gui/profile/StatusMessage.cpp                            *
 *                                                                             *
 * Copyright (C) 2009 by Retroshare Team     <retroshare.project@gmail.com>    *
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
