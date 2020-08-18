/*******************************************************************************
 * gui/common/MimeTextEdit.cpp                                                 *
 *                                                                             *
 * Copyright (C) 2012, Retroshare Team <retroshare.project@gmail.com>          *
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

#include "gui/common/FilesDefs.h"
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QTextDocumentFragment>
#include <QCompleter>
#include <QAbstractItemView>
#include <QKeyEvent>
#include <QScrollBar>
#include <QMenu>

#include "MimeTextEdit.h"
#include "util/HandleRichText.h"
#include "gui/RetroShareLink.h"

#include <retroshare/rspeers.h>

MimeTextEdit::MimeTextEdit(QWidget *parent)
    : RSTextEdit(parent), mCompleter(0)
{
	mCompleterKeyModifiers = Qt::ControlModifier;
	mCompleterKey = Qt::Key_Space;
	mForceCompleterShowNextKeyEvent = false;
	highliter = new RsSyntaxHighlighter(this);
	mOnlyPlainText = false;
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

	return RSTextEdit::canInsertFromMimeData(source);
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
				if (RsHtml::makeEmbeddedImage(image, encodedImage, 640*480, mMaxBytes)) {
					QTextDocumentFragment fragment = QTextDocumentFragment::fromHtml(encodedImage);
					textCursor().insertFragment(fragment);
					return;
				}
			}
		}
	}
#endif
	if (source == NULL) return;

	//insert retroshare links
	QList<RetroShareLink> links;
	RSLinkClipboard::parseText(source->text(), links);
	if(links.size() > 0)
	{
		for(int i = 0; i < links.size(); ++i)
			if (mOnlyPlainText)
				insertPlainText(links[i].toString());
			else
				insertHtml(links[i].toHtml() + "<br>");

		return;
	}

	return RSTextEdit::insertFromMimeData(source);
}

void MimeTextEdit::setCompleter(QCompleter *completer)
{
	if (mCompleter)
		QObject::disconnect(mCompleter, 0, this, 0);

	mCompleter = completer;

	if (!mCompleter)
		return;

	mCompleter->setWidget(this);
	mCompleter->setCompletionMode(QCompleter::PopupCompletion);
	mCompleter->setCaseSensitivity(Qt::CaseInsensitive);
	QObject::connect(mCompleter, SIGNAL(activated(QString)), this, SLOT(insertCompletion(QString)));
}

QCompleter *MimeTextEdit::completer() const
{
	return mCompleter;
}

void MimeTextEdit::insertCompletion(const QString& completion)
{
	if (mCompleter->widget() != this)
		return;

	QTextCursor tc = textCursor();
	if (mCompleter->completionPrefix().length() > 0) {
		tc.movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
	}
	tc.removeSelectedText();
	tc.insertText(mCompleterStartString+completion);
	mCompleterStartString.clear();
	setTextCursor(tc);
}

QString MimeTextEdit::textUnderCursor() const
{
	QTextCursor tc = textCursor();
	tc.select(QTextCursor::WordUnderCursor);
	return tc.selectedText();
}

void MimeTextEdit::focusInEvent(QFocusEvent *e)
{
	if (mCompleter)
		mCompleter->setWidget(this);

	RSTextEdit::focusInEvent(e);
}

void MimeTextEdit::keyPressEvent(QKeyEvent *e)
{
	if (mCompleter && mCompleter->popup()->isVisible()) {
		// The following keys are forwarded by the completer to the widget
		switch (e->key()) {
		case Qt::Key_Enter:
		case Qt::Key_Return:
		case Qt::Key_Escape:
		case Qt::Key_Tab:
		case Qt::Key_Backtab:
			mCompleter->popup()->hide();
			mForceCompleterShowNextKeyEvent=false;
			e->ignore();
			return; // let the completer do default behavior
		default:
			break;
		}
	}

	bool isShortcut = ((e->modifiers() & mCompleterKeyModifiers) && e->key() == mCompleterKey);
	if (isShortcut && !mForceCompleterShowNextKeyEvent) {
		mCompleterStartString.clear();
	}
	isShortcut |= mForceCompleterShowNextKeyEvent;
	if (!mCompleter || !isShortcut) // do not process the shortcut when we have a completer
		RSTextEdit::keyPressEvent(e);

	if (!mCompleter) return; //Nothing else to do if not mCompleter initialized

	if (!isShortcut && (mCompleter && !mCompleter->popup()->isVisible())) {
		return;
	}

	if (!mForceCompleterShowNextKeyEvent) {
		static QString eow(" ~!@#$%^&*()_+{}|:\"<>?,./;'[]\\-="); // end of word
		if (!isShortcut && !e->text().isEmpty() && eow.contains(e->text())){
			mCompleter->popup()->hide();
			return;
		}
	}

	QString completionPrefix = textUnderCursor();
	if (completionPrefix != mCompleter->completionPrefix()) {
		mCompleter->setCompletionPrefix(completionPrefix);
		mCompleter->popup()->setCurrentIndex(mCompleter->completionModel()->index(0, 0));
	}

	QRect cr = cursorRect();
	cr.setWidth(mCompleter->popup()->sizeHintForColumn(0) + mCompleter->popup()->verticalScrollBar()->sizeHint().width());
	mCompleter->complete(cr); // popup it up!

	if (mCompleter->completionCount()==0 && isShortcut){
		RSTextEdit::keyPressEvent(e);// Process the key if no match
	}
	mForceCompleterShowNextKeyEvent = false;
}

void MimeTextEdit::setCompleterKeyModifiers(Qt::KeyboardModifier modifiers)
{
	mCompleterKeyModifiers = modifiers;
}

Qt::KeyboardModifier MimeTextEdit::getCompleterKeyModifiers() const
{
	return mCompleterKeyModifiers;
}

void MimeTextEdit::setCompleterKey(Qt::Key key)
{
	mCompleterKey = key;
}

Qt::Key MimeTextEdit::getCompleterKey() const
{
	return mCompleterKey;
}

void MimeTextEdit::forceCompleterShowNextKeyEvent(QString startString)
{
	if (!mCompleter) return; //Nothing else to do if not mCompleter initialized

	if(!mCompleter->popup()->isVisible()){
		mForceCompleterShowNextKeyEvent = true;
		mCompleterStartString = startString;
	}
}

void MimeTextEdit::addContextMenuAction(QAction *action)
{
	mContextMenuActions.push_back(action);
}

void MimeTextEdit::contextMenuEvent(QContextMenuEvent *e)
{
	emit calculateContextMenuActions();

	QMenu *contextMenu = createStandardContextMenu(e->pos());

	/* Add actions for pasting links */
	contextMenu->addAction( tr("Paste as plain text"), this, SLOT(pastePlainText()));
	QAction *spoilerAction =  contextMenu->addAction(tr("Spoiler"), this, SLOT(spoiler()));
	spoilerAction->setToolTip(tr("Select text to hide, then push this button"));
	contextMenu->addSeparator();
    QAction *pasteLinkAction = contextMenu->addAction(FilesDefs::getIconFromQtResourcePath(":/images/pasterslink.png"), tr("Paste RetroShare Link"), this, SLOT(pasteLink()));
    contextMenu->addAction(FilesDefs::getIconFromQtResourcePath(":/images/pasterslink.png"), tr("Paste my certificate link"), this, SLOT(pasteOwnCertificateLink()));

	if (RSLinkClipboard::empty()) {
		pasteLinkAction->setDisabled(true);
	}

	QList<QAction*>::iterator it;
	for (it = mContextMenuActions.begin(); it != mContextMenuActions.end(); ++it) {
		contextMenu->addAction(*it);
	}

	contextMenu->exec(QCursor::pos());

	delete(contextMenu);
}

void MimeTextEdit::pasteLink()
{
	insertHtml(RSLinkClipboard::toHtml()) ;
}

void MimeTextEdit::pasteOwnCertificateLink()
{
	RsPeerId ownId = rsPeers->getOwnId();
	RetroShareLink link = RetroShareLink::createCertificate(ownId);

	if (link.valid()) {
		insertHtml(link.toHtml() + " ");
	}
}

void MimeTextEdit::pastePlainText()
{
	insertPlainText(QApplication::clipboard()->text().remove(QChar(-4)));//Char used when image on text.
}

void MimeTextEdit::spoiler()
{
	RsHtml::insertSpoilerText(this->textCursor());
}
