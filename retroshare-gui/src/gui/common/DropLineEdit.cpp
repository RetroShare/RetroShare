/****************************************************************
 *
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2012, RetroShare Team
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

#include <QDropEvent>
#include <QUrl>
#include <QFileInfo>
#include <QMimeData>

#include "DropLineEdit.h"

DropLineEdit::DropLineEdit(QWidget *parent)
	: QLineEdit(parent)
{
	accept.text = false;
	accept.file = false;
	accept.dir = false;

	setAcceptDrops(true);
}

void DropLineEdit::setAcceptText(bool on)
{
	accept.text = on;
}

void DropLineEdit::setAcceptFile(bool on)
{
	accept.file = on;
}

void DropLineEdit::setAcceptDir(bool on)
{
	accept.dir = on;
}

void DropLineEdit::dragEnterEvent(QDragEnterEvent *event)
{
	if (accept.text && event->mimeData()->hasText()) {
		event->acceptProposedAction();
		return;
	}

	if ((accept.file || accept.dir) && event->mimeData()->hasUrls()) {
		event->acceptProposedAction();
		return;
	}
}

void DropLineEdit::dropEvent(QDropEvent *event)
{
	if (event->mimeData()->hasText()) {
		setText(event->mimeData()->text());
	}

	if (event->mimeData()->hasUrls()) {
		QList<QUrl> urlList = event->mimeData()->urls();

		/* if just text was dropped, urlList is empty (size == 0) */
		if (urlList.size() > 0) {
			/* if at least one QUrl is present in list */
			QString name = urlList[0].toLocalFile();

			QFileInfo info;
			info.setFile(name);
			if (accept.file && info.isFile()) {
				setText(name);
			}
			if (accept.dir && info.isDir()) {
				setText(name);
			}
		}
	}

	event->acceptProposedAction();
}
