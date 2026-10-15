/*******************************************************************************
 * util/MarkdownTextEdit.h                                                     *
 *                                                                             *
 * Copyright (c) 2026 Retroshare Team <retroshare.project@gmail.com>           *
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

#ifndef MARKDOWNTEXTEDIT_H
#define MARKDOWNTEXTEDIT_H

#include <QKeySequence>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QWidget>

#include "util/FontSizeHandler.h"

class MarkdownSyntaxHighlighter;
class QImage;
class QLabel;
class QStackedWidget;
class QTextBrowser;
class QToolButton;
class RetroShareLink;

/**
 * @brief Plain text editor for Markdown sources.
 *
 * Pasted / dropped content is inserted as Markdown:
 *  - images (clipboard image, image files) are embedded as
 *    ![image](data:image/...;base64,...)
 *  - URLs and RetroShare links are inserted as text / Markdown links
 *  - rich text is inserted as plain text
 */
class MarkdownPlainTextEdit : public QPlainTextEdit
{
	Q_OBJECT

	// quote colors from the stylesheet, as for RSTextBrowser and MimeTextEdit
	Q_PROPERTY(QColor textColorQuote READ textColorQuote WRITE setTextColorQuote)
	Q_PROPERTY(QVariant textColorQuotes READ textColorQuotes WRITE setTextColorQuotes)

public:
	explicit MarkdownPlainTextEdit(QWidget *parent = nullptr);

	QColor textColorQuote() const;
	QVariant textColorQuotes() const;

	/** Maximum size of an embedded image (bytes, base64 encoded) */
	void setMaxImageBytes(int bytes) { mMaxImageBytes = bytes; }

	/** Embed image at cursor position. Returns false when it can't be encoded */
	bool insertImage(const QImage &image);

	/** Insert a Markdown snippet at cursor, on its own line(s) when block = true */
	void insertMarkdown(const QString &markdown, bool block = false);

public slots:
	void pasteRetroShareLinks();
	void pasteOwnCertificateLink();
	void setTextColorQuote(QColor textColorQuote);
	void setTextColorQuotes(QVariant textColorQuotes);

protected:
	bool canInsertFromMimeData(const QMimeData *source) const override;
	void insertFromMimeData(const QMimeData *source) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;

private:
	bool continueList();

	int mMaxImageBytes;
	MarkdownSyntaxHighlighter *mHighlighter;
};

/**
 * @brief Markdown (CommonMark) composer with toolbar and preview.
 *
 * Reusable replacement of RichTextEdit for GXS composers (channels, forums,
 * boards...). The message is written and stored as Markdown text; it is
 * rendered with libcmark (see RsMarkdown) by RsHtml::formatText() with the
 * RSHTML_FORMATTEXT_MARKDOWN flag.
 *
 * Old HTML messages given to setText() are converted to Markdown.
 */
class MarkdownTextEdit : public QWidget
{
	Q_OBJECT

public:
	explicit MarkdownTextEdit(QWidget *parent = nullptr);

	/** Markdown source: this is what should be stored in the message */
	QString toMarkdown() const;
	/** Same as toMarkdown() (compatibility with RichTextEdit / QTextEdit) */
	QString toPlainText() const { return toMarkdown(); }
	/** Rendered HTML, as it will be displayed */
	QString toHtml() const;

	bool isEmpty() const;
	bool isModified() const;
	void setModified(bool modified);

	QTextDocument *document() const;
	QTextCursor textCursor() const;
	void setTextCursor(const QTextCursor &cursor);
	MarkdownPlainTextEdit *editor() const { return mEditor; }

	void setPlaceholderText(const QString &text);
	void setPlaceHolderTextPosted();

	/** Maximum message size (bytes, UTF-8). Default: GXS message limit */
	void setMaxMessageSize(int bytes);
	bool isTextSizeOk() const { return mTextSizeOk; }

	/** RSHTML_FORMATTEXT_* flags used by preview (default: embed links) */
	void setPreviewFlags(unsigned long flags) { mPreviewFlags = flags; }

	/** Hide the insert image button when the dialog has its own */
	void setImageButtonVisible(bool visible);
	/** Hide the "remaining characters" label when the dialog shows its own */
	void setInfoVisible(bool visible);

	/** Hide the Preview button when the dialog drives setPreviewVisible() itself */
	void setPreviewButtonVisible(bool visible);

	/** Keep single line breaks (posts, default) or use standard Markdown (wiki) */
	void setHardBreaks(bool hardBreaks) { mHardBreaks = hardBreaks; }
	bool hardBreaks() const { return mHardBreaks; }

	bool isPreviewVisible() const;

	/** Insert [name](retroshare://...) */
	void insertRetroShareLink(const RetroShareLink &link);

signals:
	void textChanged();
	void textSizeOk(bool ok);

public slots:
	/** Set Markdown, plain text or legacy HTML (converted to Markdown) */
	void setText(const QString &text);
	void setMarkdown(const QString &markdown);
	void insertMarkdown(const QString &markdown);
	void insertPlainText(const QString &text);
	void insertLink(const QString &text, const QString &url);
	/** Ask for an image file and embed it */
	void insertImage();
	void setPreviewVisible(bool visible);
	void clear();

protected:
	void focusInEvent(QFocusEvent *event) override;

private slots:
	void textBold();
	void textItalic();
	void textCode();
	void textQuote();
	void textHeading(int level);
	void listBullet();
	void listOrdered();
	void insertRule();
	void textLink();
	void checkLength();
	void updatePreview();

private:
	enum LinePrefix { PrefixQuote, PrefixBullet, PrefixOrdered, PrefixHeading };

	QToolButton *addButton(const QString &icon, const QString &text, const QString &toolTip, const QKeySequence &shortcut = QKeySequence());
	void wrapSelection(const QString &prefix, const QString &suffix, const QString &placeholder);
	void toggleLinePrefix(LinePrefix type, int headingLevel = 0);

	MarkdownPlainTextEdit *mEditor;
	QTextBrowser *mPreview;
	QStackedWidget *mStack;
	QWidget *mToolBar;
	QToolButton *mImageButton;
	QToolButton *mPreviewButton;
	QLabel *mInfoLabel;

	int mMaxMessageSize;
	bool mTextSizeOk;
	unsigned long mPreviewFlags;
	bool mHardBreaks;

	MessageFontSizeHandler mMessageFontSizeHandler;
};

#endif // MARKDOWNTEXTEDIT_H
