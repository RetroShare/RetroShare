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
#include <QFrame>
#include <QLabel>
#include <QDesktopServices>
#include <QUrl>
#include <QtMath>
#include <QScrollArea>
#include <QImage>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextImageFormat>
#include <QVBoxLayout>
#include "gui/common/RSTextBrowser.h"
#include "util/HandleRichText.h"
#include "util/RsMarkdown.h"
#include "util/stringutil.h"

namespace CommentText
{
// Comments are Markdown (rendered with cmark), legacy HTML or plain text.
// Change only presentation dimensions; keep the embedded image data intact.
inline bool setPreview(QTextDocument *document, const QString &text, int width, int maxHeight = 180)
{
	document->setHtml(RsHtml().formatText(document, text,
	        RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS | RSHTML_FORMATTEXT_MARKDOWN));
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
			const QSize bounds(qMax(1, qMin(width, 240)), qMax(1, maxHeight));
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

// Short rich text of a comment for news feed items (QLabel): the first
// maxLines lines, images scaled down to fit maxImageHeight. Each image is
// a "rsfullimage:<n>" link, see setFeedText().
inline void feedDocument(QTextDocument &document, const QString &text, const QFont &font, int maxLines, int maxImageHeight)
{
	document.setDefaultFont(font);
	document.setDocumentMargin(0); // as QLabel
	setPreview(&document, RsStringUtil::CopyLines(text, maxLines), 240, maxImageHeight);

	int imageIndex = 0;
	for (QTextBlock block = document.begin(); block.isValid(); block = block.next())
		for (auto it = block.begin(); !it.atEnd(); ++it) {
			const QTextFragment fragment = it.fragment();
			if (!fragment.charFormat().isImageFormat()) continue;
			QTextImageFormat format = fragment.charFormat().toImageFormat();
			if (!format.name().startsWith("data:image", Qt::CaseInsensitive)) continue;
			if (format.isAnchor()) { ++imageIndex; continue; } // image inside a link
			format.setAnchor(true);
			format.setAnchorHref(QString("rsfullimage:%1").arg(imageIndex++));
			format.setToolTip(QObject::tr("Click to view full size"));
			QTextCursor cursor(&document);
			cursor.setPosition(fragment.position());
			cursor.setPosition(fragment.position() + fragment.length(), QTextCursor::KeepAnchor);
			cursor.setCharFormat(format);
		}
}

inline QString feedHtml(const QString &text, const QFont &font, int maxLines, int maxImageHeight)
{
	QTextDocument document;
	feedDocument(document, text, font, maxLines, maxImageHeight);
	return document.toHtml();
}

inline void showFullSize(QWidget *parent, const QString &text, int imageIndex = 0);

// Show a comment in a feed item label. Clicking an image opens it in full
// size, other links are opened as with QLabel::openExternalLinks.
inline void setFeedText(QLabel *label, const QString &text, int maxLines, int maxImageHeight)
{
	QTextDocument document;
	feedDocument(document, text, label->font(), maxLines, maxImageHeight);

	label->setProperty("rsCommentText", text);
	label->setText(document.toHtml());

	// The feed list sizes its rows from the item size hint, which a word
	// wrapped label underestimates: reserve the height of the image previews.
	document.setTextWidth(qMax(label->width(), 300));
	label->setMinimumHeight(qCeil(document.size().height()));
	label->updateGeometry();

	if (!label->property("rsCommentLinks").toBool()) {
		label->setProperty("rsCommentLinks", true);
		label->setOpenExternalLinks(false);
		QObject::connect(label, &QLabel::linkActivated, label, [label](const QString &link) {
			if (link.startsWith("rsfullimage:"))
				showFullSize(label->window(), label->property("rsCommentText").toString(),
				             link.section(QLatin1Char(':'), 1).toInt());
			else
				QDesktopServices::openUrl(QUrl(link));
		});
	}
}

inline void showFullSize(QWidget *parent, const QString &text, int imageIndex)
{
	QDialog dialog(parent);
	dialog.setWindowTitle(QObject::tr("Full-size image"));
	auto layout = new QVBoxLayout(&dialog);
	QTextDocument document;
	document.setHtml(RsMarkdown::renderIfMarkdown(text));
	QImage image;
	int index = 0;
	for (QTextBlock block = document.begin(); block.isValid() && image.isNull(); block = block.next())
		for (auto it = block.begin(); !it.atEnd() && image.isNull(); ++it) {
			const QTextFragment fragment = it.fragment();
			if (!fragment.charFormat().isImageFormat()) continue;
			const QString source = fragment.charFormat().toImageFormat().name();
			const int comma = source.indexOf(',');
			if (!source.startsWith("data:image", Qt::CaseInsensitive) || comma < 0) continue;
			if (index++ != imageIndex) continue;
			image = QImage::fromData(QByteArray::fromBase64(source.mid(comma + 1).toLatin1()));
		}
	if (!image.isNull()) {
		auto scroll = new QScrollArea(&dialog);
		scroll->setWidgetResizable(true);
		scroll->setAlignment(Qt::AlignCenter);
		scroll->setStyleSheet("QScrollArea { background: white; border: 0; }");
		auto imageLabel = new QLabel;
		imageLabel->setAlignment(Qt::AlignCenter);
		imageLabel->setPixmap(QPixmap::fromImage(image));
		scroll->setWidget(imageLabel);
		layout->addWidget(scroll);
		dialog.resize(qMin(image.width() + 40, 1000), qMin(image.height() + 80, 800));
	} else {
		layout->addWidget(new QLabel(QObject::tr("No embedded image found"), &dialog));
		dialog.resize(400, 120);
	}
	auto buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
	QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
	layout->addWidget(buttons);
	dialog.exec();
}
}
#endif
