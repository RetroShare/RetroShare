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
#include <QTextEdit>
#include <QClipboard>
#include <QKeyEvent>
#include <QThread>

#include "ImHistoryBrowser.h"
#include "IMHistoryItemDelegate.h"
#include "IMHistoryItemPainter.h"
#include "util/HandleRichText.h"

#include "rshare.h"
#include <retroshare/rshistory.h>
#include "gui/settings/rsharesettings.h"
#include "gui/notifyqt.h"

#define ROLE_MSGID     Qt::UserRole
#define ROLE_PLAINTEXT Qt::UserRole + 1
#define ROLE_OFFLINE   Qt::UserRole + 2

ImHistoryBrowserCreateItemsThread::ImHistoryBrowserCreateItemsThread(ImHistoryBrowser *parent, const std::string& peerId)
    : QThread(parent)
{
    m_peerId = peerId;
    m_historyBrowser = parent;
    stopped = false;
}

ImHistoryBrowserCreateItemsThread::~ImHistoryBrowserCreateItemsThread()
{
    // remove all items (when items are available, the thread was terminated)
    QList<QListWidgetItem*>::iterator it;
    for (it = m_items.begin(); it != m_items.end(); it++) {
        delete(*it);
    }

    m_items.clear();
}

void ImHistoryBrowserCreateItemsThread::stop()
{
    disconnect();
    stopped = true;
    wait();
}

void ImHistoryBrowserCreateItemsThread::run()
{
    std::list<HistoryMsg> historyMsgs;
    rsHistory->getMessages(m_peerId, historyMsgs, 0);

    int count = historyMsgs.size();
    int current = 0;

    std::list<HistoryMsg>::iterator it;
    for (it = historyMsgs.begin(); it != historyMsgs.end(); it++) {
        if (stopped) {
            break;
        }
        QListWidgetItem *itemWidget = m_historyBrowser->createItem(*it);
        if (itemWidget) {
            m_items.push_back(itemWidget);
            emit progress(++current, count);
        }
    }
}

/** Default constructor */
ImHistoryBrowser::ImHistoryBrowser(const std::string &peerId, QTextEdit *edit, QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags)
{
    /* Invoke Qt Designer generated QObject setup routine */
    ui.setupUi(this);

    ui.headerFrame->setHeaderImage(QPixmap(":/images/user/agt_forum64.png"));
    ui.headerFrame->setHeaderText(tr("Message History"));

    m_peerId = peerId;
    m_isPrivateChat = !m_peerId.empty();
    textEdit = edit;

    connect(NotifyQt::getInstance(), SIGNAL(historyChanged(uint, int)), this, SLOT(historyChanged(uint, int)));

    connect(ui.filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));

    connect(ui.copyButton, SIGNAL(clicked()), SLOT(copyMessage()));
    connect(ui.removeButton, SIGNAL(clicked()), SLOT(removeMessages()));

    connect(ui.listWidget, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));
    connect(ui.listWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customContextMenuRequested(QPoint)));

    ui.filterLineEdit->showFilterIcon();

    // embed smileys ?
    if (m_isPrivateChat) {
        embedSmileys = Settings->valueFromGroup("Chat", "Emoteicons_PrivatChat", true).toBool();
    } else {
        embedSmileys = Settings->valueFromGroup("Chat", "Emoteicons_GroupChat", true).toBool();
    }

    style.setStyleFromSettings(ChatStyle::TYPE_HISTORY);

    ui.listWidget->setItemDelegate(new IMHistoryItemDelegate);

    QByteArray geometry = Settings->valueFromGroup("HistorieBrowser", "Geometry", QByteArray()).toByteArray();
    if (geometry.isEmpty() == false) {
        restoreGeometry(geometry);
    }

    // dummy call for set buttons
    itemSelectionChanged();

    ui.listWidget->installEventFilter(this);

    m_createThread = new ImHistoryBrowserCreateItemsThread(this, m_peerId);
    connect(m_createThread, SIGNAL(finished()), this, SLOT(createThreadFinished()));
    connect(m_createThread, SIGNAL(progress(int,int)), this, SLOT(createThreadProgress(int,int)));
    m_createThread->start();
}

ImHistoryBrowser::~ImHistoryBrowser()
{
    Settings->setValueToGroup("HistorieBrowser", "Geometry", saveGeometry());

    if (m_createThread) {
        m_createThread->stop();
        delete(m_createThread);
        m_createThread = NULL;
    }
}

