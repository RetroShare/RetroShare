/*******************************************************************************
 * retroshare-gui/src/gui/profile/StatusMessage.h                              *
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

#ifndef _STATUSMESSAGE_H
#define _STATUSMESSAGE_H

#include <QDialog>

#include "ui_StatusMessage.h"

class StatusMessage : public QDialog
{
  Q_OBJECT

public:
  /** Default constructor */
  StatusMessage(QWidget *parent = 0);

private slots:

  /** Saves the changes on this page */
  void save();

private:
  /** Qt Designer generated object */
  Ui::StatusMessage ui;
};

#endif

