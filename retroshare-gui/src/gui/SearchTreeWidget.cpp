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

#include <QMimeData>

#include "SearchTreeWidget.h"

#include <iostream>

SearchTreeWidget::SearchTreeWidget(QWidget *parent)
    :QTreeWidget(parent)
{
   	return;
}

QMimeData * SearchTreeWidget::mimeData ( const QList<QTreeWidgetItem *> items ) const
{
	/* extract from each QTreeWidgetItem... all the member text */
	QList<QTreeWidgetItem *>::const_iterator it;
	QString text;
	for(it = items.begin(); it != items.end(); it++)
	{
		QString line = QString("%1/%2/%3/").arg((*it)->text(SR_NAME_COL), (*it)->text(SR_HASH_COL), (*it)->text(SR_SIZE_COL));

		bool isLocal = (*it)->data(SR_DATA_COL, SR_ROLE_LOCAL).toBool();
		if (isLocal)
		{
			line += "Local";
		}
		else
		{
			line += "Remote";
		}
		line += "/\n";

		text += line;
	}

	std::cerr << "Created MimeData:";
	std::cerr << std::endl;

	std::string str = text.toUtf8().constData();
	std::cerr << str;
	std::cerr << std::endl;

	QMimeData *data = new QMimeData();
	data->setData("application/x-rsfilelist", QByteArray(str.c_str()));

	return data;
}



QStringList SearchTreeWidget::mimeTypes () const 
{
	QStringList list;
	list.push_back("application/x-rsfilelist");

	return list;
}


Qt::DropActions SearchTreeWidget::supportedDropActions () const
{
	return Qt::CopyAction;
}

