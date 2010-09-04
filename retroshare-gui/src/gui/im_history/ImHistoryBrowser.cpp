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

#include <QMessageBox>
#include <QDateTime>
#include <QMenu>
#include <QClipboard>
#include <QTextDocument>

#include "ImHistoryBrowser.h"
#include "IMHistoryItemDelegate.h"
#include "IMHistoryItemPainter.h"

#include "rshare.h"
#include "gui/settings/rsharesettings.h"

#define ROLE_PLAINTEXT Qt::UserRole

/** Default constructor */
ImHistoryBrowser::ImHistoryBrowser(bool isPrivateChatIn, IMHistoryKeeper &histKeeper, QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags), historyKeeper(histKeeper)
{
    /* Invoke Qt Designer generated QObject setup routine */
    ui.setupUi(this);

    isPrivateChat = isPrivateChatIn;

    connect(&historyKeeper, SIGNAL(historyAdd(IMHistoryItem)), this, SLOT(historyAdd(IMHistoryItem)));
    connect(&historyKeeper, SIGNAL(historyClear()), this, SLOT(historyClear()));

    connect(ui.clearButton, SIGNAL(clicked()), this, SLOT(clearFilter()));
    connect(ui.filterPatternLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(filterRegExpChanged()));

    ui.clearButton->hide();

    // embed smileys ?
    if (isPrivateChat) {
        embedSmileys = Settings->valueFromGroup(QString("Chat"), QString::fromUtf8("Emoteicons_PrivatChat"), true).toBool();
    } else {
        embedSmileys = Settings->valueFromGroup(QString("Chat"), QString::fromUtf8("Emoteicons_GroupChat"), true).toBool();
    }

    style.setStylePath(":/qss/chat/history");
    style.loadEmoticons();

    ui.listWidget->setItemDelegate(new IMHistoryItemDelegate);

    QList<IMHistoryItem> historyItems;
    historyKeeper.getMessages(historyItems, 0);
    foreach(IMHistoryItem item, historyItems) {
        addItem(item);
    }
}

void ImHistoryBrowser::historyAdd(IMHistoryItem item)
{
    QListWidgetItem *itemWidget = addItem(item);
    if (itemWidget) {
        filterItems(itemWidget);
    }
}

void ImHistoryBrowser::historyClear()
{
    ui.listWidget->clear();
}

QListWidgetItem *ImHistoryBrowser::addItem(IMHistoryItem &item)
{
    unsigned int formatFlag = CHAT_FORMATMSG_EMBED_LINKS;

    if (embedSmileys) {
        formatFlag |= CHAT_FORMATMSG_EMBED_SMILEYS;
    }

    ChatStyle::enumFormatMessage type;
    if (item.incoming) {
        type = ChatStyle::FORMATMSG_INCOMING;
    } else {
        type = ChatStyle::FORMATMSG_OUTGOING;
    }

    QString formatMsg = style.formatMessage(type, item.name, item.sendTime, item.messageText, formatFlag);

    QListWidgetItem *itemWidget = new QListWidgetItem;
    itemWidget->setData(Qt::DisplayRole, qVariantFromValue(IMHistoryItemPainter(formatMsg)));

    /* calculate plain text */
    QTextDocument doc;
    doc.setHtml(item.messageText);
    itemWidget->setData(ROLE_PLAINTEXT, doc.toPlainText());
    ui.listWidget->addItem(itemWidget);

    return itemWidget;
}

void ImHistoryBrowser::filterRegExpChanged()
{
    QString text = ui.filterPatternLineEdit->text();

    if (text.isEmpty()) {
        ui.clearButton->hide();
    } else {
        ui.clearButton->show();
    }

    filterItems();
}

void ImHistoryBrowser::clearFilter()
{
    ui.filterPatternLineEdit->clear();
    ui.filterPatternLineEdit->setFocus();
}

void ImHistoryBrowser::filterItems(QListWidgetItem *item)
{
    QString text = ui.filterPatternLineEdit->text();

    QRegExp regExp(text);
    if (item == NULL) {
        int count = ui.listWidget->count();
        for (int i = 0; i < count; i++) {
            item = ui.listWidget->item(i);
            if (text.isEmpty()) {
                item->setHidden(false);
            } else {
                if (item->data(ROLE_PLAINTEXT).toString().contains(regExp)) {
                    item->setHidden(false);
                } else {
                    item->setHidden(true);
                }
            }
        }
    } else {
        if (text.isEmpty()) {
            item->setHidden(false);
        } else {
            if (item->data(ROLE_PLAINTEXT).toString().contains(regExp)) {
                item->setHidden(false);
            } else {
                item->setHidden(true);
            }
        }
    }
}
