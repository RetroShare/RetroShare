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

#ifndef _PHOTODIALOG_H
#define _PHOTODIALOG_H

#include <QFileDialog>

#include "gui/mainpage.h"
#include "ui_PhotoDialog.h"

#include <map>


class PhotoDialog : public MainPage
{
  Q_OBJECT

public:
  /** Default Constructor */
  PhotoDialog(QWidget *parent = 0);
  /** Default Destructor */

  void  insertExample();
  void insertShowLists();

private slots:
  /** Create the context popup menu and it's submenus */
  void photoTreeWidgetCustomPopupMenu( QPoint point );
  void peerTreeWidgetCustomPopupMenu( QPoint point );

  void updatePhotoList();

  void removePhoto();
  void addPhotos();

  void checkUpdate();

   /* handle splitter */
  void togglefileview();

  void showPhoto( QTreeWidgetItem *item, int column);

private:

  void addShows(std::string peerid);

  void addPhoto(QString filename);

  /* Worker Functions */
  /* (1) Update Display */

  /* (2) Utility Fns */
  QTreeWidgetItem *getCurrentLine();

  std::map<QString, QPixmap *> photoMap;

  std::string mCurrentPID;
  std::string mCurrentSID;

  /** Define the popup menus for the Context menu */
  QMenu* contextMnu;

  QAction *rateExcellenAct;
  QAction *rateGoodAct;
  QAction *rateAverageAct;
  QAction *rateBelowAvarageAct;
  QAction *rateBadAct;
  QAction *rateUnratedAct;

  QTreeWidget *exampletreeWidget;

  /** Qt Designer generated object */
  Ui::PhotoDialog ui;
};

#endif