void ImHistoryBrowser::createThreadFinished()
{
    if (m_createThread == sender()) {
        ui.progressBar->setVisible(false);

        if (!m_createThread->wasStopped()) {
            // append created items
            QList<QListWidgetItem*>::iterator it;
            for (it = m_createThread->m_items.begin(); it != m_createThread->m_items.end(); it++) {
                ui.listWidget->addItem(*it);
            }

            // clear list
            m_createThread->m_items.clear();

            filterChanged(ui.filterLineEdit->text());

            // dummy call for set buttons
            itemSelectionChanged();

            m_createThread->deleteLater();
            m_createThread = NULL;

            QList<HistoryMsg>::iterator histIt;
            for (histIt = itemsAddedOnLoad.begin(); histIt != itemsAddedOnLoad.end(); histIt++) {
                historyAdd(*histIt);
            }
            itemsAddedOnLoad.clear();
        }
    }
}

void ImHistoryBrowser::createThreadProgress(int current, int count)
{
    if (count) {
        ui.progressBar->setValue(current * ui.progressBar->maximum() / count);
    }
}

bool ImHistoryBrowser::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui.listWidget) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent && keyEvent->key() == Qt::Key_Delete) {
                // Delete pressed
                removeMessages();
                return true; // eat event
            }
        }
    }
    // pass the event on to the parent class
    return QDialog::eventFilter(obj, event);
}

void ImHistoryBrowser::historyAdd(HistoryMsg& msg)
{
    if (m_createThread) {
        // create later
        itemsAddedOnLoad.push_back(msg);
        return;
    }

    QListWidgetItem *itemWidget = createItem(msg);
    if (itemWidget) {
        ui.listWidget->addItem(itemWidget);
        filterItems(ui.filterLineEdit->text(), itemWidget);
    }
}

void ImHistoryBrowser::historyChanged(uint msgId, int type)
{
    if (type == NOTIFY_TYPE_ADD) {
        /* history message added */
        HistoryMsg msg;
        if (rsHistory->getMessage(msgId, msg) == false) {
            return;
        }

        historyAdd(msg);

        return;
    }

    if (type == NOTIFY_TYPE_DEL) {
        /* history message removed */
        int count = ui.listWidget->count();
        for (int i = 0; i < count; i++) {
            QListWidgetItem *itemWidget = ui.listWidget->item(i);
            if (itemWidget->data(ROLE_MSGID).toString().toUInt() == msgId) {
                delete(ui.listWidget->takeItem(i));
                break;
            }
        }
        return;
    }

    if (type == NOTIFY_TYPE_MOD) {
        /* clear history */
        ui.listWidget->clear();
        return;
    }
}

void ImHistoryBrowser::fillItem(QListWidgetItem *itemWidget, HistoryMsg& msg)
{
    unsigned int formatTextFlag = RSHTML_FORMATTEXT_EMBED_LINKS;

    if (embedSmileys) {
        formatTextFlag |= RSHTML_FORMATTEXT_EMBED_SMILEYS;
    }

    ChatStyle::enumFormatMessage type;
    if (msg.incoming) {
        type = ChatStyle::FORMATMSG_INCOMING;
    } else {
        type = ChatStyle::FORMATMSG_OUTGOING;
    }

    QString messageText = RsHtml().formatText(NULL, QString::fromUtf8(msg.message.c_str()), formatTextFlag);
    QString formatMsg = style.formatMessage(type, QString::fromUtf8(msg.peerName.c_str()), QDateTime::fromTime_t(msg.sendTime), messageText);

    itemWidget->setData(Qt::DisplayRole, qVariantFromValue(IMHistoryItemPainter(formatMsg)));
    itemWidget->setData(ROLE_MSGID, msg.msgId);
    itemWidget->setData(ROLE_OFFLINE, (type == ChatStyle::FORMATMSG_OOUTGOING) ? true : false);

    /* calculate plain text */
    QTextDocument doc;
    doc.setHtml(messageText);
    itemWidget->setData(ROLE_PLAINTEXT, doc.toPlainText());
}

QListWidgetItem *ImHistoryBrowser::createItem(HistoryMsg& msg)
{
    QListWidgetItem *itemWidget = new QListWidgetItem;
    fillItem(itemWidget, msg);
    return itemWidget;
}

void ImHistoryBrowser::filterChanged(const QString &text)
{
    filterItems(text);
}

