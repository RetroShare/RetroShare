/****************************************************************
 *  RetroShare GUI is distributed under the following license:
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

#ifndef _LINKS_DIALOG_H
#define _LINKS_DIALOG_H

#include <QFileDialog>

#include "mainpage.h"
#include "ui_LinksDialog.h"


class LinksDialog : public MainPage
{
  Q_OBJECT

public:
  /** Default Constructor */
  LinksDialog(QWidget *parent = 0);
  /** Default Destructor */

  void  insertExample();

private slots:
  /** Create the context popup menu and it's submenus */
  void linkTreeWidgetCostumPopupMenu( QPoint point );

  void voteup_anon();
  void voteup_score(int score);
  void voteup_p2();
  void voteup_p1();
  void voteup_p0();
  void voteup_m1();
  void voteup_m2();
  void downloadSelected();

void changedSortRank( int index );
void changedSortPeriod( int index );
void changedSortFrom( int index );
void changedSortTop( int index );

void updateLinks();
void addLinkComment( void );
void toggleWindows( void );

void  openLink ( QTreeWidgetItem * item, int column );
void  changedItem(QTreeWidgetItem *curr, QTreeWidgetItem *prev);
void checkAnon();

void checkUpdate();

private:

void  updateComments(std::string rid, std::string pid);

  int mStart; /* start of rank list */
  std::string mLinkId;

  /* Worker Functions */
  /* (1) Update Display */

  /* (2) Utility Fns */
  QTreeWidgetItem *getCurrentLine();

  /** Define the popup menus for the Context menu */
  QMenu* contextMnu;
    /** Defines the actions for the context menu */
  QAction* voteupAct;
  QAction* votedownAct;
  QAction* downloadAct;

  QTreeWidget *exampletreeWidget;

  /** Qt Designer generated object */
  Ui::LinksDialog ui;
};

#endif

