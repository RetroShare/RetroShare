/****************************************************************
 *  RShare is distributed under the following license:
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

#ifndef _PHOTOSHOW_H
#define _PHOTOSHOW_H

#include <QFileDialog>

#include "gui/mainpage.h"
#include "ui_PhotoShow.h"

#include <map>


class PhotoShow : public QWidget
{
  Q_OBJECT

public:
  /** Default Constructor */
  PhotoShow(QWidget *parent = 0);
  /** Default Destructor */

  void  setPeerId(std::string id);
  void  setPhotoId(std::string id);
  void  setPhotoShow(std::string id);


private slots:
  /** Create the context popup menu and it's submenus */
  void photoCustomPopupMenu( QPoint point );

  void updatePhotoShow();


private:

  void updatePhoto(std::string pid, std::string photoId);

  bool mDoShow;
  std::string mPeerId;
  std::string mShowId;
  std::string mPhotoId;

  /** Qt Designer generated object */
  Ui::PhotoShow ui;
};

#endif

