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

#include <QClipboard>
#include <QColorDialog>
#include <QFontComboBox>
#include <QFile>
#include <QFileInfo>
#include <QPrintDialog>
#include <QPrinter>
#include <QTextCodec>
#include <QTextDocumentWriter>
#include <QTextDocumentFragment>
#include <QMessageBox>
#include <QPrintPreviewDialog>
#include <QTextBlock>

#include "CreateBlogMsg.h"
#include "gui/msgs/textformat.h"
#include "util/misc.h"

#include <retroshare/rsblogs.h>

/** Constructor */
CreateBlogMsg::CreateBlogMsg(std::string cId ,QWidget* parent, Qt::WindowFlags flags)
: mBlogId(cId), QMainWindow (parent, flags)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	setAttribute ( Qt::WA_DeleteOnClose, true );

	setupFileActions();
  setupEditActions();
  setupViewActions();
  setupInsertActions();
  setupParagraphActions();
  
  setAcceptDrops(true);
	setStartupText();
	
	newBlogMsg();
		
	ui.toolBar_2->addAction(ui.actionIncreasefontsize);
  ui.toolBar_2->addAction(ui.actionDecreasefontsize);
  ui.toolBar_2->addAction(ui.actionBlockquoute);
  ui.toolBar_2->addAction(ui.actionOrderedlist);
  ui.toolBar_2->addAction(ui.actionUnorderedlist);
  ui.toolBar_2->addAction(ui.actionBlockquoute);
  ui.toolBar_2->addAction(ui.actionCode);
  ui.toolBar_2->addAction(ui.actionsplitPost);
  
  setupTextActions();

	connect(ui.actionPublish, SIGNAL(triggered()), this, SLOT(sendMsg()));
	connect(ui.actionNew, SIGNAL(triggered()), this, SLOT (fileNew()));
	
	connect(ui.actionIncreasefontsize, SIGNAL (triggered()), this, SLOT (fontSizeIncrease()));
  connect(ui.actionDecreasefontsize, SIGNAL (triggered()), this, SLOT (fontSizeDecrease()));
  connect(ui.actionBlockquoute, SIGNAL (triggered()), this, SLOT (blockQuote()));
  connect(ui.actionCode, SIGNAL (triggered()), this, SLOT (toggleCode()));
  connect(ui.actionsplitPost, SIGNAL (triggered()), this, SLOT (addPostSplitter()));  
  connect(ui.actionOrderedlist, SIGNAL (triggered()), this, SLOT (addOrderedList()));
  connect(ui.actionUnorderedlist, SIGNAL (triggered()), this, SLOT (addUnorderedList()));

  //connect(webView, SIGNAL(loadFinished(bool)),this, SLOT(updateTextEdit()));
  connect( ui.msgEdit, SIGNAL( textChanged(const QString &)), this, SLOT(updateTextEdit()));
  
  connect( ui.msgEdit, SIGNAL(currentCharFormatChanged(QTextCharFormat)),
            this, SLOT(currentCharFormatChanged(QTextCharFormat)));
  connect( ui.msgEdit, SIGNAL(cursorPositionChanged()),
            this, SLOT(cursorPositionChanged()));
	
	QPalette palette = QApplication::palette();
  codeBackground = palette.color( QPalette::Active, QPalette::Midlight );
  
  fontChanged(ui.msgEdit->font());
  colorChanged(ui.msgEdit->textColor());
  alignmentChanged(ui.msgEdit->alignment());
  
    connect( ui.msgEdit->document(), SIGNAL(modificationChanged(bool)),
            actionSave, SLOT(setEnabled(bool)));
    connect( ui.msgEdit->document(), SIGNAL(modificationChanged(bool)),
            this, SLOT(setWindowModified(bool)));
    connect( ui.msgEdit->document(), SIGNAL(undoAvailable(bool)),
            actionUndo, SLOT(setEnabled(bool)));
    connect( ui.msgEdit->document(), SIGNAL(undoAvailable(bool)),
            ui.actionUndo, SLOT(setEnabled(bool)));        
    connect( ui.msgEdit->document(), SIGNAL(redoAvailable(bool)),
            actionRedo, SLOT(setEnabled(bool)));

    setWindowModified( ui.msgEdit->document()->isModified());
    actionSave->setEnabled( ui.msgEdit->document()->isModified());
    actionUndo->setEnabled( ui.msgEdit->document()->isUndoAvailable());
    ui.actionUndo->setEnabled( ui.msgEdit->document()->isUndoAvailable());
    actionRedo->setEnabled( ui.msgEdit->document()->isRedoAvailable());

    connect(actionUndo, SIGNAL(triggered()), ui.msgEdit, SLOT(undo()));
    connect(ui.actionUndo, SIGNAL(triggered()), ui.msgEdit, SLOT(undo()));
    connect(actionRedo, SIGNAL(triggered()), ui.msgEdit, SLOT(redo()));
    
    actionCut->setEnabled(false);
    actionCopy->setEnabled(false);

    connect(actionCut, SIGNAL(triggered()), ui.msgEdit, SLOT(cut()));
    connect(actionCopy, SIGNAL(triggered()), ui.msgEdit, SLOT(copy()));
    connect(actionPaste, SIGNAL(triggered()), ui.msgEdit, SLOT(paste()));
    
    connect(ui.msgEdit, SIGNAL(copyAvailable(bool)), actionCut, SLOT(setEnabled(bool)));
    connect(ui.msgEdit, SIGNAL(copyAvailable(bool)), actionCopy, SLOT(setEnabled(bool)));
    
