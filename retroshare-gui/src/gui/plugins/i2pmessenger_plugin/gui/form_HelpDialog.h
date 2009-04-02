/****************************************************************
 *  I2P Messenger is distributed under the following license:
 *
 *  Copyright (C) 2009, I2P Messenger
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

#ifndef _HELPDIALOG_H
#define _HELPDIALOG_H

#include <QFileDialog>

#include "ui_form_HelpDialog.h"


class form_HelpDialog : public QDialog 
{
  Q_OBJECT

public:
  	form_HelpDialog(QString ProgrammVersion,QString ProtocolVersion,QWidget *parent = 0);
private slots:

private:
  	Ui::form_HelpDialog ui;
};

#endif

