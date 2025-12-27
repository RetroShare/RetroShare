/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2007, RetroShare Team
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


#ifndef _PROFILE_VIEW_H
#define _PROFILE_VIEW_H

#include "ui_ProfileView.h"

#include <string>
#include <QDialog>

class ProfileView : public QDialog
{
  Q_OBJECT

public:
  /** Default Constructor */

  ProfileView(QWidget *parent = 0);
  /** Default Destructor */

void setPeerId(std::string id);

void clear();
void update();
void loadAvatar();


public slots:

private slots:

  /** context popup menus */
  void imageCustomPopupMenu( QPoint point );
  void profileCustomPopupMenu( QPoint point );
  void fileCustomPopupMenu( QPoint point );

  /* For context Menus */
  /* Image Context Menu */
  void selectimagefile();
  void clearimage();

  /* for Profile */
  void profileEdit();

  /* for Favourite Files */
  void fileRemove();
  void filesClear();
  void fileDownload();
  void filesDownloadAll();
  /* add must be done from Shared Files */


 /* expand / close */
 void expand();
 void closeView();

private:
  std::string pId;
  bool mIsOwnId;

  QPixmap picture;


  /** Qt Designer generated object */
  Ui::ProfileView ui;

};

#endif

