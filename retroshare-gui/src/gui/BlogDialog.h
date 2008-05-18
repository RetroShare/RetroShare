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

#ifndef _BLOG_DIALOG_H
#define _BLOG_DIALOG_H

#include "mainpage.h"
#include "ui_BlogDialog.h"

class BlogDialog : public MainPage , private Ui::BlogDialog
{
  Q_OBJECT

public:
  /** Default Constructor */
  BlogDialog(QWidget *parent = 0);
  /** Default Destructor */

  /** Qt Designer generated object */
  Ui::BlogDialog ui;
  
public slots:
	void sendBlog();
	void setFont();
	void setStatus();
	/// populates blog service with current information from core
	void update(); 

	
private slots:
/*nothing here yet */

private:

/// to add usr to usr list: utility function for update
void addUser(const std::string& usr);

/// remove everything from usrlist and blogText box
void clear(); 

/* Current Font */
QFont mCurrentFont;

/* Font to be used for username (bold) */
QFont mUsrFont;

};



#endif