void ImHistoryBrowser::filterItems(const QString &text, QListWidgetItem *item)
{
    if (item == NULL) {
        int count = ui.listWidget->count();
        for (int i = 0; i < count; i++) {
            item = ui.listWidget->item(i);
            if (text.isEmpty()) {
                item->setHidden(false);
            } else {
                if (item->data(ROLE_PLAINTEXT).toString().contains(text, Qt::CaseInsensitive)) {
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
            if (item->data(ROLE_PLAINTEXT).toString().contains(text, Qt::CaseInsensitive)) {
                item->setHidden(false);
            } else {
                item->setHidden(true);
            }
        }
    }
}

void ImHistoryBrowser::getSelectedItems(std::list<uint32_t> &items)
{
    QList<QListWidgetItem*> itemWidgets = ui.listWidget->selectedItems();

    QList<QListWidgetItem*>::iterator it;
    for (it = itemWidgets.begin(); it != itemWidgets.end(); it++) {
        QListWidgetItem *item = *it;
        if (item->isHidden()) {
            continue;
        }
        items.push_back(item->data(ROLE_MSGID).toString().toInt());
    }
}

void ImHistoryBrowser::itemSelectionChanged()
{
    std::list<uint32_t> msgIds;
    getSelectedItems(msgIds);

    if (msgIds.size()) {
        // activate buttons
        ui.copyButton->setEnabled(true);
        ui.removeButton->setEnabled(true);
    } else {
        // deactivate buttons
        ui.copyButton->setDisabled(true);
        ui.removeButton->setDisabled(true);
    }
}

void ImHistoryBrowser::customContextMenuRequested(QPoint /*pos*/)
{
    std::list<uint32_t> msgIds;
    getSelectedItems(msgIds);

    QListWidgetItem *currentItem = ui.listWidget->currentItem();

    QMenu contextMnu(this);

    QAction *selectAll = new QAction(tr("Mark all"), &contextMnu);
    QAction *copyMessage = new QAction(tr("Copy"), &contextMnu);
    QAction *removeMessages = new QAction(tr("Delete"), &contextMnu);
    QAction *clearHistory = new QAction(tr("Clear history"), &contextMnu);

    QAction *sendItem = NULL;
    if (textEdit) {
        sendItem = new QAction(tr("Send"), &contextMnu);
        if (currentItem) {
            connect(sendItem, SIGNAL(triggered()), this, SLOT(sendMessage()));
        } else {
            sendItem->setDisabled(true);
        }
    }

    if (msgIds.size()) {
        connect(selectAll, SIGNAL(triggered()), ui.listWidget, SLOT(selectAll()));
        connect(copyMessage, SIGNAL(triggered()), this, SLOT(copyMessage()));
        connect(removeMessages, SIGNAL(triggered()), this, SLOT(removeMessages()));
        connect(clearHistory, SIGNAL(triggered()), this, SLOT(clearHistory()));
    } else {
        selectAll->setDisabled(true);
        copyMessage->setDisabled(true);
        removeMessages->setDisabled(true);
        clearHistory->setDisabled(true);
    }

    contextMnu.addAction(selectAll);
    contextMnu.addSeparator();
    contextMnu.addAction(copyMessage);
    contextMnu.addAction(removeMessages);
    contextMnu.addAction(clearHistory);
    if (sendItem) {
        contextMnu.addSeparator();
        contextMnu.addAction(sendItem);
    }

    contextMnu.exec(QCursor::pos());
}

void ImHistoryBrowser::copyMessage()
{
    QListWidgetItem *currentItem = ui.listWidget->currentItem();
    if (currentItem) {
        uint32_t msgId = currentItem->data(ROLE_MSGID).toString().toInt();
        HistoryMsg msg;
        if (rsHistory->getMessage(msgId, msg)) {
            QTextDocument doc;
            doc.setHtml(QString::fromUtf8(msg.message.c_str()));
            QApplication::clipboard()->setText(doc.toPlainText());
        }
    }
}

void ImHistoryBrowser::removeMessages()
{
    std::list<uint32_t> msgIds;
    getSelectedItems(msgIds);

    rsHistory->removeMessages(msgIds);
}

void ImHistoryBrowser::clearHistory()
{
    rsHistory->clear(m_peerId);
}

void ImHistoryBrowser::sendMessage()
{
    if (textEdit) {
        QListWidgetItem *currentItem = ui.listWidget->currentItem();
        if (currentItem) {
            uint32_t msgId = currentItem->data(ROLE_MSGID).toString().toInt();
            HistoryMsg msg;
            if (rsHistory->getMessage(msgId, msg)) {
                textEdit->clear();
                textEdit->setText(QString::fromUtf8(msg.message.c_str()));
                textEdit->setFocus();
                QTextCursor cursor = textEdit->textCursor();
                cursor.movePosition(QTextCursor::End);
                textEdit->setTextCursor(cursor);
                close();
            }
        }
    }
}
