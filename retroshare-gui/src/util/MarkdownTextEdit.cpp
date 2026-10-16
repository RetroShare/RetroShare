/*******************************************************************************
 * util/MarkdownTextEdit.cpp                                                   *
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

#include "util/MarkdownTextEdit.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QFileInfo>
#include <QFontMetricsF>
#include <QImageReader>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMimeData>
#include <QRegularExpression>
#include <QStackedWidget>
#include <QTextBlock>
#include <QTextBrowser>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

#include <retroshare/rspeers.h>

#include "gui/RetroShareLink.h"
#include "gui/common/RSTextBrowser.h"
#include "gui/common/FilesDefs.h"
#include "gui/settings/rsharesettings.h"
#include "util/HandleRichText.h"
#include "util/MarkdownSyntaxHighlighter.h"
#include "util/RsMarkdown.h"
#include "util/misc.h"

static const int MAX_ALLOWED_GXS_MESSAGE_SIZE = 199000;
static const int MAX_EMBEDDED_IMAGE_PIXELS = 640 * 480;

/* Extract src of the <img> built by RsHtml::makeEmbeddedImage() */
static QString embeddedImageSource(const QString &imgHtml)
{
	static const QRegularExpression srcRe("src=\"([^\"]+)\"");
	QRegularExpressionMatch match = srcRe.match(imgHtml);
	return match.hasMatch() ? match.captured(1) : QString();
}

static bool isImageFile(const QString &path)
{
	return !path.isEmpty() && QImageReader(path).canRead();
}

/******************************************************************************
 * MarkdownPlainTextEdit
 ******************************************************************************/

MarkdownPlainTextEdit::MarkdownPlainTextEdit(QWidget *parent)
    : QPlainTextEdit(parent), mMaxImageBytes(MAX_ALLOWED_GXS_MESSAGE_SIZE - 200)
{
	setTabChangesFocus(false);
	mHighlighter = new MarkdownSyntaxHighlighter(document());
}

QColor MarkdownPlainTextEdit::textColorQuote() const
{
	return mHighlighter->textColorQuote();
}

QVariant MarkdownPlainTextEdit::textColorQuotes() const
{
	return mHighlighter->textColorQuotes();
}

void MarkdownPlainTextEdit::setTextColorQuote(QColor textColorQuote)
{
	mHighlighter->setTextColorQuote(textColorQuote);
}

void MarkdownPlainTextEdit::setTextColorQuotes(QVariant textColorQuotes)
{
	mHighlighter->setTextColorQuotes(textColorQuotes);
}

void MarkdownPlainTextEdit::insertMarkdown(const QString &markdown, bool block)
{
	QTextCursor cursor = textCursor();
	QString text = markdown;

	if (block) {
		if (!cursor.atBlockStart())
			text.prepend(QLatin1Char('\n'));
		text.append(QLatin1Char('\n'));
	}

	cursor.insertText(text);
	setTextCursor(cursor);
	ensureCursorVisible();
}

bool MarkdownPlainTextEdit::insertImage(const QImage &image)
{
	if (image.isNull())
		return false;

	QString imgHtml;
	if (!RsHtml::makeEmbeddedImage(image, imgHtml, MAX_EMBEDDED_IMAGE_PIXELS, mMaxImageBytes))
		return false;

	const QString source = embeddedImageSource(imgHtml);
	if (source.isEmpty())
		return false;

	insertMarkdown(RsMarkdown::makeImage(source, tr("image")), true);
	return true;
}

void MarkdownPlainTextEdit::pasteRetroShareLinks()
{
	QList<RetroShareLink> links;
	RSLinkClipboard::pasteLinks(links);

	QStringList markdown;
	for (const RetroShareLink &link : links)
		if (link.valid())
			markdown << RsMarkdown::makeLink(link.niceName(), link.toString());

	if (!markdown.isEmpty())
		insertMarkdown(markdown.join(QLatin1Char('\n')), markdown.size() > 1);
}

