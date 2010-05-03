/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2010 RetroShare Team
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

#include "rsiface/rspeers.h" //for rsPeers variable
#include "rsiface/rsiface.h"

#include <QtGui>
#include <QClipboard>

#include <rshare.h>
#include "ChatPage.h"

#include <sstream>
#include <iostream>
#include <set>

/** Constructor */
ChatPage::ChatPage(QWidget * parent, Qt::WFlags flags)
    : ConfigPage(parent, flags)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  /* Create RshareSettings object */
  _settings = new RshareSettings();

  //connect(ui.copykeyButton, SIGNAL(clicked()), this, SLOT(copyPublicKey()));
  //connect(ui.saveButton, SIGNAL(clicked()), this, SLOT(fileSaveAs()));


  //loadPublicKey();


  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void
ChatPage::closeEvent (QCloseEvent * event)
{

    QWidget::closeEvent(event);
}

/** Saves the changes on this page */
bool
ChatPage::save(QString &errmsg)
{
  _settings->setValue(QString::fromUtf8("Emoteicons_PrivatChat"), emotePrivatChat());

  _settings->setValue(QString::fromUtf8("Emoteicons_GroupChat"), emoteGroupChat());

  _settings->setValue(QString::fromUtf8("GroupChat_History"), groupchatHistory());

 	return true;
}

/** Loads the settings for this page */
void
ChatPage::load()
{

  ui.checkBox_emoteprivchat->setChecked(_settings->value(QString::fromUtf8("Emoteicons_PrivatChat"), true).toBool());

  ui.checkBox_emotegroupchat->setChecked(_settings->value(QString::fromUtf8("Emoteicons_GroupChat"), true).toBool());
  
  ui.checkBox_groupchathistory->setChecked(_settings->value(QString::fromUtf8("GroupChat_History"), true).toBool());

}

bool ChatPage::emotePrivatChat() const {
  if(ui.checkBox_emoteprivchat->isChecked()) return true;
  return ui.checkBox_emoteprivchat->isChecked();
}

bool ChatPage::emoteGroupChat() const {
  if(ui.checkBox_emotegroupchat->isChecked()) return true;
  return ui.checkBox_emotegroupchat->isChecked();
}

bool ChatPage::groupchatHistory() const {
  if(ui.checkBox_groupchathistory->isChecked()) return true;
  return ui.checkBox_groupchathistory->isChecked();
}
