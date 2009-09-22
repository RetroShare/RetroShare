/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2009, RetroShare Team
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


#include "InfoDialog.h"
#include "rsiface/rsiface.h"

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

/* Images for context menu icons */
#define IMAGE_DOWNLOAD       ":/images/start.png"

/** Constructor */
InfoDialog::InfoDialog(QWidget *parent)
:QDialog(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);
  
  connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));


  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}