void MarkdownPlainTextEdit::pasteOwnCertificateLink()
{
	RetroShareLink link = RetroShareLink::createCertificate(rsPeers->getOwnId());
	if (link.valid())
		insertMarkdown(RsMarkdown::makeLink(link.niceName(), link.toString()) + QLatin1Char(' '));
}

bool MarkdownPlainTextEdit::canInsertFromMimeData(const QMimeData *source) const
{
	if (source->hasImage())
		return true;

	if (source->hasUrls()) {
		// Local files which are not images are left to the dialog (attachments)
		for (const QUrl &url : source->urls())
			if (!url.isLocalFile() || isImageFile(url.toLocalFile()))
				return true;
		return false;
	}

	return QPlainTextEdit::canInsertFromMimeData(source);
}

void MarkdownPlainTextEdit::insertFromMimeData(const QMimeData *source)
{
	if (source->hasImage()) {
		if (insertImage(qvariant_cast<QImage>(source->imageData())))
			return;
	}

	if (source->hasUrls()) {
		QStringList urls;
		bool handled = false;

		for (const QUrl &url : source->urls()) {
			if (url.isLocalFile()) {
				const QString path = url.toLocalFile();
				if (!isImageFile(path))
					continue;

				QString imgHtml;
				if (RsHtml::makeEmbeddedImage(path, imgHtml, MAX_EMBEDDED_IMAGE_PIXELS, mMaxImageBytes)) {
					const QString src = embeddedImageSource(imgHtml);
					if (!src.isEmpty()) {
						insertMarkdown(RsMarkdown::makeImage(src, url.fileName()), true);
						handled = true;
					}
				}
			} else {
				urls << url.toString();
			}
		}

		if (!urls.isEmpty()) {
			insertPlainText(urls.join(QLatin1Char('\n')));
			handled = true;
		}

		if (handled)
			return;
	}

	if (source->hasText()) {
		// Never keep rich text formatting: Markdown is plain text
		insertPlainText(source->text());
		return;
	}

	QPlainTextEdit::insertFromMimeData(source);
}

void MarkdownPlainTextEdit::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu *menu = createStandardContextMenu(event->pos());

	menu->addSeparator();
	QAction *pasteLinkAction = menu->addAction(FilesDefs::getIconFromQtResourcePath(":/images/pasterslink.png"),
	                                           tr("Paste RetroShare Link"), this, SLOT(pasteRetroShareLinks()));
	pasteLinkAction->setEnabled(!RSLinkClipboard::empty());
	menu->addAction(FilesDefs::getIconFromQtResourcePath(":/images/pasterslink.png"),
	                tr("Paste my certificate link"), this, SLOT(pasteOwnCertificateLink()));

	menu->exec(event->globalPos());
	delete menu;
}

void MarkdownPlainTextEdit::keyPressEvent(QKeyEvent *event)
{
	if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
	        && (event->modifiers() & ~Qt::KeypadModifier) == Qt::NoModifier
	        && continueList())
		return;

	QPlainTextEdit::keyPressEvent(event);
}

/* Enter at the end of a list item starts the next item, on an empty item ends the list */
bool MarkdownPlainTextEdit::continueList()
{
	static const QRegularExpression itemRe("^([ \\t]*)(?:([-*+])|(\\d{1,9})([.)]))([ \\t]+)(\\[[ xX]\\][ \\t]+)?(.*)$");

	QTextCursor cursor = textCursor();
	if (cursor.hasSelection())
		return false;

	const QString line = cursor.block().text();
	if (cursor.positionInBlock() != line.size())
		return false;

	QRegularExpressionMatch match = itemRe.match(line);
	if (!match.hasMatch())
		return false;

	if (match.captured(7).trimmed().isEmpty()) {
		// empty item: remove the marker and leave the list
		cursor.movePosition(QTextCursor::StartOfBlock);
		cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
		cursor.removeSelectedText();
		setTextCursor(cursor);
		return true;
	}

	const QString marker = match.captured(2).isEmpty()
	        ? QString::number(match.captured(3).toInt() + 1) + match.captured(4)
	        : match.captured(2);

	// a task list continues with an open task
	const QString task = match.captured(6).isEmpty() ? QString() : QStringLiteral("[ ] ");

	cursor.insertText(QLatin1Char('\n') + match.captured(1) + marker + match.captured(5) + task);
	setTextCursor(cursor);
	ensureCursorVisible();
	return true;
}

