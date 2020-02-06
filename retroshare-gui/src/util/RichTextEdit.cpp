/*******************************************************************************
 * util/RichTextEdit.cpp                                                       *
 *                                                                             *
 * Copyright (c) 2019 Retroshare Team <retroshare.project@gmail.com>           *
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

#include "RichTextEdit.h"
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QFontDatabase>
#include <QInputDialog>
#include <QColorDialog>
#include <QTextList>
#include <QtDebug>
#include <QFileDialog>
#include <QImageReader>
#include <QSettings>
#include <QBuffer>
#include <QUrl>
#include <QPlainTextEdit>
#include <QMenu>
#include <QDialog>
#include <QTextDocumentFragment>

#include "util/misc.h"
#include "util/HandleRichText.h"

static const uint32_t MAX_ALLOWED_GXS_MESSAGE_SIZE = 199000;

RichTextEdit::RichTextEdit(QWidget *parent) : QWidget(parent) {
    setupUi(this);
    m_lastBlockList = 0;
    f_textedit->setTabStopWidth(40);

    connect(f_textedit, SIGNAL(currentCharFormatChanged(QTextCharFormat)),
            this,     SLOT(slotCurrentCharFormatChanged(QTextCharFormat)));
    connect(f_textedit, SIGNAL(cursorPositionChanged()),
            this,     SLOT(slotCursorPositionChanged()));

    m_fontsize_h1 = 18;
    m_fontsize_h2 = 16;
    m_fontsize_h3 = 14;
    m_fontsize_h4 = 12;

    fontChanged(f_textedit->font());
    bgColorChanged(f_textedit->textColor());

    // paragraph formatting

    m_paragraphItems    << tr("Standard")
                        << tr("Heading 1")
                        << tr("Heading 2")
                        << tr("Heading 3")
                        << tr("Heading 4")
                        << tr("Monospace");
    f_paragraph->addItems(m_paragraphItems);

    connect(f_paragraph, SIGNAL(activated(int)),
            this, SLOT(textStyle(int)));

    // undo & redo

    f_undo->setShortcut(QKeySequence::Undo);
    f_redo->setShortcut(QKeySequence::Redo);

    connect(f_textedit->document(), SIGNAL(undoAvailable(bool)),
            f_undo, SLOT(setEnabled(bool)));
    connect(f_textedit->document(), SIGNAL(redoAvailable(bool)),
            f_redo, SLOT(setEnabled(bool)));

    f_undo->setEnabled(f_textedit->document()->isUndoAvailable());
    f_redo->setEnabled(f_textedit->document()->isRedoAvailable());

    connect(f_undo, SIGNAL(clicked()), f_textedit, SLOT(undo()));
    connect(f_redo, SIGNAL(clicked()), f_textedit, SLOT(redo()));

    // cut, copy & paste

    f_cut->setShortcut(QKeySequence::Cut);
    f_copy->setShortcut(QKeySequence::Copy);
    f_paste->setShortcut(QKeySequence::Paste);

    f_cut->setEnabled(false);
    f_copy->setEnabled(false);

    connect(f_cut, SIGNAL(clicked()), f_textedit, SLOT(cut()));
    connect(f_copy, SIGNAL(clicked()), f_textedit, SLOT(copy()));
    connect(f_paste, SIGNAL(clicked()), f_textedit, SLOT(paste()));

    connect(f_textedit, SIGNAL(copyAvailable(bool)), f_cut, SLOT(setEnabled(bool)));
    connect(f_textedit, SIGNAL(copyAvailable(bool)), f_copy, SLOT(setEnabled(bool)));

#ifndef QT_NO_CLIPBOARD
    connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(slotClipboardDataChanged()));
#endif

    // link

    f_link->setShortcut(Qt::CTRL + Qt::Key_L);

    connect(f_link, SIGNAL(clicked(bool)), this, SLOT(textLink(bool)));

    // bold, italic & underline

    f_bold->setShortcut(Qt::CTRL + Qt::Key_B);
    f_italic->setShortcut(Qt::CTRL + Qt::Key_I);
    f_underline->setShortcut(Qt::CTRL + Qt::Key_U);

    connect(f_bold, SIGNAL(clicked()), this, SLOT(textBold()));
    connect(f_italic, SIGNAL(clicked()), this, SLOT(textItalic()));
    connect(f_underline, SIGNAL(clicked()), this, SLOT(textUnderline()));
    connect(f_strikeout, SIGNAL(clicked()), this, SLOT(textStrikeout()));

    QAction *removeFormat = new QAction(tr("Remove character formatting"), this);
    removeFormat->setShortcut(QKeySequence("CTRL+M"));
    connect(removeFormat, SIGNAL(triggered()), this, SLOT(textRemoveFormat()));
    f_textedit->addAction(removeFormat);

    QAction *removeAllFormat = new QAction(tr("Remove all formatting"), this);
    connect(removeAllFormat, SIGNAL(triggered()), this, SLOT(textRemoveAllFormat()));
    f_textedit->addAction(removeAllFormat);

    QAction *textsource = new QAction(tr("Edit document source"), this);
    textsource->setShortcut(QKeySequence("CTRL+O"));
    connect(textsource, SIGNAL(triggered()), this, SLOT(textSource()));
    f_textedit->addAction(textsource);

    QMenu *menu = new QMenu(this);
    menu->addAction(removeAllFormat);
    menu->addAction(removeFormat);
    menu->addAction(textsource);
    f_menu->setMenu(menu);
    f_menu->setPopupMode(QToolButton::InstantPopup);

    // lists

    f_list_bullet->setShortcut(Qt::CTRL + Qt::Key_Minus);
    f_list_ordered->setShortcut(Qt::CTRL + Qt::Key_Equal);

    connect(f_list_bullet, SIGNAL(clicked(bool)), this, SLOT(listBullet(bool)));
    connect(f_list_ordered, SIGNAL(clicked(bool)), this, SLOT(listOrdered(bool)));

    // indentation

    f_indent_dec->setShortcut(Qt::CTRL + Qt::Key_Comma);
    f_indent_inc->setShortcut(Qt::CTRL + Qt::Key_Period);

    connect(f_indent_inc, SIGNAL(clicked()), this, SLOT(increaseIndentation()));
    connect(f_indent_dec, SIGNAL(clicked()), this, SLOT(decreaseIndentation()));

    // font size

    QFontDatabase db;
    foreach(int size, db.standardSizes())
        f_fontsize->addItem(QString::number(size));

    connect(f_fontsize, SIGNAL(activated(QString)),
            this, SLOT(textSize(QString)));
    f_fontsize->setCurrentIndex(f_fontsize->findText(QString::number(QApplication::font()
                                                                   .pointSize())));

    // text foreground color

    QPixmap pix(16, 16);
    pix.fill(QApplication::palette().foreground().color());
    f_fgcolor->setIcon(pix);

    connect(f_fgcolor, SIGNAL(clicked()), this, SLOT(textFgColor()));

    // text background color

    pix.fill(QApplication::palette().background().color());
    f_bgcolor->setIcon(pix);

    connect(f_bgcolor, SIGNAL(clicked()), this, SLOT(textBgColor()));

    // images
    connect(f_image, SIGNAL(clicked()), this, SLOT(insertImage()));

	// check message length
	connect(f_textedit, SIGNAL(textChanged()), this, SLOT(checkLength()));

}


void RichTextEdit::textSource() {
    QDialog *dialog = new QDialog(this);
    QPlainTextEdit *pte = new QPlainTextEdit(dialog);
    pte->setPlainText( f_textedit->toHtml() );
    QGridLayout *gl = new QGridLayout(dialog);
    gl->addWidget(pte,0,0,1,1);
    dialog->setWindowTitle(tr("Document source"));
    dialog->setMinimumWidth (400);
    dialog->setMinimumHeight(600);
    dialog->exec();

    f_textedit->setHtml(pte->toPlainText());

    delete dialog;
}


void RichTextEdit::textRemoveFormat() {
    QTextCharFormat fmt;
    fmt.setFontWeight(QFont::Normal);
    fmt.setFontUnderline  (false);
    fmt.setFontStrikeOut  (false);
    fmt.setFontItalic     (false);
    fmt.setFontPointSize  (9);
//  fmt.setFontFamily     ("Helvetica");
//  fmt.setFontStyleHint  (QFont::SansSerif);
//  fmt.setFontFixedPitch (true);

    f_bold      ->setChecked(false);
    f_underline ->setChecked(false);
    f_italic    ->setChecked(false);
    f_strikeout ->setChecked(false);
    f_fontsize  ->setCurrentIndex(f_fontsize->findText("9"));

//  QTextBlockFormat bfmt = cursor.blockFormat();
//  bfmt->setIndent(0);

    fmt.clearBackground();

    mergeFormatOnWordOrSelection(fmt);
}


void RichTextEdit::textRemoveAllFormat() {
    f_bold      ->setChecked(false);
    f_underline ->setChecked(false);
    f_italic    ->setChecked(false);
    f_strikeout ->setChecked(false);
    f_fontsize  ->setCurrentIndex(f_fontsize->findText("9"));
    QString text = f_textedit->toPlainText();
    f_textedit->setPlainText(text);
}


void RichTextEdit::textBold() {
    QTextCharFormat fmt;
    fmt.setFontWeight(f_bold->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}


void RichTextEdit::focusInEvent(QFocusEvent *) {
    f_textedit->setFocus(Qt::TabFocusReason);
}


void RichTextEdit::textUnderline() {
    QTextCharFormat fmt;
    fmt.setFontUnderline(f_underline->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void RichTextEdit::textItalic() {
    QTextCharFormat fmt;
    fmt.setFontItalic(f_italic->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void RichTextEdit::textStrikeout() {
    QTextCharFormat fmt;
    fmt.setFontStrikeOut(f_strikeout->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void RichTextEdit::textSize(const QString &p) {
    qreal pointSize = p.toFloat();
    if (p.toFloat() > 0) {
        QTextCharFormat fmt;
        fmt.setFontPointSize(pointSize);
        mergeFormatOnWordOrSelection(fmt);
    }
}

void RichTextEdit::textLink(bool checked) {
    bool unlink = false;
    QTextCharFormat fmt;
    if (checked) {
        QString url = f_textedit->currentCharFormat().anchorHref();
        bool ok;
        QString newUrl = QInputDialog::getText(this, tr("Create a link"),
                                        tr("Link URL:"), QLineEdit::Normal,
                                        url,
                                        &ok);
        if (ok) {
            fmt.setAnchor(true);
            fmt.setAnchorHref(newUrl);
            fmt.setForeground(QApplication::palette().color(QPalette::Link));
            fmt.setFontUnderline(true);
          } else {
            unlink = true;
            }
      } else {
        unlink = true;
        }
    if (unlink) {
        fmt.setAnchor(false);
        fmt.setForeground(QApplication::palette().color(QPalette::Text));
        fmt.setFontUnderline(false);
        }
    mergeFormatOnWordOrSelection(fmt);
}

void RichTextEdit::textStyle(int index) {
    QTextCursor cursor = f_textedit->textCursor();
    cursor.beginEditBlock();

    // standard
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::BlockUnderCursor);
        }
    QTextCharFormat fmt;
    cursor.setCharFormat(fmt);
    f_textedit->setCurrentCharFormat(fmt);

    if (index == ParagraphHeading1
            || index == ParagraphHeading2
            || index == ParagraphHeading3
            || index == ParagraphHeading4 ) {
        if (index == ParagraphHeading1) {
            fmt.setFontPointSize(m_fontsize_h1);
            }
        if (index == ParagraphHeading2) {
            fmt.setFontPointSize(m_fontsize_h2);
            }
        if (index == ParagraphHeading3) {
            fmt.setFontPointSize(m_fontsize_h3);
            }
        if (index == ParagraphHeading4) {
            fmt.setFontPointSize(m_fontsize_h4);
            }
        if (index == ParagraphHeading2 || index == ParagraphHeading4) {
            fmt.setFontItalic(true);
            }

        fmt.setFontWeight(QFont::Bold);
        }
    if (index == ParagraphMonospace) {
        fmt = cursor.charFormat();
        fmt.setFontFamily("Monospace");
        fmt.setFontStyleHint(QFont::Monospace);
        fmt.setFontFixedPitch(true);
        }
    cursor.setCharFormat(fmt);
    f_textedit->setCurrentCharFormat(fmt);

    cursor.endEditBlock();
}

void RichTextEdit::textFgColor() {
    QColor col = QColorDialog::getColor(f_textedit->textColor(), this);
    QTextCursor cursor = f_textedit->textCursor();
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
        }
    QTextCharFormat fmt = cursor.charFormat();
    if (col.isValid()) {
        fmt.setForeground(col);
      } else {
        fmt.clearForeground();
        }
    cursor.setCharFormat(fmt);
    f_textedit->setCurrentCharFormat(fmt);
    fgColorChanged(col);
}

void RichTextEdit::textBgColor() {
    QColor col = QColorDialog::getColor(f_textedit->textBackgroundColor(), this);
    QTextCursor cursor = f_textedit->textCursor();
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
        }
    QTextCharFormat fmt = cursor.charFormat();
    if (col.isValid()) {
        fmt.setBackground(col);
      } else {
        fmt.clearBackground();
        }
    cursor.setCharFormat(fmt);
    f_textedit->setCurrentCharFormat(fmt);
    bgColorChanged(col);
}

void RichTextEdit::listBullet(bool checked) {
    if (checked) {
        f_list_ordered->setChecked(false);
        }
    list(checked, QTextListFormat::ListDisc);
}

void RichTextEdit::listOrdered(bool checked) {
    if (checked) {
        f_list_bullet->setChecked(false);
        }
    list(checked, QTextListFormat::ListDecimal);
}

void RichTextEdit::list(bool checked, QTextListFormat::Style style) {
    QTextCursor cursor = f_textedit->textCursor();
    cursor.beginEditBlock();
    if (!checked) {
        QTextBlockFormat obfmt = cursor.blockFormat();
        QTextBlockFormat bfmt;
        bfmt.setIndent(obfmt.indent());
        cursor.setBlockFormat(bfmt);
      } else {
        QTextListFormat listFmt;
        if (cursor.currentList()) {
            listFmt = cursor.currentList()->format();
            }
        listFmt.setStyle(style);
        cursor.createList(listFmt);
        }
    cursor.endEditBlock();
}

void RichTextEdit::mergeFormatOnWordOrSelection(const QTextCharFormat &format) {
    QTextCursor cursor = f_textedit->textCursor();
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
        }
    cursor.mergeCharFormat(format);
    f_textedit->mergeCurrentCharFormat(format);
    f_textedit->setFocus(Qt::TabFocusReason);
}

void RichTextEdit::slotCursorPositionChanged() {
    QTextList *l = f_textedit->textCursor().currentList();
    if (m_lastBlockList && (l == m_lastBlockList || (l != 0 && m_lastBlockList != 0
                                 && l->format().style() == m_lastBlockList->format().style()))) {
        return;
        }
    m_lastBlockList = l;
    if (l) {
        QTextListFormat lfmt = l->format();
        if (lfmt.style() == QTextListFormat::ListDisc) {
            f_list_bullet->setChecked(true);
            f_list_ordered->setChecked(false);
          } else if (lfmt.style() == QTextListFormat::ListDecimal) {
            f_list_bullet->setChecked(false);
            f_list_ordered->setChecked(true);
          } else {
            f_list_bullet->setChecked(false);
            f_list_ordered->setChecked(false);
            }
      } else {
        f_list_bullet->setChecked(false);
        f_list_ordered->setChecked(false);
        }
}

void RichTextEdit::fontChanged(const QFont &f) {
    f_fontsize->setCurrentIndex(f_fontsize->findText(QString::number(f.pointSize())));
    f_bold->setChecked(f.bold());
    f_italic->setChecked(f.italic());
    f_underline->setChecked(f.underline());
    f_strikeout->setChecked(f.strikeOut());
    if (f.pointSize() == m_fontsize_h1) {
        f_paragraph->setCurrentIndex(ParagraphHeading1);
      } else if (f.pointSize() == m_fontsize_h2) {
        f_paragraph->setCurrentIndex(ParagraphHeading2);
      } else if (f.pointSize() == m_fontsize_h3) {
        f_paragraph->setCurrentIndex(ParagraphHeading3);
      } else if (f.pointSize() == m_fontsize_h4) {
        f_paragraph->setCurrentIndex(ParagraphHeading4);
      } else {
        if (f.fixedPitch() && f.family() == "Monospace") {
            f_paragraph->setCurrentIndex(ParagraphMonospace);
          } else {
            f_paragraph->setCurrentIndex(ParagraphStandard);
            }
        }
    if (f_textedit->textCursor().currentList()) {
        QTextListFormat lfmt = f_textedit->textCursor().currentList()->format();
        if (lfmt.style() == QTextListFormat::ListDisc) {
            f_list_bullet->setChecked(true);
            f_list_ordered->setChecked(false);
          } else if (lfmt.style() == QTextListFormat::ListDecimal) {
            f_list_bullet->setChecked(false);
            f_list_ordered->setChecked(true);
          } else {
            f_list_bullet->setChecked(false);
            f_list_ordered->setChecked(false);
            }
      } else {
        f_list_bullet->setChecked(false);
        f_list_ordered->setChecked(false);
      }
}

void RichTextEdit::fgColorChanged(const QColor &c) {
    QPixmap pix(16, 16);
    if (c.isValid()) {
        pix.fill(c);
      } else {
        pix.fill(QApplication::palette().foreground().color());
        }
    f_fgcolor->setIcon(pix);
}

void RichTextEdit::bgColorChanged(const QColor &c) {
    QPixmap pix(16, 16);
    if (c.isValid()) {
        pix.fill(c);
      } else {
        pix.fill(QApplication::palette().background().color());
        }
    f_bgcolor->setIcon(pix);
}

void RichTextEdit::slotCurrentCharFormatChanged(const QTextCharFormat &format) {
    fontChanged(format.font());
    bgColorChanged((format.background().isOpaque()) ? format.background().color() : QColor());
    fgColorChanged((format.foreground().isOpaque()) ? format.foreground().color() : QColor());
    f_link->setChecked(format.isAnchor());
}

void RichTextEdit::slotClipboardDataChanged() {
#ifndef QT_NO_CLIPBOARD
    if (const QMimeData *md = QApplication::clipboard()->mimeData())
        f_paste->setEnabled(md->hasText());
#endif
}

QString RichTextEdit::toHtml() const {
    QString s = f_textedit->toHtml();
    // convert emails to links
    s = s.replace(QRegExp("(<[^a][^>]+>(?:<span[^>]+>)?|\\s)([a-zA-Z\\d]+@[a-zA-Z\\d]+\\.[a-zA-Z]+)"), "\\1<a href=\"mailto:\\2\">\\2</a>");
    // convert links
    s = s.replace(QRegExp("(<[^a][^>]+>(?:<span[^>]+>)?|\\s)((?:https?|ftp|file)://[^\\s'\"<>]+)"), "\\1<a href=\"\\2\">\\2</a>");
    // see also: Utils::linkify()
    return s;
}

void RichTextEdit::increaseIndentation() {
    indent(+1);
}

void RichTextEdit::decreaseIndentation() {
    indent(-1);
}

void RichTextEdit::indent(int delta) {
    QTextCursor cursor = f_textedit->textCursor();
    cursor.beginEditBlock();
    QTextBlockFormat bfmt = cursor.blockFormat();
    int ind = bfmt.indent();
    if (ind + delta >= 0) {
        bfmt.setIndent(ind + delta);
        }
    cursor.setBlockFormat(bfmt);
    cursor.endEditBlock();
}

void RichTextEdit::setText(const QString& text) {
    if (text.isEmpty()) {
        setPlainText(text);
        return;
        }
    if (text[0] == '<') {
        setHtml(text);
      } else {
        setPlainText(text);
        }
}

void RichTextEdit::insertImage() {
	QString file;
	if (misc::getOpenFileName(window(), RshareSettings::LASTDIR_IMAGES, tr("Load Picture File"), "Pictures (*.png *.xpm *.jpg *.jpeg *.gif *.webp)", file)) {
		QString encodedImage;
		if (RsHtml::makeEmbeddedImage(file, encodedImage, 640*480, MAX_ALLOWED_GXS_MESSAGE_SIZE - 200)) {
			QTextDocumentFragment fragment = QTextDocumentFragment::fromHtml(encodedImage);
			f_textedit->textCursor().insertFragment(fragment);
		}
	}
}

void RichTextEdit::checkLength(){
	QString text;
	RsHtml::optimizeHtml(f_textedit, text);
	std::wstring msg = text.toStdWString();
	int charRemains = MAX_ALLOWED_GXS_MESSAGE_SIZE - msg.length();
	if(charRemains >= 0) {
		text = tr("It remains %1 characters after HTML conversion.").arg(charRemains);
		f_info->setStyleSheet("QLabel#f_info { }");
	}else{
		text = tr("Warning: This message is too big of %1 characters after HTML conversion.").arg((0-charRemains));
	    f_info->setStyleSheet("QLabel#f_info {color: red; font: bold; }");
	}
	//buttonBox->button(QDialogButtonBox::Ok)->setEnabled(charRemains>=0);
	f_info->setText(text);
}


void RichTextEdit::setPlaceHolderTextPosted() {
	f_textedit->setPlaceholderText(tr("Text (optional)"));
}
