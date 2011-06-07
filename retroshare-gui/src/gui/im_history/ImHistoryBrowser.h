/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2010 The RetroShare Team
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


#ifndef _IMHISTORYBROWSER_H
#define _IMHISTORYBROWSER_H

#include <QDialog>
#include <QThread>

#include <retroshare/rsmsgs.h>
#include "IMHistoryKeeper.h"
#include "gui/chat/ChatStyle.h"

#include "ui_ImHistoryBrowser.h"

class QTextEdit;
class ImHistoryBrowserCreateItemsThread;

class ImHistoryBrowser : public QDialog
{
    Q_OBJECT

    friend class ImHistoryBrowserCreateItemsThread;

public:
    /** Default constructor */
    ImHistoryBrowser(const std::string &peerId, IMHistoryKeeper &histKeeper, QTextEdit *edit, QWidget *parent = 0, Qt::WFlags flags = 0);
    /** Default destructor */
    virtual ~ImHistoryBrowser();

protected:
    bool eventFilter(QObject *obj, QEvent *ev);

private slots:
    void createThreadFinished();
    void createThreadProgress(int current, int count);

    void historyAdd(IMHistoryItem item);
    void historyRemove(IMHistoryItem item);
    void historyClear();

    void filterRegExpChanged();
    void clearFilter();

    void itemSelectionChanged();
    void customContextMenuRequested(QPoint pos);

    void copyMessage();
    void removeMessages();
    void clearHistory();
    void sendMessage();

    void privateChatChanged(int list, int type);

private:
    QListWidgetItem *createItem(IMHistoryItem &item);
    void fillItem(QListWidgetItem *itemWidget, IMHistoryItem &item);
    void filterItems(QListWidgetItem *item = NULL);

    void getSelectedItems(QList<int> &items);

    ImHistoryBrowserCreateItemsThread *m_createThread;

    std::string m_peerId;
    bool m_isPrivateChat;
    QTextEdit *textEdit;
    bool embedSmileys;
    IMHistoryKeeper &historyKeeper;
    ChatStyle style;

    std::list<ChatInfo> m_savedOfflineChat;
    QList<IMHistoryItem> m_itemsAddedOnLoad;

    /** Qt Designer generated object */
    Ui::ImHistoryBrowser ui;
};

class ImHistoryBrowserCreateItemsThread : public QThread
{
    Q_OBJECT

public:
    ImHistoryBrowserCreateItemsThread(ImHistoryBrowser *parent, IMHistoryKeeper &histKeeper);
    ~ImHistoryBrowserCreateItemsThread();

    void run();
    void stop();
    bool wasStopped() { return stopped; }

signals:
    void progress(int current, int count);

public:
    QList<QListWidgetItem*> m_items;

private:
    IMHistoryKeeper &m_historyKeeper;
    ImHistoryBrowser *m_historyBrowser;
    volatile bool stopped;
};

#endif