/******************************************************************************
 * MarkdownTextEdit
 ******************************************************************************/

MarkdownTextEdit::MarkdownTextEdit(QWidget *parent)
    : QWidget(parent), mMaxMessageSize(MAX_ALLOWED_GXS_MESSAGE_SIZE), mTextSizeOk(true)
    , mPreviewFlags(RSHTML_FORMATTEXT_EMBED_LINKS), mHardBreaks(true)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(2);

	/* toolbar */
	mToolBar = new QWidget(this);
	QHBoxLayout *toolLayout = new QHBoxLayout(mToolBar);
	toolLayout->setContentsMargins(0, 0, 0, 0);
	toolLayout->setSpacing(1);
	layout->addWidget(mToolBar);

	/* undo / redo, cut / copy / paste (as RichTextEdit). The editor handles
	 * the standard shortcuts itself, so the buttons get none. */
	mUndoButton = addButton(":/icons/textedit/undo.png", QString(), tr("Undo (Ctrl+Z)"));
	mRedoButton = addButton(":/icons/textedit/redo.png", QString(), tr("Redo (Ctrl+Y)"));
	toolLayout->addSpacing(6);
	mCutButton = addButton(":/icons/textedit/cut.png", QString(), tr("Cut (Ctrl+X)"));
	mCopyButton = addButton(":/icons/textedit/copy.png", QString(), tr("Copy (Ctrl+C)"));
	mPasteButton = addButton(":/icons/textedit/paste.png", QString(), tr("Paste (Ctrl+V)"));
	toolLayout->addSpacing(6);

	connect(addButton(":/icons/textedit/bold.png", QString(), tr("Bold (Ctrl+B)"), QKeySequence("Ctrl+B")),
	        SIGNAL(clicked()), this, SLOT(textBold()));
	connect(addButton(":/icons/textedit/italic.png", QString(), tr("Italic (Ctrl+I)"), QKeySequence("Ctrl+I")),
	        SIGNAL(clicked()), this, SLOT(textItalic()));

	QToolButton *headingButton = addButton(":/icons/textedit/heading.png", QString(), tr("Heading"));
	QMenu *headingMenu = new QMenu(headingButton);
	for (int level = 1; level <= 3; ++level) {
		QAction *action = headingMenu->addAction(tr("Heading %1").arg(level));
		connect(action, &QAction::triggered, this, [this, level]() { textHeading(level); });
	}
	headingMenu->addSeparator();
	connect(headingMenu->addAction(tr("Normal text")), &QAction::triggered, this, [this]() { textHeading(0); });
	headingButton->setMenu(headingMenu);
	headingButton->setPopupMode(QToolButton::InstantPopup);

	connect(addButton(":/icons/textedit/blockquote.png", QString(), tr("Quote")), SIGNAL(clicked()), this, SLOT(textQuote()));
	connect(addButton(":/icons/textedit/code.png", QString(), tr("Code")), SIGNAL(clicked()), this, SLOT(textCode()));
	connect(addButton(":/icons/textedit/numberd-list.png", QString(), tr("Numbered list")), SIGNAL(clicked()), this, SLOT(listOrdered()));
	connect(addButton(":/icons/textedit/bullet-list.png", QString(), tr("Bulleted list")), SIGNAL(clicked()), this, SLOT(listBullet()));
	connect(addButton(":/icons/textedit/tasklist.png", QString(), tr("Task list")), SIGNAL(clicked()), this, SLOT(listTask()));

	connect(addButton(":/icons/textedit/link.png", QString(), tr("Insert link (Ctrl+K)"), QKeySequence("Ctrl+K")),
	        SIGNAL(clicked()), this, SLOT(textLink()));
	mImageButton = addButton(":/icons/textedit/photo-of-a-landscape.png", QString(), tr("Insert image"));
	connect(mImageButton, SIGNAL(clicked()), this, SLOT(insertImage()));

	toolLayout->addStretch();

	QToolButton *helpButton = addButton(QString(), "?", QString());
	helpButton->setToolTip(tr("<b>Markdown</b><br>"
	                          "**bold** &nbsp; *italic* &nbsp; `code`<br>"
	                          "# Heading 1 &nbsp; ## Heading 2<br>"
	                          "- list item &nbsp; 1. numbered item<br>"
	                          "- [ ] task &nbsp; - [x] done task<br>"
	                          "&gt; quote<br>"
	                          "```<br>code block<br>```<br>"
	                          "[text](https://...) &nbsp; [text](retroshare://...)<br>"
	                          "![image](...) &nbsp; --- line"));
	helpButton->setFocusPolicy(Qt::NoFocus);

	mPreviewButton = addButton(QString(), tr("Preview"), tr("Show how the message will look"));
	mPreviewButton->setCheckable(true);
	connect(mPreviewButton, SIGNAL(toggled(bool)), this, SLOT(setPreviewVisible(bool)));

	/* editor and preview */
	mStack = new QStackedWidget(this);
	layout->addWidget(mStack, 1);

	mEditor = new MarkdownPlainTextEdit(mStack);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
	mEditor->setTabStopDistance(40);