#ifndef QT_NO_CLIPBOARD
    connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(clipboardDataChanged()));
#endif
  
  //defaultCharFormat
  defaultCharFormat = ui.msgEdit->currentCharFormat();

  const QFont defaultFont = ui.msgEdit->document()->defaultFont();
  defaultCharFormat.setFont( defaultFont );
  defaultCharFormat.setForeground( ui.msgEdit->currentCharFormat().foreground() );
  defaultCharFormat.setProperty( QTextFormat::FontSizeAdjustment, QVariant( 0 ) );
  defaultCharFormat.setBackground( palette.color( QPalette::Active,
                                                    QPalette::Base ) );
  defaultCharFormat.setProperty( TextFormat::HasCodeStyle, QVariant( false ) );

  //defaultBlockFormat
  defaultBlockFormat = ui.msgEdit->textCursor().blockFormat();
  
}


void CreateBlogMsg::cancelMsg()
{
	std::cerr << "CreateBlogMsg::cancelMsg()";
	std::cerr << std::endl;
	close();
	return;
}

void CreateBlogMsg::newBlogMsg()
{

	if (!rsBlogs)
		return;

	BlogInfo ci;
	if (!rsBlogs->getBlogInfo(mBlogId, ci))
	{

		return;
	}
			
	ui.channelName->setText(QString::fromStdWString(ci.blogName));

}


void CreateBlogMsg::sendMsg()
{
	std::cerr << "CreateBlogMsg::sendMsg()";
	std::cerr << std::endl;

	/* construct message bits */
	std::wstring subject = ui.subjectEdit->text().toStdWString();
	std::wstring msg     = ui.msgEdit->toHtml().toStdWString();

	sendMessage(subject, msg);

}

void CreateBlogMsg::sendMessage(std::wstring subject, std::wstring msg)
{
	std::cerr << "CreateBlogMsg::sendMessage()" << std::endl;

	QString name = ui.subjectEdit->text();

	if(name.isEmpty())
	{	/* error message */
		QMessageBox::warning(this, tr("RetroShare"),
                   tr("Please add a Subject"),
                   QMessageBox::Ok, QMessageBox::Ok);
                   
		return; //Don't add  a empty Subject!!
	}
	else

	/* rsChannels */
	if (rsBlogs)
	{
		BlogMsgInfo msgInfo;
				
		msgInfo.blogId = mBlogId;
		msgInfo.msgId = "";
				
		msgInfo.subject = subject;
		msgInfo.msg = msg;
		msgInfo.msgIdReply = "nothing";
		rsBlogs->BlogMessageSend(msgInfo);
	}
			
	close();
  return;

}

void CreateBlogMsg::fontSizeIncrease()
{
    if ( !( ui.msgEdit->textCursor().blockFormat().hasProperty( TextFormat::HtmlHeading ) &&
        ui.msgEdit->textCursor().blockFormat().intProperty( TextFormat::HtmlHeading ) ) ) {
        QTextCharFormat format;
        int idx = ui.msgEdit->currentCharFormat().intProperty( QTextFormat::FontSizeAdjustment );
        if ( idx < 3 ) {
            format.setProperty( QTextFormat::FontSizeAdjustment, QVariant( ++idx ) );
            ui.msgEdit->textCursor().mergeCharFormat( format );
        }
    }
    ui.msgEdit->setFocus( Qt::OtherFocusReason );
}

