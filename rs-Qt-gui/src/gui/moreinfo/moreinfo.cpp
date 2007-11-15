/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,  crypton
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

#include "moreinfo.h"

/** Default constructor */
moreinfo::moreinfo(QWidget *parent, Qt::WFlags flags)
: QDialog(parent, flags)
{
	/* Invoke Qt Designer generated QObject setup routine */
	ui.setupUi(this);

	setFixedSize(QSize(400, 300));

	//Closing window 
	connect(ui.pushButton, SIGNAL(clicked()), this, SLOT(cancel()));
}



void
moreinfo::show()
{
  //loadSettings();
  if(!this->isVisible()) {
    QDialog::show();

  }
}

void moreinfo::closeEvent (QCloseEvent * event)
{
 QWidget::closeEvent(event);
}

void moreinfo::cancel()
{
	close();
}
