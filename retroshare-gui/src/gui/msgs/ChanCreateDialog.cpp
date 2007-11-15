/****************************************************************
 *  RetroShare is distributed under the following license:
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


#include "rshare.h"
#include "ChanCreateDialog.h"

#include "rsiface/rsiface.h"
#include <sstream>

#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QPixmap>

/** Constructor */
ChanCreateDialog::ChanCreateDialog(QWidget *parent, Qt::WFlags flags)
: QMainWindow(parent, flags)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);
  
  // connect up the buttons.
  connect( ui.cancelButton, SIGNAL( clicked ( bool ) ), this, SLOT( cancelChan( ) ) );
  connect( ui.createButton, SIGNAL( clicked ( bool ) ), this, SLOT( createChan( ) ) );

    setFixedSize(QSize(392, 167));

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}


void  ChanCreateDialog::newChan()
{
	/* clear all */
}

void  ChanCreateDialog::createChan()
{
	hide();
	return;
}


void  ChanCreateDialog::cancelChan()
{
	hide();
	return;
}

