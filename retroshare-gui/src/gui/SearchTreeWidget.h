/*******************************************************************************
 * gui/SearchTreeWidget.h                                                      *
 *                                                                             *
 * Copyright (c) 2008 Robert Fernie    <retroshare.project@gmail.com>          *
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

#ifndef _SEARCH_TREE_WIDGET_H
#define _SEARCH_TREE_WIDGET_H

#include <QTreeWidget>

/* indicies for search results item columns SR_ = Search Result */
#define SR_NAME_COL         0
#define SR_SIZE_COL         1
#define SR_SOURCES_COL      2
#define SR_TYPE_COL         3
#define SR_AGE_COL          4
#define SR_HASH_COL         5
#define SR_SEARCH_ID_COL    6
#define SR_UID_COL          7
#define SR_COL_COUNT        6//8 ??
#define SR_DATA_COL         SR_NAME_COL

#define SR_ROLE_LOCAL       Qt::UserRole

class SearchTreeWidget : public QTreeWidget
{
    Q_OBJECT
        
        public:
    SearchTreeWidget(QWidget *parent = 0);

	protected:
virtual QMimeData * mimeData ( const QList<QTreeWidgetItem *> items ) const;
virtual QStringList mimeTypes () const; 
virtual Qt::DropActions supportedDropActions () const;

};


#endif