void CreateBlogMsg::fontSizeDecrease()
{
    if ( !( ui.msgEdit->textCursor().blockFormat().hasProperty( TextFormat::HtmlHeading ) &&
        ui.msgEdit->textCursor().blockFormat().intProperty( TextFormat::HtmlHeading ) ) ) {
        QTextCharFormat format;
        int idx = ui.msgEdit->currentCharFormat().intProperty( QTextFormat::FontSizeAdjustment );
        if ( idx > -1 ) {
            format.setProperty( QTextFormat::FontSizeAdjustment, QVariant( --idx ) );
            ui.msgEdit->textCursor().mergeCharFormat( format );
        }
    }
    ui.msgEdit->setFocus( Qt::OtherFocusReason );
}

void CreateBlogMsg::blockQuote()
{
    QTextBlockFormat blockFormat = ui.msgEdit->textCursor().blockFormat();
    QTextBlockFormat f;

    if ( blockFormat.hasProperty( TextFormat::IsBlockQuote ) && 
         blockFormat.boolProperty( TextFormat::IsBlockQuote ) ) {
        f.setProperty( TextFormat::IsBlockQuote, QVariant( false ) );
        f.setLeftMargin( 0 );
        f.setRightMargin( 0 );
    } else {
        f.setProperty( TextFormat::IsBlockQuote, QVariant( true ) );
        f.setLeftMargin( 40 );
        f.setRightMargin( 40 );
    }
    ui.msgEdit->textCursor().mergeBlockFormat( f );
}

void CreateBlogMsg::toggleCode()
{
    static QString preFontFamily;

    QTextCharFormat charFormat = ui.msgEdit->currentCharFormat();
    QTextCharFormat f;
    
    if ( charFormat.hasProperty( TextFormat::HasCodeStyle ) &&
         charFormat.boolProperty( TextFormat::HasCodeStyle ) ) {
        f.setProperty( TextFormat::HasCodeStyle, QVariant( false ) );
        f.setBackground( defaultCharFormat.background() );
        f.setFontFamily( preFontFamily );
        ui.msgEdit->textCursor().mergeCharFormat( f );

    } else {
        preFontFamily = ui.msgEdit->fontFamily();
        f.setProperty( TextFormat::HasCodeStyle, QVariant( true ) );
        f.setBackground( codeBackground );
        f.setFontFamily( "Dejavu Sans Mono" );
        ui.msgEdit->textCursor().mergeCharFormat( f );
    }
    ui.msgEdit->setFocus( Qt::OtherFocusReason );
}

void CreateBlogMsg::addPostSplitter()
{
    QTextBlockFormat f = ui.msgEdit->textCursor().blockFormat();
    QTextBlockFormat f1 = f;

    f.setProperty( TextFormat::IsHtmlTagSign, true );
    f.setProperty( QTextFormat::BlockTrailingHorizontalRulerWidth, 
             QTextLength( QTextLength::PercentageLength, 80 ) );
    if ( ui.msgEdit->textCursor().block().text().isEmpty() ) {
        ui.msgEdit->textCursor().mergeBlockFormat( f );
    } else {
        ui.msgEdit->textCursor().insertBlock( f );
    }
    ui.msgEdit->textCursor().insertBlock( f1 );
}

void CreateBlogMsg::setStartupText()
{
    QString string = "<html><body><h1>HTML Previewer</h1>"
                     " <p>This example shows you how to use QWebView to"
                     " preview HTML data written in a QPlainTextEdit.</p>"
                     " </body></html>";
    //webView->setHtml(string);
}

void CreateBlogMsg::updateTextEdit()
{
    //QWebFrame *mainFrame = webView->page()->mainFrame();
    QString frameText = ui.msgEdit->toHtml();
    ui.plainTextEdit->setPlainText(frameText);
    //QString text = plainTextEdit->toPlainText();
}
			
void CreateBlogMsg::textBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(actionTextBold->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}

