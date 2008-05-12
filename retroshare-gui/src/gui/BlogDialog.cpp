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

#include "rsiface/rsQblog.h"

/** Constructor */
BlogDialog::BlogDialog(QWidget *parent)
: MainPage (parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  setupUi(this);
  
  connect(sendBtn, SIGNAL(clicked()), this, SLOT(sendBlog()));
  connect(statusBtn, SIGNAL(clicked()), this, SLOT(setStatus()));
  connect(boldBtn, SIGNAL(clicked()), this, SLOT(setFont()));
  connect(underlineBtn, SIGNAL(clicked()), this, SLOT(setFont()));
  connect(italicBtn, SIGNAL(clicked()), this, SLOT(setFont()));
  
  /* Current Font */
  mCurrentFont = QFont("Comic Sans MS", 8);

  /* Font for username and timestamp */
  mUsrFont = QFont("Comic Sans MS", 8);
  
}

/* Currently both status and message are the same, will soon be changed
 * Status will stay the same, well relatively.
 * However the blog will only display when the user is clicked
 * and will have multiple messages from the same user.
 */

void BlogDialog::sendBlog()
{
	QString blogMsg = lineEdit->toPlainText();
	QString debug = "unsuccessful";
	QString blogUsr;
	
	std::list<std::string> UsrList;
	
	/* test to see if load dummy data worked ! */
	
	 if(!rsQblog->getFriendList(UsrList))
	 {
	 	blogText->append(debug); // put error on screen if problem getting usr list
	 }
	 else
	 {
	 	if(blogMsg == "")
	 	{
	 		QMessageBox::information(this, tr("No message entered"),
            tr("Please enter a message."),QMessageBox::Ok,
            QMessageBox::Ok);
	 	}
	 	else
	 	{
	 		
	 		/* I know its messy, I will clean it up */
	 		blogUsr = UsrList.begin()->c_str();
	 		blogText->setCurrentFont(mUsrFont); // make bold for username
	 		blogText->setTextColor(QColor(255, 0, 0, 255));
	 		QTime qtime;
			qtime = qtime.currentTime();
			QString timestamp = qtime.toString("H:m:s");
	 		blogText->append("[" + timestamp + "] " + blogUsr); // past the first usr list to blog screen
	 		blogText->setCurrentFont(mCurrentFont); // reset the font for blog message
	 		blogText->setTextColor(QColor(0, 0, 0, 255));
	 		blogText->append(blogMsg); // append the users message
	 	}
	 }
	 
	/* Clear lineEdit */
	lineEdit->clear();
	
	/* setFocus on lineEdit */
	lineEdit->setFocus();
}

void BlogDialog::setFont()
{
	mCurrentFont.setUnderline(underlineBtn->isChecked());
	mCurrentFont.setItalic(italicBtn->isChecked());
	mCurrentFont.setBold(boldBtn->isChecked());
	lineEdit->setFont(mCurrentFont);
	lineEdit->setFocus();
}

/* Currently both status and message are the same, will soon be changed
 * Status will stay the same, well relatively.
 * However the blog will only display when the user is clicked
 * and will have multiple messages from the same user.
 */
void BlogDialog::setStatus()
{
	QString statusMsg = lineEdit->toPlainText();
	QString debug = "unsuccessful";
	QString blogUsr;
	
	std::list<std::string> UsrList;
	
	/* test to see if load dummy data worked ! */
	
	 if(!rsQblog->getFriendList(UsrList))
	 {
	 	blogText->append(debug); // put error on screen if problem getting usr list
	 }
	 else
	 {
	 	if(statusMsg == "")
	 	{
	 		QMessageBox::information(this, tr("No message"),
            tr("Please enter a message."),QMessageBox::Ok,
            QMessageBox::Ok);
	 	}
	 	else
	 	{
	 		blogUsr = UsrList.begin()->c_str();
	 		blogText->setCurrentFont(mUsrFont); // make bold for username
	 		blogText->append(blogUsr + ": "); // past the first usr list to blog screen
	 		blogText->setCurrentFont(mCurrentFont); // reset the font for blog message
	 		blogText->append(statusMsg); // append the users message
	 	}
	 }
	 
	/* Clear lineEdit */
	lineEdit->clear();
	
	/* setFocus on lineEdit */
	lineEdit->setFocus();
		
}
	
	
	