#else
	mEditor->setTabStopWidth(40);
#endif
	mStack->addWidget(mEditor);

	// RSTextBrowser: rendered with the same quote colors as forum/channel posts
	RSTextBrowser *preview = new RSTextBrowser(mStack);
	preview->activateLinkClick(false); // don't open links while composing
	mPreview = preview;
	mStack->addWidget(mPreview);

	/* size info */
	mInfoLabel = new QLabel(this);
	mInfoLabel->setObjectName("f_info");
	layout->addWidget(mInfoLabel);

	connect(mEditor, SIGNAL(textChanged()), this, SLOT(checkLength()));

	connect(mUndoButton, SIGNAL(clicked()), mEditor, SLOT(undo()));
	connect(mRedoButton, SIGNAL(clicked()), mEditor, SLOT(redo()));
	connect(mCutButton, SIGNAL(clicked()), mEditor, SLOT(cut()));
	connect(mCopyButton, SIGNAL(clicked()), mEditor, SLOT(copy()));
	connect(mPasteButton, SIGNAL(clicked()), mEditor, SLOT(paste()));
	connect(mEditor->document(), SIGNAL(undoAvailable(bool)), this, SLOT(updateEditButtons()));
	connect(mEditor->document(), SIGNAL(redoAvailable(bool)), this, SLOT(updateEditButtons()));
	connect(mEditor, SIGNAL(copyAvailable(bool)), this, SLOT(updateEditButtons()));
	connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(updateEditButtons()));
	updateEditButtons();
	connect(mEditor, SIGNAL(textChanged()), this, SIGNAL(textChanged()));

	mMessageFontSizeHandler.registerFontSize(mEditor);
	mMessageFontSizeHandler.registerFontSize(mPreview);

	setFocusProxy(mEditor);
	checkLength();
}

QToolButton *MarkdownTextEdit::addButton(const QString &icon, const QString &text, const QString &toolTip, const QKeySequence &shortcut)
{
	QToolButton *button = new QToolButton(mToolBar);
	button->setAutoRaise(true);
	button->setFocusPolicy(Qt::NoFocus);
	button->setToolTip(toolTip);

	if (icon.isEmpty()) {
		button->setText(text);
		QFont font = button->font();
		font.setBold(true);
		button->setFont(font);
	} else {
		button->setIcon(FilesDefs::getIconFromQtResourcePath(icon));
		// 22 px (as RichTextEdit) for a 14 px font, scaled with the font / screen DPI
		const int iconSize = qRound(22 * QFontMetricsF(font()).height() / 14.0);
		button->setIconSize(QSize(iconSize, iconSize));
	}

	if (!shortcut.isEmpty())
		button->setShortcut(shortcut);

	mToolBar->layout()->addWidget(button);
	return button;
}

