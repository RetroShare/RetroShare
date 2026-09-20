/*******************************************************************************
 * retroshare-gui/src/gui/gxs/CommentText.h                                    *
 *                                                                             *
 * Copyright 2026 by RetroShare Team   <retroshare.project@gmail.com>          *
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

#ifndef COMMENT_TEXT_H
#define COMMENT_TEXT_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QImage>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextImageFormat>
#include <QVBoxLayout>
#include "gui/common/RSTextBrowser.h"
#include "util/HandleRichText.h"

namespace CommentText
{
// Change only presentation dimensions; keep the embedded image data intact.
inline bool setPreview(QTextDocument *document, const QString &text, int width)
{
	document->setHtml(RsHtml().formatText(document, text,
	        RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));
	bool hasImages = false;
	for (QTextBlock block = document->begin(); block.isValid(); block = block.next())
		for (auto it = block.begin(); !it.atEnd(); ++it) {
			const QTextFragment fragment = it.fragment();
			if (!fragment.charFormat().isImageFormat()) continue;
			QTextImageFormat format = fragment.charFormat().toImageFormat();
			const QString source = format.name();
			if (!source.startsWith("data:image", Qt::CaseInsensitive)) continue;
			const int comma = source.indexOf(',');
			if (comma < 0) continue;
			const QImage image = QImage::fromData(
			        QByteArray::fromBase64(source.mid(comma + 1).toLatin1()));
			if (image.isNull()) continue;
			hasImages = true;
			QSize size = image.size();
			const QSize bounds(qMax(1, qMin(width, 240)), 180);
			if (size.width() > bounds.width() || size.height() > bounds.height())
				size.scale(bounds, Qt::KeepAspectRatio);
			format.setWidth(size.width());
			format.setHeight(size.height());
			QTextCursor cursor(document);
			cursor.setPosition(fragment.position());
			cursor.setPosition(fragment.position() + fragment.length(), QTextCursor::KeepAnchor);
			cursor.setCharFormat(format);
		}
	return hasImages;
}

inline void showFullSize(QWidget *parent, const QString &text)
{
	QDialog dialog(parent);
	dialog.setWindowTitle(QObject::tr("Full-size comment"));
	auto layout = new QVBoxLayout(&dialog);
	auto browser = new RSTextBrowser(&dialog);
	browser->setHtml(RsHtml().formatText(browser->document(), text,
	        RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));
	layout->addWidget(browser);
	auto buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
	QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
	layout->addWidget(buttons);
	dialog.resize(800, 600);
	dialog.exec();
}
}
#endif
