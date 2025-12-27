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

#ifndef _CALENDAR_DIALOG_H
#define _CALENDAR_DIALOG_H

#include <QFileDialog>
#include <QTableWidget>
#include <QString>
#include <QMap>

#include "retroshare-gui/mainpage.h"
#include "ui_CalDialog.h"

class CalItem
{
  public:
    CalItem() {return;}
    QString name;
    QString location;
    int status;
};

class CalDialog : public MainPage
{
  Q_OBJECT

public:
/** Default Constructor */
  CalDialog(QWidget *parent = 0);

private slots:
  void addItem();
  void removeItem();
  void updateList();
  void clearDetails();
  void addFile();
  void setDetails(QListWidgetItem*);

private:
  //Should this be a vector for alphabetical sorting?
  QMap<QString, CalItem> calItems;
  CalItem *itemDetails;

/** Qt Designer generated object */
  Ui::CalDialog ui;

  QLineEdit* name;
  QLineEdit* location;
  QComboBox* status;
  QListWidget* calendarList;
};

#endif