QString MarkdownTextEdit::toMarkdown() const
{
	return mEditor->toPlainText();
}

QString MarkdownTextEdit::toHtml() const
{
	return RsHtml().formatText(nullptr, RsMarkdown::toHtml(toMarkdown(), mHardBreaks), mPreviewFlags | RSHTML_FORMATTEXT_MARKDOWN);
}

bool MarkdownTextEdit::isEmpty() const
{
	return toMarkdown().trimmed().isEmpty();
}

bool MarkdownTextEdit::isModified() const
{
	return mEditor->document()->isModified();
}

void MarkdownTextEdit::setModified(bool modified)
{
	mEditor->document()->setModified(modified);
}

QTextDocument *MarkdownTextEdit::document() const
{
	return mEditor->document();
}

QTextCursor MarkdownTextEdit::textCursor() const
{
	return mEditor->textCursor();
}

void MarkdownTextEdit::setTextCursor(const QTextCursor &cursor)
{
	mEditor->setTextCursor(cursor);
}

void MarkdownTextEdit::setPlaceholderText(const QString &text)
{
	mEditor->setPlaceholderText(text);
}

void MarkdownTextEdit::setPlaceHolderTextPosted()
{
	setPlaceholderText(tr("Text (optional)"));
}

void MarkdownTextEdit::setMaxMessageSize(int bytes)
{
	mMaxMessageSize = bytes;
	mEditor->setMaxImageBytes(bytes - 200);
	checkLength();
}

void MarkdownTextEdit::setImageButtonVisible(bool visible)
{
	mImageButton->setVisible(visible);
}

void MarkdownTextEdit::setInfoVisible(bool visible)
{
	mInfoLabel->setVisible(visible);
}

void MarkdownTextEdit::setPreviewButtonVisible(bool visible)
{
	mPreviewButton->setVisible(visible);
}

bool MarkdownTextEdit::isPreviewVisible() const
{
	return mStack->currentWidget() == mPreview;
}

void MarkdownTextEdit::setText(const QString &text)
{
	setMarkdown(RsMarkdown::isHtml(text) ? RsMarkdown::fromHtml(text) : text);
}

void MarkdownTextEdit::setMarkdown(const QString &markdown)
{
	mEditor->setPlainText(markdown);
	if (isPreviewVisible())
		updatePreview();
}

void MarkdownTextEdit::clear()
{
	mEditor->clear();
}

void MarkdownTextEdit::insertMarkdown(const QString &markdown)
{
	setPreviewVisible(false);
	mEditor->insertMarkdown(markdown);
}

void MarkdownTextEdit::insertPlainText(const QString &text)
{
	setPreviewVisible(false);
	mEditor->insertPlainText(text);
}

void MarkdownTextEdit::insertLink(const QString &text, const QString &url)
{
	if (url.trimmed().isEmpty())
		return;

	setPreviewVisible(false);
	QTextCursor cursor = mEditor->textCursor();
	cursor.insertText(RsMarkdown::makeLink(text, url.trimmed()));
	mEditor->setTextCursor(cursor);
	mEditor->setFocus();
}

void MarkdownTextEdit::insertRetroShareLink(const RetroShareLink &link)
{
	if (link.valid())
		insertLink(link.niceName(), link.toString());
}

void MarkdownTextEdit::insertImage()
{
	QString file;
	if (!misc::getOpenFileName(window(), RshareSettings::LASTDIR_IMAGES, tr("Load Picture File"),
	                           "Pictures (*.png *.xpm *.jpg *.jpeg *.gif *.webp)", file))
		return;

	QString imgHtml;
	if (!RsHtml::makeEmbeddedImage(file, imgHtml, MAX_EMBEDDED_IMAGE_PIXELS, mMaxMessageSize - 200))
		return;

	const QString source = embeddedImageSource(imgHtml);
	if (source.isEmpty())
		return;

	setPreviewVisible(false);
	mEditor->insertMarkdown(RsMarkdown::makeImage(source, QFileInfo(file).completeBaseName()), true);
	mEditor->setFocus();
}

