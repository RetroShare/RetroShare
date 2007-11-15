/****************************************************************
 *  RShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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


#ifndef _CHAN_CREATE_DIALOG_H
#define _CHAN_CREATE_DIALOG_H

#include <QMainWindow>

//#include <config/rsharesettings.h>

#include "ui_ChanCreateDialog.h"

class ChanCreateDialog : public QMainWindow 
{
  Q_OBJECT

public:
  /** Default Constructor */

  ChanCreateDialog(QWidget *parent = 0, Qt::WFlags flags = 0);
  /** Default Destructor */

void  newChan(); /* cleanup */

private slots:

	/* actions to take.... */
void  createChan();
void  cancelChan();

private:

  /** Qt Designer generated object */
  Ui::ChanCreateDialog ui;
};

#endif

