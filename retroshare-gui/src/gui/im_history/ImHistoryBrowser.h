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

#include "IMHistoryKeeper.h"
#include "gui/chat/ChatStyle.h"

#include "ui_ImHistoryBrowser.h"

class QTextEdit;

class ImHistoryBrowser : public QDialog
{
  Q_OBJECT

public:
    /** Default constructor */
    ImHistoryBrowser(bool isPrivateChat, IMHistoryKeeper &histKeeper, QTextEdit *edit, QWidget *parent = 0, Qt::WFlags flags = 0);
    /** Default destructor */
    virtual ~ImHistoryBrowser();

protected:
    bool eventFilter(QObject *obj, QEvent *ev);

private slots:
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

private:
    QListWidgetItem *addItem(IMHistoryItem &item);
    void filterItems(QListWidgetItem *item = NULL);

    void getSelectedItems(QList<int> &items);

    bool isPrivateChat;
    QTextEdit *textEdit;
    bool embedSmileys;
    IMHistoryKeeper &historyKeeper;
    ChatStyle style;

    /** Qt Designer generated object */
    Ui::ImHistoryBrowser ui;
};

#endif

