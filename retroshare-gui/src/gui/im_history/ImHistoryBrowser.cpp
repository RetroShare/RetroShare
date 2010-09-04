/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2010  The RetroShare Team
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
#include "ImHistoryBrowser.h"

#include <QMessageBox>
#include <QDateTime>
#include <QMenu>
#include <QClipboard>
//#include <QDomDocument>

#include "rshare.h"

//#include "gui/chat/HandleRichText.h"

/** Default constructor */
ImHistoryBrowser::ImHistoryBrowser(IMHistoryKeeper &histKeeper, QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags), historyKeeper(histKeeper)
{
    /* Invoke Qt Designer generated QObject setup routine */
    ui.setupUi(this);

    connect(&historyKeeper, SIGNAL(historyAdd(IMHistoryItem)), this, SLOT(historyAdd(IMHistoryItem)));
    connect(&historyKeeper, SIGNAL(historyClear()), this, SLOT(historyClear()));

    QList<IMHistoryItem> historyItems;
    historyKeeper.getMessages(historyItems, 0);
    foreach(IMHistoryItem item, historyItems) {
        addItem(item);
    }
}

void ImHistoryBrowser::historyAdd(IMHistoryItem item)
{
    addItem(item);
}

void ImHistoryBrowser::historyClear()
{
    ui.textBrowser->clear();
}

void ImHistoryBrowser::addItem(IMHistoryItem &item)
{
    QString timestamp = item.sendTime.toString("hh:mm:ss");
    QString text = "<span style=\"color:#C00000\">" + timestamp + "</span>" +
                   "<span style=\"color:#2D84C9\"><strong>" + " " + item.name + "</strong></span>";

    // create a DOM tree object from the message and embed contents with HTML tags
//    QDomDocument doc;
//    doc.setContent(item.messageText);
//
//    // embed links
//    QDomElement body = doc.documentElement();
//    RsChat::embedHtml(doc, body, defEmbedAhref);
//
//    // embed smileys
//    Settings->beginGroup("Chat");
//    if (Settings->value(QString::fromUtf8("Emoteicons_GroupChat"), true).toBool()) {
//        RsChat::embedHtml(doc, body, defEmbedImg);
//    }
//    Settings->endGroup();
//
//    text += doc.toString(-1);		// -1 removes any annoying carriage return misinterpreted by QTextEdit

    text += item.messageText;

    ui.textBrowser->append(text);
}