void CreateBlogMsg::textUnderline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(actionTextUnderline->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void CreateBlogMsg::textItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(actionTextItalic->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}


void CreateBlogMsg::textAlign(QAction *a)
{
    if (a == actionAlignLeft)
        ui.msgEdit->setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
    else if (a == actionAlignCenter)
        ui.msgEdit->setAlignment(Qt::AlignHCenter | Qt::AlignAbsolute);
    else if (a == actionAlignRight)
        ui.msgEdit->setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
    else if (a == actionAlignJustify)
        ui.msgEdit->setAlignment(Qt::AlignJustify | Qt::AlignAbsolute);

}

void CreateBlogMsg::alignmentChanged(Qt::Alignment a)
{
    if (a & Qt::AlignLeft) {
        actionAlignLeft->setChecked(true);
    } else if (a & Qt::AlignHCenter) {
        actionAlignCenter->setChecked(true);
    } else if (a & Qt::AlignRight) {
        actionAlignRight->setChecked(true);
    } else if (a & Qt::AlignJustify) {
        actionAlignJustify->setChecked(true);
    }
}

void CreateBlogMsg::textFamily(const QString &f)
{
    QTextCharFormat fmt;
    fmt.setFontFamily(f);
    mergeFormatOnWordOrSelection(fmt);
}

void CreateBlogMsg::textSize(const QString &p)
{
    qreal pointSize = p.toFloat();
    if (p.toFloat() > 0) {
        QTextCharFormat fmt;
        fmt.setFontPointSize(pointSize);
        mergeFormatOnWordOrSelection(fmt);
    }
}

void CreateBlogMsg::changeFormatType(int styleIndex )
{
    ui.msgEdit->setFocus( Qt::OtherFocusReason );

    QTextCursor cursor = ui.msgEdit->textCursor();
    //QTextBlockFormat bformat = cursor.blockFormat();
    QTextBlockFormat bformat;
    QTextCharFormat cformat;

    switch (styleIndex) {
         default:
            case 0:
            bformat.setProperty( TextFormat::HtmlHeading, QVariant( 0 ) );
            cformat.setFontWeight( QFont::Normal );
            cformat.setProperty( QTextFormat::FontSizeAdjustment, QVariant( 0 ) );
            break;
        case 1:
            bformat.setProperty( TextFormat::HtmlHeading, QVariant( 1 ) );
            cformat.setFontWeight( QFont::Bold );
            cformat.setProperty( QTextFormat::FontSizeAdjustment, QVariant( 3 ) );
            break;
        case 2:
            bformat.setProperty( TextFormat::HtmlHeading, QVariant( 2 ) );
            cformat.setFontWeight( QFont::Bold );
            cformat.setProperty( QTextFormat::FontSizeAdjustment, QVariant( 2 ) );
            break;
        case 3:
            bformat.setProperty( TextFormat::HtmlHeading, QVariant( 3 ) );
            cformat.setFontWeight( QFont::Bold );
            cformat.setProperty( QTextFormat::FontSizeAdjustment, QVariant( 1 ) );
            break;
        case 4:
            bformat.setProperty( TextFormat::HtmlHeading, QVariant( 4 ) );
            cformat.setFontWeight( QFont::Bold );
            cformat.setProperty( QTextFormat::FontSizeAdjustment, QVariant( 0 ) );
            break;
        case 5:
            bformat.setProperty( TextFormat::HtmlHeading, QVariant( 5 ) );
            cformat.setFontWeight( QFont::Bold );
            cformat.setProperty( QTextFormat::FontSizeAdjustment, QVariant( -1 ) );
            break;
        case 6:
            bformat.setProperty( TextFormat::HtmlHeading, QVariant( 6 ) );
            cformat.setFontWeight( QFont::Bold );
            cformat.setProperty( QTextFormat::FontSizeAdjustment, QVariant( -2 ) );
            break;
    }
    //cformat.clearProperty( TextFormat::HasCodeStyle );

    cursor.beginEditBlock();
    cursor.mergeBlockFormat( bformat );
    cursor.select( QTextCursor::BlockUnderCursor );
    cursor.mergeCharFormat( cformat );
    cursor.endEditBlock();
}


void CreateBlogMsg::textColor()
{
    QColor col = QColorDialog::getColor(ui.msgEdit->textColor(), this);
    if (!col.isValid())
        return;
    QTextCharFormat fmt;
    fmt.setForeground(col);
    mergeFormatOnWordOrSelection(fmt);
    colorChanged(col);
}

void CreateBlogMsg::fontChanged(const QFont &f)
{
    comboFont->setCurrentIndex(comboFont->findText(QFontInfo(f).family()));
    comboSize->setCurrentIndex(comboSize->findText(QString::number(f.pointSize())));
    actionTextBold->setChecked(f.bold());
    actionTextItalic->setChecked(f.italic());
    actionTextUnderline->setChecked(f.underline());
}

void CreateBlogMsg::colorChanged(const QColor &c)
{
    QPixmap pix(16, 16);
    pix.fill(c);
    //ui.colorbtn->setIcon(pix);
    actionTextColor->setIcon(pix);
}

void CreateBlogMsg::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = ui.msgEdit->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);
    ui.msgEdit->mergeCurrentCharFormat(format);
}