void MarkdownTextEdit::setPreviewVisible(bool visible)
{
	if (visible)
		updatePreview();

	mStack->setCurrentWidget(visible ? static_cast<QWidget*>(mPreview) : static_cast<QWidget*>(mEditor));

	// only the preview toggle stays usable while previewing
	for (QToolButton *button : mToolBar->findChildren<QToolButton*>())
		if (button != mPreviewButton && button->text() != "?")
			button->setEnabled(!visible);

	updateEditButtons();

	if (mPreviewButton->isChecked() != visible) {
		mPreviewButton->blockSignals(true);
		mPreviewButton->setChecked(visible);
		mPreviewButton->blockSignals(false);
	}

	if (!visible)
		mEditor->setFocus();
}

void MarkdownTextEdit::updateEditButtons()
{
	const bool editing = mStack->currentWidget() == mEditor;
	const bool selection = mEditor->textCursor().hasSelection();

	mUndoButton->setEnabled(editing && mEditor->document()->isUndoAvailable());
	mRedoButton->setEnabled(editing && mEditor->document()->isRedoAvailable());
	mCutButton->setEnabled(editing && selection && !mEditor->isReadOnly());
	mCopyButton->setEnabled(editing && selection);
	mPasteButton->setEnabled(editing && mEditor->canPaste());
}

void MarkdownTextEdit::updatePreview()
{
	const QString markdown = toMarkdown();
	if (markdown.trimmed().isEmpty()) {
		mPreview->setHtml("<i>" + tr("Nothing to preview") + "</i>");
		return;
	}

	mPreview->setHtml(RsHtml().formatText(mPreview->document(), RsMarkdown::toHtml(markdown, mHardBreaks),
	                                      mPreviewFlags | RSHTML_FORMATTEXT_MARKDOWN));
}

void MarkdownTextEdit::focusInEvent(QFocusEvent *)
{
	(isPreviewVisible() ? static_cast<QWidget*>(mPreview) : static_cast<QWidget*>(mEditor))->setFocus(Qt::TabFocusReason);
}

void MarkdownTextEdit::checkLength()
{
	const int charRemains = mMaxMessageSize - toMarkdown().toUtf8().size();
	mTextSizeOk = charRemains >= 0;

	if (mTextSizeOk) {
		mInfoLabel->setText(tr("It remains %1 characters.").arg(charRemains));
		mInfoLabel->setStyleSheet("QLabel#f_info { }");
	} else {
		mInfoLabel->setText(tr("Warning: This message is too big of %1 characters.").arg(-charRemains));
		mInfoLabel->setStyleSheet("QLabel#f_info {color: red; font: bold; }");
	}

	emit textSizeOk(mTextSizeOk);
}

/* formatting **************************************************************/

void MarkdownTextEdit::wrapSelection(const QString &prefix, const QString &suffix, const QString &placeholder)
{
	QTextCursor cursor = mEditor->textCursor();
	cursor.beginEditBlock();

	QString selected = cursor.selectedText();
	selected.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));

	if (!selected.trimmed().isEmpty()) {
		if (selected.size() >= prefix.size() + suffix.size() && selected.startsWith(prefix) && selected.endsWith(suffix)) {
			// already formatted: remove
			cursor.insertText(selected.mid(prefix.size(), selected.size() - prefix.size() - suffix.size()));
		} else {
			// markers must be next to the text, keep spaces outside
			int lead = 0;
			while (lead < selected.size() && selected.at(lead).isSpace())
				++lead;
			int trail = 0;
			while (trail < selected.size() - lead && selected.at(selected.size() - 1 - trail).isSpace())
				++trail;

			cursor.insertText(selected.left(lead) + prefix + selected.mid(lead, selected.size() - lead - trail)
			                  + suffix + selected.right(trail));
		}
	} else {
		const int position = cursor.position();
		cursor.insertText(prefix + placeholder + suffix);
		cursor.setPosition(position + prefix.size());
		cursor.setPosition(position + prefix.size() + placeholder.size(), QTextCursor::KeepAnchor);
	}

	cursor.endEditBlock();
	mEditor->setTextCursor(cursor);
	mEditor->setFocus();
}

