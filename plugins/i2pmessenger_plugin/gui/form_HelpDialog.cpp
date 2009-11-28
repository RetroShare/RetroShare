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


#include "form_HelpDialog.h"

#include <iostream>
#include <sstream>

#include <QtCore/QFile>
#include <QtCore/QTextStream>

#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QPixmap>


form_HelpDialog::form_HelpDialog(QString ProgrammVersion,QString ProtocolVersion,QWidget *parent)
:QDialog(parent)
{
  
  ui.setupUi(this);
  this->setAttribute(Qt::WA_DeleteOnClose,true);

  QString tmp;  



  QFile aboutFile(QString(QApplication::applicationDirPath()+"/about.html"));
   if (aboutFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&aboutFile);
	tmp=in.readAll();
	tmp.replace("[APPVERSIONSTRING]",ProgrammVersion);
	tmp.replace("[PROTOCOLVERSIONSTRING]",ProtocolVersion);
        ui.about->setText(tmp);
   }

/*
  QFile authorsFile(QLatin1String(":/authors.html"));
   if (authorsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&authorsFile);
        ui.authors->setText(in.readAll());
   }
  QFile thanksFile(QLatin1String(":/thanks.html"));
   if (thanksFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&thanksFile);
        ui.thanks->setText(in.readAll());
   }
  */
	//ui.authors->setText
	
 
   	ui.label_2->setMinimumWidth(20);
}


