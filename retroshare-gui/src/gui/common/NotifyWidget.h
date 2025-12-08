/*******************************************************************************
 * gui/common/NotifyWidget.h                                                   *
 *                                                                             *
 * Copyright (C) 2010, Retroshare Team <retroshare.project@gmail.com>          *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifndef NOTIFYWIDGET_H
#define NOTIFYWIDGET_H

#include <QTreeWidgetItem>
#include <QWidget>
#include <QDateTime>

#include <set>

#define GTW_COLUMN_NAME         0
#define GTW_COLUMN_UNREAD       1
#define GTW_COLUMN_POSTS        2
#define GTW_COLUMN_POPULARITY   3
#define GTW_COLUMN_LAST_POST    4
#define GTW_COLUMN_SEARCH_SCORE 5
#define GTW_COLUMN_DESCRIPTION  6
#define GTW_COLUMN_COUNT        7
#define GTW_COLUMN_DATA         GTW_COLUMN_NAME

namespace Ui {
class NotifyWidget;
}

//class GroupItemInfo
//{
//public:
//    GroupItemInfo()
//      : popularity(0), publishKey(false), adminKey(false)
//      , subscribeFlags(0), max_visible_posts(0)
//    {}

//public:
//    QString               id;
//    QString               name;
//    QString               description;
//    int                   popularity;
//    QDateTime             lastpost;
//    QIcon                 icon;
//    bool                  publishKey;
//    bool                  adminKey;
//    quint32               subscribeFlags;
//    quint32               max_visible_posts ;
//    std::set<std::string> context_strings;
//};

class NotifyWidget : public QWidget
{
    Q_OBJECT

public:
    NotifyWidget(QWidget *parent = nullptr);
    ~NotifyWidget();

    // Set the unread count of an item
    void setUnreadCount(QTreeWidgetItem *item, int unreadCount);
    QTreeWidgetItem *getItemFromId(const QString &id);

private:
    Ui::NotifyWidget *ui;

};

#endif // NOTIFYWIDGET_H
