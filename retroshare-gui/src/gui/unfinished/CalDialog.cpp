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

#include <QFile>
#include <QFileInfo>
#include <QDateTime>

#include <QDesktopServices>
#include <QUrl>

//#include "rshare.h"
#include "CalDialog.h"
//#include <retroshare/rspeers.h>
//#include <retroshare/rsrank.h>

#include <iostream>
#include <map>

#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QPixmap>
#include <QMessageBox>
#include <QHeaderView>
#include <QTimer>
#include <QTableWidget>
#include <QComboBox>

using namespace std;

/** Constructor */
CalDialog::CalDialog(QWidget *parent)
: MainPage(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  //declare local variables for GUI items
  name = ui.lne_name;
  location = ui.lne_location;
  status = ui.cbx_status;
  calendarList = ui.lst_calList;

  //Set GUI Item Properties
  location->setEnabled(false);

  //Create GUI Connections
  connect(ui.btn_add,SIGNAL(clicked()),this,SLOT(addItem()));
  connect(ui.btn_addFile,SIGNAL(clicked()),this,SLOT(addFile()));
  connect(ui.lst_calList,SIGNAL(itemPressed(QListWidgetItem*)),this,SLOT(setDetails(QListWidgetItem*)));
  connect(ui.btn_remove,SIGNAL(clicked()),this,SLOT(removeItem()));
}

void CalDialog::addItem()
{
  itemDetails = new CalItem();
  if(name->text() != QString(""))
    itemDetails->name = name->text();
  if(location->text() != QString(""))
    itemDetails->location = QString(location->text());
  itemDetails->status = status->currentIndex();

  //Add Item to Calendar Item Map
  calItems.insert(itemDetails->name, *itemDetails);
  clearDetails();  //Clear Textbox Details (and itemDetails pointer)
  updateList();    //Update listWidget with new map data.
}

void CalDialog::removeItem()
{
  QListWidgetItem* item = calendarList->currentItem();
  calItems.remove(item->text());
  clearDetails();
  updateList();
}

void CalDialog::updateList()
{
  calendarList->clear();  //Clear existing items
  //Rebuild list from Map
  QMap<QString, CalItem>::iterator it;
  for(it = calItems.begin();it != calItems.end();++it)
    calendarList->addItem(it.key());
}

void CalDialog::clearDetails()
{
  name->setText("");
  location->setText("");
  status->setCurrentIndex(0);
  itemDetails = NULL;
}

void CalDialog::addFile()
{
  //Prompt for ICS calendar file
  QStringList file = QFileDialog::getOpenFileNames(this,
			"Select calendar to share",location->text(), "Calendars (*.ics)");

  //Populate location LineEdit if file selected
  if(!file.empty())
    location->setText(*file.begin());
}

void CalDialog::setDetails(QListWidgetItem* item)
{
  clearDetails();
  QMap<QString, CalItem>::iterator it = calItems.find(item->text());
  CalItem temp = it.value();
  name->setText(temp.name);
  location->setText(temp.location);
  status->setCurrentIndex(temp.status);
}