void MarkdownTextEdit::toggleLinePrefix(LinePrefix type, int headingLevel)
{
	static const QRegularExpression quoteRe("^ {0,3}> ?");
	static const QRegularExpression listRe("^ {0,3}([-*+]|\\d{1,9}[.)])[ \\t]+");
	static const QRegularExpression bulletRe("^ {0,3}[-*+][ \\t]+(?!\\[[ xX]\\](?:[ \\t]|$))");
	static const QRegularExpression taskRe("^ {0,3}[-*+][ \\t]+\\[[ xX]\\](?:[ \\t]+|$)");
	static const QRegularExpression orderedRe("^ {0,3}\\d{1,9}[.)][ \\t]+");
	static const QRegularExpression headingRe("^ {0,3}#{1,6}[ \\t]+");

	QRegularExpression ownRe;
	switch (type) {
	case PrefixQuote:   ownRe = quoteRe;   break;
	case PrefixBullet:  ownRe = bulletRe;  break;
	case PrefixOrdered: ownRe = orderedRe; break;
	case PrefixTask:    ownRe = taskRe;    break;
	case PrefixHeading:
		ownRe = headingLevel > 0 ? QRegularExpression(QString("^ {0,3}#{%1}[ \\t]+").arg(headingLevel)) : headingRe;
		break;
	}

	QTextCursor cursor = mEditor->textCursor();
	QTextDocument *doc = mEditor->document();

	QTextBlock first = doc->findBlock(cursor.selectionStart());
	QTextBlock last = doc->findBlock(cursor.selectionEnd());
	if (cursor.hasSelection() && last.blockNumber() > first.blockNumber() && cursor.selectionEnd() == last.position())
		last = last.previous();
	const int lastNumber = last.blockNumber();
	const bool singleLine = first.blockNumber() == lastNumber;

	// remove the prefix when every (non empty) line already has it
	bool remove = (type == PrefixHeading && headingLevel == 0);
	if (!remove) {
		remove = true;
		for (QTextBlock block = first; block.isValid() && block.blockNumber() <= lastNumber; block = block.next())
			if (!block.text().trimmed().isEmpty() && !ownRe.match(block.text()).hasMatch())
				remove = false;
	}

	cursor.beginEditBlock();

	int number = 1;
	for (QTextBlock block = first; block.isValid() && block.blockNumber() <= lastNumber; block = block.next()) {
		QTextCursor blockCursor(block);
		const QString text = block.text();

		auto removePrefix = [&](const QRegularExpression &re) {
			QRegularExpressionMatch match = re.match(text);
			if (match.hasMatch()) {
				blockCursor.setPosition(block.position());
				blockCursor.setPosition(block.position() + match.capturedLength(), QTextCursor::KeepAnchor);
				blockCursor.removeSelectedText();
			}
		};

		if (remove) {
			removePrefix(ownRe);
			continue;
		}

		if (text.trimmed().isEmpty() && !singleLine)
			continue;

		QString prefix;
		switch (type) {
		case PrefixQuote:
			prefix = "> ";
			break;
		case PrefixBullet:
			removePrefix(taskRe.match(text).hasMatch() ? taskRe : listRe);
			prefix = "- ";
			break;
		case PrefixOrdered:
			removePrefix(taskRe.match(text).hasMatch() ? taskRe : listRe);
			prefix = QString("%1. ").arg(number++);
			break;
		case PrefixTask:
			removePrefix(taskRe.match(text).hasMatch() ? taskRe : listRe);
			// keep the state of existing tasks
			prefix = text.contains(QRegularExpression("^ {0,3}[-*+][ \\t]+\\[[xX]\\]")) ? "- [x] " : "- [ ] ";
			break;
		case PrefixHeading:
			removePrefix(headingRe);
			prefix = QString(headingLevel, QLatin1Char('#')) + QLatin1Char(' ');
			break;
		}

		blockCursor.setPosition(block.position());
		blockCursor.insertText(prefix);
	}

	cursor.endEditBlock();
	mEditor->setFocus();
}

