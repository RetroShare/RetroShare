/****************************************************************
 *  RShare is distributed under the following license:
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

#ifndef _EXAMPLEDIALOG_H
#define _EXAMPLEDIALOG_H

#include <QFileDialog>

#include "retroshare-gui/mainpage.h"
#include "ui_ExampleDialog.h"


class ExampleDialog : public MainPage
{
  Q_OBJECT

public:
  /** Default Constructor */
  ExampleDialog(QWidget *parent = 0);
  /** Default Destructor */

  void  insertExample();

private slots:
  /** Create the context popup menu and it's submenus */
  void peertreeWidgetCostumPopupMenu( QPoint point );

  void voteup();
  void votedown();

private:

  /* Worker Functions */
  /* (1) Update Display */

  /* (2) Utility Fns */
  QTreeWidgetItem *getCurrentLine();

  /** Define the popup menus for the Context menu */
  QMenu* contextMnu;
    /** Defines the actions for the context menu */
  QAction* voteupAct;
  QAction* votedownAct;

  QTreeWidget *exampletreeWidget;

  /** Qt Designer generated object */
  Ui::ExampleDialog ui;
};

#endif

