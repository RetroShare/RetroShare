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
#include <QtGui>

#include "BlogDialog.h"

/** Constructor */
BlogDialog::BlogDialog(QWidget *parent)
: MainPage (parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  setupUi(this);
  
  connect(sendBtn, SIGNAL(clicked()), this, SLOT(sendBlog()));
  connect(boldBtn, SIGNAL(clicked()), this, SLOT(setFont()));
  connect(underlineBtn, SIGNAL(clicked()), this, SLOT(setFont()));
  connect(italicBtn, SIGNAL(clicked()), this, SLOT(setFont()));
  
  mCurrentFont = QFont("Comic Sans MS", 8);
  
}

void BlogDialog::sendBlog()
{
	QString msg = lineEdit->toPlainText();
	
	msgText->setCurrentFont(mCurrentFont);
		
	/* Write text into windows */
	msgText->append(msg);
	
	/* Clear lineEdit */
	lineEdit->clear();
	
}

void BlogDialog::setFont()
{
	mCurrentFont.setUnderline(underlineBtn->isChecked());
	mCurrentFont.setItalic(italicBtn->isChecked());
	mCurrentFont.setBold(boldBtn->isChecked());
	lineEdit->setFont(mCurrentFont);
	lineEdit->setFocus();
}
	
	