void MarkdownTextEdit::textBold()
{
	wrapSelection("**", "**", tr("bold text"));
}

void MarkdownTextEdit::textItalic()
{
	wrapSelection("*", "*", tr("italic text"));
}

void MarkdownTextEdit::textCode()
{
	QTextCursor cursor = mEditor->textCursor();
	QString selected = cursor.selectedText();

	const bool block = selected.contains(QChar::ParagraphSeparator)
	        || (!cursor.hasSelection() && cursor.block().text().trimmed().isEmpty());

	if (!block) {
		wrapSelection("`", "`", tr("code"));
		return;
	}

	selected.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));

	QTextCursor start(cursor);
	start.setPosition(cursor.selectionStart());

	QString text = "```\n" + selected + "\n```";
	if (!start.atBlockStart())
		text.prepend(QLatin1Char('\n'));

	cursor.beginEditBlock();
	cursor.insertText(text);
	if (selected.isEmpty())
		cursor.movePosition(QTextCursor::Up); // inside the empty block
	cursor.endEditBlock();

	mEditor->setTextCursor(cursor);
	mEditor->setFocus();
}

void MarkdownTextEdit::textQuote()
{
	toggleLinePrefix(PrefixQuote);
}

void MarkdownTextEdit::textHeading(int level)
{
	toggleLinePrefix(PrefixHeading, level);
}

void MarkdownTextEdit::listBullet()
{
	toggleLinePrefix(PrefixBullet);
}

void MarkdownTextEdit::listTask()
{
	toggleLinePrefix(PrefixTask);
}

void MarkdownTextEdit::listOrdered()
{
	toggleLinePrefix(PrefixOrdered);
}


void MarkdownTextEdit::textLink()
{
	QTextCursor cursor = mEditor->textCursor();
	QString selected = cursor.selectedText();
	selected.replace(QChar::ParagraphSeparator, QLatin1Char(' '));

	static const QRegularExpression urlRe("^(https?|retroshare)://\\S+$", QRegularExpression::CaseInsensitiveOption);

	QDialog dialog(this);
	dialog.setWindowTitle(tr("Insert link"));

	QFormLayout *form = new QFormLayout(&dialog);
	QLineEdit *textEdit = new QLineEdit(&dialog);
	QLineEdit *urlEdit = new QLineEdit(&dialog);
	urlEdit->setPlaceholderText("https://...  retroshare://...");
	urlEdit->setMinimumWidth(350);

	const QString clipboard = QApplication::clipboard()->text().trimmed();
	if (urlRe.match(selected.trimmed()).hasMatch()) {
		urlEdit->setText(selected.trimmed());
	} else {
		textEdit->setText(selected);
		if (urlRe.match(clipboard).hasMatch())
			urlEdit->setText(clipboard);
	}

	form->addRow(tr("Text:"), textEdit);
	form->addRow(tr("URL:"), urlEdit);

	QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
	connect(buttons, SIGNAL(accepted()), &dialog, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), &dialog, SLOT(reject()));
	form->addRow(buttons);

	(textEdit->text().isEmpty() && !urlEdit->text().isEmpty() ? textEdit : urlEdit)->setFocus();

	if (dialog.exec() != QDialog::Accepted)
		return;

	mEditor->setTextCursor(cursor); // keep selection, it gets replaced
	insertLink(textEdit->text(), urlEdit->text());
}
