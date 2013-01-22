/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2010, RetroShare Team
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
#include <QTextDocumentFragment>
#include "MimeTextEdit.h"
#include "util/HandleRichText.h"

MimeTextEdit::MimeTextEdit(QWidget *parent)
	: QTextEdit(parent)
{
}

bool MimeTextEdit::canInsertFromMimeData(const QMimeData* source) const
{
#if QT_VERSION >= 0x040700
	// embedded images are not supported before QT 4.7.0
	if (source != NULL) {
		if (source->hasImage()) {
			return true;
		}
	}
#endif

	return QTextEdit::canInsertFromMimeData(source);
}

void MimeTextEdit::insertFromMimeData(const QMimeData* source)
{
#if QT_VERSION >= 0x040700
	// embedded images are not supported before QT 4.7.0
	if (source != NULL) {
		if (source->hasImage()) {
			// insert as embedded image
			QImage image = qvariant_cast<QImage>(source->imageData());
			if (image.isNull() == false) {
				QString	encodedImage;
				if (RsHtml::makeEmbeddedImage(image, encodedImage, 640*480)) {
					QTextDocumentFragment fragment = QTextDocumentFragment::fromHtml(encodedImage);
					this->textCursor().insertFragment(fragment);
					return;
				}
			}
		}
	}
#endif

	return QTextEdit::insertFromMimeData(source);
}
