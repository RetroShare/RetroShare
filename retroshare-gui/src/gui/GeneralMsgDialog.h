/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
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

#ifndef _GENERAL_MSG_DIALOG_H
#define _GENERAL_MSG_DIALOG_H

#include "ui_GeneralMsgDialog.h"

class GeneralMsgDialog : public QDialog, private Ui::GeneralMsgDialog
{
  Q_OBJECT

public:
  /** Default Constructor */
  GeneralMsgDialog(QWidget *parent = 0);
  /** Default Destructor */

virtual void addDestination(uint32_t type, std::string grpId, std::string inReplyTo);

virtual void cancelMsg();
virtual void sendMsg();

private:

};



#endif