void CreateBlogMsg::currentCharFormatChanged(const QTextCharFormat &format)
{
    fontChanged(format.font());
    colorChanged(format.foreground().color());
}

void CreateBlogMsg::cursorPositionChanged()
{
    alignmentChanged(ui.msgEdit->alignment());
}

void CreateBlogMsg::clipboardDataChanged()
{
#ifndef QT_NO_CLIPBOARD
    actionPaste->setEnabled(!QApplication::clipboard()->text().isEmpty());
#endif
}


void CreateBlogMsg::addOrderedList()
{
    ui.msgEdit->textCursor().createList( QTextListFormat::ListDecimal );        

}

void CreateBlogMsg::addUnorderedList()
{
    ui.msgEdit->textCursor().createList( QTextListFormat::ListDisc );        
}

void CreateBlogMsg::setupFileActions()
{
    QMenu *menu = new QMenu(tr("&File"), this);
    menuBar()->addMenu(menu);

    QAction *a;

    a = new QAction(QIcon(":/images/textedit/filenew.png"), tr("&New"), this);
    a->setShortcut(QKeySequence::New);
    connect(a, SIGNAL(triggered()), this, SLOT(fileNew()));
    menu->addAction(a);

    a = new QAction(QIcon(":/images/textedit/fileopen.png"), tr("&Open..."), this);
    a->setShortcut(QKeySequence::Open);
    connect(a, SIGNAL(triggered()), this, SLOT(fileOpen()));
    menu->addAction(a);

    menu->addSeparator();

    actionSave = a = new QAction(QIcon(":/images/textedit/filesave.png"), tr("&Save"), this);
    a->setShortcut(QKeySequence::Save);
    connect(a, SIGNAL(triggered()), this, SLOT(fileSave()));
    a->setEnabled(false);
    menu->addAction(a);

    a = new QAction(tr("Save &As..."), this);
    connect(a, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
    menu->addAction(a);
    menu->addSeparator();

    a = new QAction(QIcon(":/images/textedit/fileprint.png"), tr("&Print..."), this);
    a->setShortcut(QKeySequence::Print);
    connect(a, SIGNAL(triggered()), this, SLOT(filePrint()));
    menu->addAction(a);

    a = new QAction(QIcon(":/images/textedit/fileprint.png"), tr("Print Preview..."), this);
    connect(a, SIGNAL(triggered()), this, SLOT(filePrintPreview()));
    menu->addAction(a);

    a = new QAction(QIcon(":/images/textedit/exportpdf.png"), tr("&Export PDF..."), this);
    a->setShortcut(Qt::CTRL + Qt::Key_D);
    connect(a, SIGNAL(triggered()), this, SLOT(filePrintPdf()));
    menu->addAction(a);

    menu->addSeparator();

    a = new QAction(tr("&Quit"), this);
    a->setShortcut(Qt::CTRL + Qt::Key_Q);
    connect(a, SIGNAL(triggered()), this, SLOT(cancelMsg()));
    menu->addAction(a);
}

void CreateBlogMsg::setupEditActions()
{
    QMenu *menu = new QMenu(tr("&Edit"), this);
    menuBar()->addMenu(menu);

    QAction *a;
    a = actionUndo = new QAction(QIcon(":/images/textedit/editundo.png"), tr("&Undo"), this);
    a->setShortcut(QKeySequence::Undo);
    menu->addAction(a);
    a = actionRedo = new QAction(QIcon(":/images/textedit/editredo.png"), tr("&Redo"), this);
    a->setShortcut(QKeySequence::Redo);
    menu->addAction(a);
    menu->addSeparator();
    a = actionCut = new QAction(QIcon(":/images/textedit/editcut.png"), tr("Cu&t"), this);
    a->setShortcut(QKeySequence::Cut);
    menu->addAction(a);
    a = actionCopy = new QAction(QIcon(":/images/textedit/editcopy.png"), tr("&Copy"), this);
    a->setShortcut(QKeySequence::Copy);
    menu->addAction(a);
    a = actionPaste = new QAction(QIcon(":/images/textedit/editpaste.png"), tr("&Paste"), this);
    a->setShortcut(QKeySequence::Paste);
    menu->addAction(a);
    actionPaste->setEnabled(!QApplication::clipboard()->text().isEmpty());
}

void CreateBlogMsg::setupViewActions()
{
    QMenu *menu = new QMenu(tr("&View"), this);
    menuBar()->addMenu(menu);

    QAction *a;


}

void CreateBlogMsg::setupInsertActions()
{
    QMenu *menu = new QMenu(tr("&Insert"), this);
    menuBar()->addMenu(menu);

    QAction *a;

    a = new QAction(QIcon(""), tr("&Image"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(addImage()));
    menu->addAction(a);

}

void CreateBlogMsg::setupParagraphActions()
{
    comboStyle = new QComboBox(ui.toolBar_2);
    ui.toolBar_2->addWidget(comboStyle);
    comboStyle->addItem("Paragraph");
    comboStyle->addItem("Heading 1");
    comboStyle->addItem("Heading 2");
    comboStyle->addItem("Heading 3");
    comboStyle->addItem("Heading 4");
    comboStyle->addItem("Heading 5");
    comboStyle->addItem("Heading 6");

    connect(comboStyle, SIGNAL(activated(int)),
            this, SLOT(changeFormatType(int)));
}

void CreateBlogMsg::setupTextActions()
{

    QMenu *menu = new QMenu(tr("F&ormat"), this);
    menuBar()->addMenu(menu);
    
    actionTextBold = new QAction(QIcon(":/images/textedit/textbold.png"),tr("&Bold"), this);
    actionTextBold->setShortcut(Qt::CTRL + Qt::Key_B);
    //actionTextBold->setPriority(QAction::LowPriority);
    QFont bold;
    bold.setBold(true);
    actionTextBold->setFont(bold);
    connect(actionTextBold, SIGNAL(triggered()), this, SLOT(textBold()));
    
    ui.toolBar_2->addAction(actionTextBold);
    menu->addAction(actionTextBold);
    actionTextBold->setCheckable(true);

    actionTextItalic = new QAction(QIcon(":/images/textedit/textitalic.png"),tr("&Italic"), this);
    //actionTextItalic->setPriority(QAction::LowPriority);
    actionTextItalic->setShortcut(Qt::CTRL + Qt::Key_I);
    QFont italic;
    italic.setItalic(true);
    actionTextItalic->setFont(italic);
    connect(actionTextItalic, SIGNAL(triggered()), this, SLOT(textItalic()));
    
    ui.toolBar_2->addAction(actionTextItalic);
    menu->addAction(actionTextItalic);
    actionTextItalic->setCheckable(true);

    actionTextUnderline = new QAction(QIcon(":/images/textedit/textunder.png"),tr("&Underline"), this);
    actionTextUnderline->setShortcut(Qt::CTRL + Qt::Key_U);
    //actionTextUnderline->setPriority(QAction::LowPriority);
    QFont underline;
    underline.setUnderline(true);
    actionTextUnderline->setFont(underline);
    connect(actionTextUnderline, SIGNAL(triggered()), this, SLOT(textUnderline()));
    
    ui.toolBar_2->addAction(actionTextUnderline);
    menu->addAction(actionTextUnderline);
    actionTextUnderline->setCheckable(true);

    menu->addSeparator();

    QActionGroup *grp = new QActionGroup(this);
    connect(grp, SIGNAL(triggered(QAction*)), this, SLOT(textAlign(QAction*)));

    // Make sure the alignLeft  is always left of the alignRight
    if (QApplication::isLeftToRight()) {
        actionAlignLeft = new QAction(QIcon(":/images/textedit/textleft.png"),tr("&Left"), grp);
        actionAlignCenter = new QAction(QIcon(":/images/textedit/textcenter.png"), tr("C&enter"), grp);
        actionAlignRight = new QAction(QIcon(":/images/textedit/textright.png"), tr("&Right"), grp);
    } else {
        actionAlignRight = new QAction(QIcon(":/images/textedit/textright.png"), tr("&Right"), grp);
        actionAlignCenter = new QAction(QIcon(":/images/textedit/textcenter.png"), tr("C&enter"), grp);
        actionAlignLeft = new QAction(QIcon(":/images/textedit/textleft.png"), tr("&Left"), grp);
    }
    actionAlignJustify = new QAction(QIcon(":/images/textedit/textjustify.png"), tr("&Justify"), grp);

    actionAlignLeft->setShortcut(Qt::CTRL + Qt::Key_L);
    actionAlignLeft->setCheckable(true);
    //actionAlignLeft->setPriority(QAction::LowPriority);
    actionAlignCenter->setShortcut(Qt::CTRL + Qt::Key_E);
    actionAlignCenter->setCheckable(true);
    //actionAlignCenter->setPriority(QAction::LowPriority);
    actionAlignRight->setShortcut(Qt::CTRL + Qt::Key_R);
    actionAlignRight->setCheckable(true);
    //actionAlignRight->setPriority(QAction::LowPriority);
    actionAlignJustify->setShortcut(Qt::CTRL + Qt::Key_J);
    actionAlignJustify->setCheckable(true);
    //actionAlignJustify->setPriority(QAction::LowPriority);

    ui.toolBar_2->addActions(grp->actions());
    menu->addActions(grp->actions());

    menu->addSeparator();

    QPixmap pix(16, 16);
    pix.fill(Qt::black);
    actionTextColor = new QAction(pix, tr("&Text Color..."), this);
    connect(actionTextColor, SIGNAL(triggered()), this, SLOT(textColor()));
    
    ui.toolBar_2->addAction(actionTextColor);
    menu->addAction(actionTextColor);
    
    menu->addAction(ui.actionOrderedlist);
    menu->addAction(ui.actionUnorderedlist);
    menu->addAction(ui.actionBlockquoute);


    /*comboStyle = new QComboBox(ui.toolBar_2);
    ui.toolBar_2->addWidget(comboStyle);
    comboStyle->addItem("Paragraph");
    comboStyle->addItem("Heading 1");
    comboStyle->addItem("Heading 2");
    comboStyle->addItem("Heading 3");
    comboStyle->addItem("Heading 4");
    comboStyle->addItem("Heading 5");
    comboStyle->addItem("Heading 6");

    connect(comboStyle, SIGNAL(activated(int)),
            this, SLOT(changeFormatType(int)));*/

    comboFont = new QFontComboBox(ui.toolBar_2);
    ui.toolBar_2->addWidget(comboFont);
    connect(comboFont, SIGNAL(activated(QString)),
            this, SLOT(textFamily(QString)));

    comboSize = new QComboBox(ui.toolBar_2);
    comboSize->setObjectName("comboSize");
    ui.toolBar_2->addWidget(comboSize);
    comboSize->setEditable(true);

    QFontDatabase db;
    foreach(int size, db.standardSizes())
        comboSize->addItem(QString::number(size));

    connect(comboSize, SIGNAL(activated(QString)),
            this, SLOT(textSize(QString)));
    comboSize->setCurrentIndex(comboSize->findText(QString::number(QApplication::font()
                                                                   .pointSize())));
}

bool CreateBlogMsg::load(const QString &f)
{
    if (!QFile::exists(f))
        return false;
    QFile file(f);
    if (!file.open(QFile::ReadOnly))
        return false;

    QByteArray data = file.readAll();
    QTextCodec *codec = Qt::codecForHtml(data);
    QString str = codec->toUnicode(data);
    if (Qt::mightBeRichText(str)) {
        ui.msgEdit->setHtml(str);
    } else {
        str = QString::fromLocal8Bit(data);
        ui.msgEdit->setPlainText(str);
    }

    setCurrentFileName(f);
    return true;
}

bool CreateBlogMsg::maybeSave()
{
    if (!ui.msgEdit->document()->isModified())
        return true;
    if (fileName.startsWith(QLatin1String(":/")))
        return true;
    QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, tr("Application"),
                               tr("The document has been modified.\n"
                                  "Do you want to save your changes?"),
                               QMessageBox::Save | QMessageBox::Discard
                               | QMessageBox::Cancel);
    if (ret == QMessageBox::Save)
        return fileSave();
    else if (ret == QMessageBox::Cancel)
        return false;
    return true;
}

void CreateBlogMsg::fileNew()
{
    if (maybeSave()) {
        ui.msgEdit->clear();
        setCurrentFileName(QString());
    }
}

void CreateBlogMsg::fileOpen()
{
    QString fn;
    if (misc::getOpenFileName(this, RshareSettings::LASTDIR_BLOGS, tr("Open File..."), tr("HTML-Files (*.htm *.html);;All Files (*)"), fn))
        load(fn);
}

bool CreateBlogMsg::fileSave()
{
    if (fileName.isEmpty())
        return fileSaveAs();

    QTextDocumentWriter writer(fileName);
    bool success = writer.write(ui.msgEdit->document());
    if (success)
        ui.msgEdit->document()->setModified(false);
    return success;
}

bool CreateBlogMsg::fileSaveAs()
{
    QString fn;
    if (misc::getSaveFileName(this, RshareSettings::LASTDIR_BLOGS, tr("Save as..."), tr("ODF files (*.odt);;HTML-Files (*.htm *.html);;All Files (*)"), fn)) {
        if (! (fn.endsWith(".odt", Qt::CaseInsensitive) || fn.endsWith(".htm", Qt::CaseInsensitive) || fn.endsWith(".html", Qt::CaseInsensitive)) )
            fn += ".odt"; // default
        setCurrentFileName(fn);
        return fileSave();
    }

    return false;
}

void CreateBlogMsg::filePrint()
{
#ifndef QT_NO_PRINTER
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog *dlg = new QPrintDialog(&printer, this);
    if (ui.msgEdit->textCursor().hasSelection())
        dlg->addEnabledOption(QAbstractPrintDialog::PrintSelection);
    dlg->setWindowTitle(tr("Print Document"));
    if (dlg->exec() == QDialog::Accepted) {
        ui.msgEdit->print(&printer);
    }
    delete dlg;
#endif
}

void CreateBlogMsg::filePrintPreview()
{
#ifndef QT_NO_PRINTER
    QPrinter printer(QPrinter::HighResolution);
    QPrintPreviewDialog preview(&printer, this);
    connect(&preview, SIGNAL(paintRequested(QPrinter*)), SLOT(printPreview(QPrinter*)));
    preview.exec();
#endif
}

void CreateBlogMsg::printPreview(QPrinter *printer)
{
#ifdef QT_NO_PRINTER
    Q_UNUSED(printer);
#else
    ui.msgEdit->print(printer);
#endif
}


void CreateBlogMsg::filePrintPdf()
{
#ifndef QT_NO_PRINTER
//! [0]
    QString fileName;
    if (misc::getSaveFileName(this, RshareSettings::LASTDIR_MESSAGES, tr("Export PDF"), "*.pdf", fileName)) {
        if (QFileInfo(fileName).suffix().isEmpty())
            fileName.append(".pdf");
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(fileName);
        ui.msgEdit->document()->print(&printer);
    }
//! [0]
#endif
}

void CreateBlogMsg::setCurrentFileName(const QString &fileName)
{
    this->fileName = fileName;
    ui.msgEdit->document()->setModified(false);

    QString shownName;
    if (fileName.isEmpty())
        shownName = "untitled.txt";
    else
        shownName = QFileInfo(fileName).fileName();

    //setWindowTitle(tr("%1[*] - %2").arg(shownName).arg(tr("Rich Text")));
    setWindowModified(false);
}

void CreateBlogMsg::addImage()
{
    QString fileimg;
    if (misc::getOpenFileName(this, RshareSettings::LASTDIR_MESSAGES, tr("Choose Image"), tr("Image Files supported (*.png *.jpeg *.jpg *.gif)"), fileimg)) {
        QImage base(fileimg);
    
        Create_New_Image_Tag(fileimg);
    }
}

void  CreateBlogMsg::Create_New_Image_Tag( const QString urlremoteorlocal )
{
   /*if (image_extension(urlremoteorlocal)) {*/
       QString subtext = QString("<p><img src=\"%1\">").arg(urlremoteorlocal);
               ///////////subtext.append("<br><br>Description on image.</p>");
       QTextDocumentFragment fragment = QTextDocumentFragment::fromHtml(subtext);
       ui.msgEdit->textCursor().insertFragment(fragment);
       //emit statusMessage(QString("Image new :").arg(urlremoteorlocal));
   //}
}
