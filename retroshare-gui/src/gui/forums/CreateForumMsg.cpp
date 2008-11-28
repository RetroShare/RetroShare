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


#include "CreateForumMsg.h"

#include <gui/Preferences/rsharesettings.h>

#include "rsiface/rsforums.h"

#include <QContextMenuEvent>
#include <QCloseEvent>
#include <QColorDialog>
#include <QClipboard>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QMessageBox>
#include <QPixmap>
#include <QPrintDialog>
#include <QPrinter>
#include <QHeaderView>
#include <QTextCodec>
#include <QTextEdit>
#include <QTextCursor>
#include <QTextList>
#include <QTextStream>
#include <QTextDocumentFragment>

/** Constructor */
CreateForumMsg::CreateForumMsg(std::string fId, std::string pId, QWidget *parent, Qt::WFlags flags)
: QMainWindow(parent, flags), mForumId(fId), mParentId(pId)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  setupFileActions();
  setupEditActions();
  setupViewActions();
  setupInsertActions();
  
  RshareSettings config;
  config.loadWidgetInformation(this);

  setAttribute ( Qt::WA_DeleteOnClose, true );

  
  // connect up the buttons.
  connect( ui.postmessage_action, SIGNAL( triggered (bool) ), this, SLOT( createMsg( ) ) );
  connect( ui.close_action, SIGNAL( triggered (bool) ), this, SLOT( cancelMsg( ) ) );

  connect(ui.boldbtn, SIGNAL(clicked()), this, SLOT(textBold()));
  connect(ui.underlinebtn, SIGNAL(clicked()), this, SLOT(textUnderline()));
  connect(ui.italicbtn, SIGNAL(clicked()), this, SLOT(textItalic()));
  connect(ui.colorbtn, SIGNAL(clicked()), this, SLOT(textColor()));

  connect(ui.forumMessage, SIGNAL(currentCharFormatChanged(const QTextCharFormat &)),
            this, SLOT(currentCharFormatChanged(const QTextCharFormat &)));
  connect(ui.forumMessage, SIGNAL(cursorPositionChanged()),
            this, SLOT(cursorPositionChanged()));

    connect(ui.forumMessage->document(), SIGNAL(modificationChanged(bool)),
            actionSave, SLOT(setEnabled(bool)));
    connect(ui.forumMessage->document(), SIGNAL(modificationChanged(bool)),
            this, SLOT(setWindowModified(bool)));
    connect(ui.forumMessage->document(), SIGNAL(undoAvailable(bool)),
            actionUndo, SLOT(setEnabled(bool)));
    connect(ui.forumMessage->document(), SIGNAL(redoAvailable(bool)),
            actionRedo, SLOT(setEnabled(bool)));

    setWindowModified(ui.forumMessage->document()->isModified());
    actionSave->setEnabled(ui.forumMessage->document()->isModified());
    actionUndo->setEnabled(ui.forumMessage->document()->isUndoAvailable());
    actionRedo->setEnabled(ui.forumMessage->document()->isRedoAvailable());

    connect(actionUndo, SIGNAL(triggered()), ui.forumMessage, SLOT(undo()));
    connect(actionRedo, SIGNAL(triggered()), ui.forumMessage, SLOT(redo()));

    actionCut->setEnabled(false);
    actionCopy->setEnabled(false);

    connect(actionCut, SIGNAL(triggered()), ui.forumMessage, SLOT(cut()));
    connect(actionCopy, SIGNAL(triggered()), ui.forumMessage, SLOT(copy()));
    connect(actionPaste, SIGNAL(triggered()), ui.forumMessage, SLOT(paste()));

    connect(ui.forumMessage, SIGNAL(copyAvailable(bool)), actionCut, SLOT(setEnabled(bool)));
    connect(ui.forumMessage, SIGNAL(copyAvailable(bool)), actionCopy, SLOT(setEnabled(bool)));

    connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(clipboardDataChanged()));

    QActionGroup *grp = new QActionGroup(this);
    connect(grp, SIGNAL(triggered(QAction *)), this, SLOT(textAlign(QAction *)));

    actionAlignLeft = new QAction(QIcon(":/images/textedit/textleft.png"), tr("&Left"), grp);
    actionAlignLeft->setShortcut(Qt::CTRL + Qt::Key_L);
    actionAlignLeft->setCheckable(true);
    actionAlignCenter = new QAction(QIcon(":/images/textedit/textcenter.png"), tr("C&enter"), grp);
    actionAlignCenter->setShortcut(Qt::CTRL + Qt::Key_E);
    actionAlignCenter->setCheckable(true);
    actionAlignRight = new QAction(QIcon(":/images/textedit/textright.png"), tr("&Right"), grp);
    actionAlignRight->setShortcut(Qt::CTRL + Qt::Key_R);
    actionAlignRight->setCheckable(true);
    actionAlignJustify = new QAction(QIcon(":/images/textedit/textjustify.png"), tr("&Justify"), grp);
    actionAlignJustify->setShortcut(Qt::CTRL + Qt::Key_J);
    actionAlignJustify->setCheckable(true);
    
    ui.comboStyle->addItem("Standard");
    ui.comboStyle->addItem("Bullet List (Disc)");
    ui.comboStyle->addItem("Bullet List (Circle)");
    ui.comboStyle->addItem("Bullet List (Square)");
    ui.comboStyle->addItem("Ordered List (Decimal)");
    ui.comboStyle->addItem("Ordered List (Alpha lower)");
    ui.comboStyle->addItem("Ordered List (Alpha upper)");
    connect(ui.comboStyle, SIGNAL(activated(int)),this, SLOT(textStyle(int)));
    
    connect(ui.comboFont, SIGNAL(activated(const QString &)), this, SLOT(textFamily(const QString &)));
    
    ui.comboSize->setEditable(true);

    QFontDatabase db;
    foreach(int size, db.standardSizes())
        ui.comboSize->addItem(QString::number(size));

    connect(ui.comboSize, SIGNAL(activated(const QString &)),this, SLOT(textSize(const QString &)));
    ui.comboSize->setCurrentIndex(ui.comboSize->findText(QString::number(QApplication::font().pointSize())));

    ui.boldbtn->setIcon(QIcon(QString(":/images/textedit/textbold.png")));
    ui.underlinebtn->setIcon(QIcon(QString(":/images/textedit/textunder.png")));
    ui.italicbtn->setIcon(QIcon(QString(":/images/textedit/textitalic.png")));
    ui.textalignmentbtn->setIcon(QIcon(QString(":/images/textedit/textcenter.png")));
    //ui.actionContactsView->setIcon(QIcon(":/images/contacts24.png"));
    //ui.actionSaveas->setIcon(QIcon(":/images/save24.png"));

    QMenu * alignmentmenu = new QMenu();
    alignmentmenu->addAction(actionAlignLeft);
    alignmentmenu->addAction(actionAlignCenter);
    alignmentmenu->addAction(actionAlignRight);
    alignmentmenu->addAction(actionAlignJustify);
    ui.textalignmentbtn->setMenu(alignmentmenu);
    
	QPixmap pxm(24,24);
	pxm.fill(Qt::black);
	ui.colorbtn->setIcon(pxm);

  newMsg();

}


void  CreateForumMsg::newMsg()
{
	/* clear all */
	ForumInfo fi;
	if (rsForums->getForumInfo(mForumId, fi))
	{
		ForumMsgInfo msg;

		QString name = QString::fromStdWString(fi.forumName);
		QString subj;
		if ((mParentId != "") && (rsForums->getForumMessage(
				mForumId, mParentId, msg)))
		{
			name += " In Reply to: ";
			name += QString::fromStdWString(msg.title);
			subj = "Re: " + QString::fromStdWString(msg.title);
		}

		ui.forumName->setText(name);
		ui.forumSubject->setText(subj);
	}

	ui.forumMessage->setText("");
}

void  CreateForumMsg::createMsg()
{
	QString name = ui.forumSubject->text();
	QString desc = ui.forumMessage->toHtml();


	ForumMsgInfo msgInfo;

	msgInfo.forumId = mForumId;
	msgInfo.threadId = "";
	msgInfo.parentId = mParentId;
	msgInfo.msgId = "";

	msgInfo.title = name.toStdWString();
	msgInfo.msg = desc.toStdWString();

	if ((msgInfo.msg == L"") && (msgInfo.title == L""))
		return; /* do nothing */

	rsForums->ForumMessageSend(msgInfo);

	close();
	return;
}


void  CreateForumMsg::cancelMsg()
{
	close();
	return;
	        
	RshareSettings config;
	config.saveWidgetInformation(this);
}

void CreateForumMsg::textBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(ui.boldbtn->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}

void CreateForumMsg::textUnderline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(ui.underlinebtn->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void CreateForumMsg::textItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(ui.italicbtn->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void CreateForumMsg::textFamily(const QString &f)
{
    QTextCharFormat fmt;
    fmt.setFontFamily(f);
    mergeFormatOnWordOrSelection(fmt);
}

void CreateForumMsg::textSize(const QString &p)
{
    QTextCharFormat fmt;
    fmt.setFontPointSize(p.toFloat());
    mergeFormatOnWordOrSelection(fmt);
}

void CreateForumMsg::textStyle(int styleIndex)
{
    QTextCursor cursor = ui.forumMessage->textCursor();

    if (styleIndex != 0) {
        QTextListFormat::Style style = QTextListFormat::ListDisc;

        switch (styleIndex) {
            default:
            case 1:
                style = QTextListFormat::ListDisc;
                break;
            case 2:
                style = QTextListFormat::ListCircle;
                break;
            case 3:
                style = QTextListFormat::ListSquare;
                break;
            case 4:
                style = QTextListFormat::ListDecimal;
                break;
            case 5:
                style = QTextListFormat::ListLowerAlpha;
                break;
            case 6:
                style = QTextListFormat::ListUpperAlpha;
                break;
        }

        cursor.beginEditBlock();

        QTextBlockFormat blockFmt = cursor.blockFormat();

        QTextListFormat listFmt;

        if (cursor.currentList()) {
            listFmt = cursor.currentList()->format();
        } else {
            listFmt.setIndent(blockFmt.indent() + 1);
            blockFmt.setIndent(0);
            cursor.setBlockFormat(blockFmt);
        }

        listFmt.setStyle(style);

        cursor.createList(listFmt);

        cursor.endEditBlock();
    } else {
        // ####
        QTextBlockFormat bfmt;
        bfmt.setObjectIndex(-1);
        cursor.mergeBlockFormat(bfmt);
    }
}

void CreateForumMsg::textColor()
{
    QColor col = QColorDialog::getColor(ui.forumMessage->textColor(), this);
    if (!col.isValid())
        return;
    QTextCharFormat fmt;
    fmt.setForeground(col);
    mergeFormatOnWordOrSelection(fmt);
    colorChanged(col);
}

void CreateForumMsg::textAlign(QAction *a)
{
    if (a == actionAlignLeft)
        ui.forumMessage->setAlignment(Qt::AlignLeft);
    else if (a == actionAlignCenter)
        ui.forumMessage->setAlignment(Qt::AlignHCenter);
    else if (a == actionAlignRight)
        ui.forumMessage->setAlignment(Qt::AlignRight);
    else if (a == actionAlignJustify)
        ui.forumMessage->setAlignment(Qt::AlignJustify);
}

void CreateForumMsg::currentCharFormatChanged(const QTextCharFormat &format)
{
    fontChanged(format.font());
    colorChanged(format.foreground().color());
}

void CreateForumMsg::cursorPositionChanged()
{
    alignmentChanged(ui.forumMessage->alignment());
}

void CreateForumMsg::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = ui.forumMessage->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);
    ui.forumMessage->mergeCurrentCharFormat(format);
}

void CreateForumMsg::fontChanged(const QFont &f)
{
    ui.comboFont->setCurrentIndex(ui.comboFont->findText(QFontInfo(f).family()));
    ui.comboSize->setCurrentIndex(ui.comboSize->findText(QString::number(f.pointSize())));
    ui.boldbtn->setChecked(f.bold());
    ui.italicbtn->setChecked(f.italic());
    ui.underlinebtn->setChecked(f.underline());
}

void CreateForumMsg::colorChanged(const QColor &c)
{
    QPixmap pix(16, 16);
    pix.fill(c);
    ui.colorbtn->setIcon(pix);
}

void CreateForumMsg::alignmentChanged(Qt::Alignment a)
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

void CreateForumMsg::clipboardDataChanged()
{
    actionPaste->setEnabled(!QApplication::clipboard()->text().isEmpty());
}

void CreateForumMsg::fileNew()
{
    if (maybeSave()) {
        ui.forumMessage->clear();
        //setCurrentFileName(QString());
    }
}

void CreateForumMsg::fileOpen()
{
    QString fn = QFileDialog::getOpenFileName(this, tr("Open File..."),
                                              QString(), tr("HTML-Files (*.htm *.html);;All Files (*)"));
    if (!fn.isEmpty())
        load(fn);
}

bool CreateForumMsg::fileSave()
{
    if (fileName.isEmpty())
        return fileSaveAs();

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly))
        return false;
    QTextStream ts(&file);
    ts.setCodec(QTextCodec::codecForName("UTF-8"));
    ts << ui.forumMessage->document()->toHtml("UTF-8");
    ui.forumMessage->document()->setModified(false);
    return true;
}

bool CreateForumMsg::fileSaveAs()
{
    QString fn = QFileDialog::getSaveFileName(this, tr("Save as..."),
                                              QString(), tr("HTML-Files (*.htm *.html);;All Files (*)"));
    if (fn.isEmpty())
        return false;
    setCurrentFileName(fn);
    return fileSave();
}

void CreateForumMsg::filePrint()
{
#ifndef QT_NO_PRINTER
    QPrinter printer(QPrinter::HighResolution);
    printer.setFullPage(true);
    QPrintDialog *dlg = new QPrintDialog(&printer, this);
    if (ui.forumMessage->textCursor().hasSelection())
        dlg->addEnabledOption(QAbstractPrintDialog::PrintSelection);
    dlg->setWindowTitle(tr("Print Document"));
    if (dlg->exec() == QDialog::Accepted) {
        ui.forumMessage->print(&printer);
    }
    delete dlg;
#endif
}

/*void TextEdit::filePrintPreview()
{
    PrintPreview *preview = new PrintPreview(textEdit->document(), this);
    preview->setWindowModality(Qt::WindowModal);
    preview->setAttribute(Qt::WA_DeleteOnClose);
    preview->show();
}*/

void CreateForumMsg::filePrintPdf()
{
#ifndef QT_NO_PRINTER
    QString fileName = QFileDialog::getSaveFileName(this, "Export PDF",
                                                    QString(), "*.pdf");
    if (!fileName.isEmpty()) {
        if (QFileInfo(fileName).suffix().isEmpty())
            fileName.append(".pdf");
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(fileName);
        ui.forumMessage->document()->print(&printer);
    }
#endif
}

void CreateForumMsg::setCurrentFileName(const QString &fileName)
{
    this->fileName = fileName;
    ui.forumMessage->document()->setModified(false);

    setWindowModified(false);
}

bool CreateForumMsg::load(const QString &f)
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
        ui.forumMessage->setHtml(str);
    } else {
        str = QString::fromLocal8Bit(data);
        ui.forumMessage->setPlainText(str);
    }

    setCurrentFileName(f);
    return true;
}


bool CreateForumMsg::maybeSave()
{
    if (!ui.forumMessage->document()->isModified())
        return true;
    if (fileName.startsWith(QLatin1String(":/")))
        return true;
    QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, tr("Save Message"),
                               tr("Forum Message has not been Sent.\n"
                                  "Do you want to save forum message ?"),
                               QMessageBox::Save | QMessageBox::Discard
                               | QMessageBox::Cancel);
    if (ret == QMessageBox::Save)
        return fileSave();
    else if (ret == QMessageBox::Cancel)
        return false;
    return true;
}

void CreateForumMsg::setupFileActions()
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

    /*a = new QAction(QIcon(":/images/textedit/fileprint.png"), tr("Print Preview..."), this);
    connect(a, SIGNAL(triggered()), this, SLOT(filePrintPreview()));
    menu->addAction(a);*/

    a = new QAction(QIcon(":/images/textedit/exportpdf.png"), tr("&Export PDF..."), this);
    a->setShortcut(Qt::CTRL + Qt::Key_D);
    connect(a, SIGNAL(triggered()), this, SLOT(filePrintPdf()));
    menu->addAction(a);

    menu->addSeparator();

    a = new QAction(tr("&Quit"), this);
    a->setShortcut(Qt::CTRL + Qt::Key_Q);
    connect(a, SIGNAL(triggered()), this, SLOT(close()));
    menu->addAction(a);
}

void CreateForumMsg::setupEditActions()
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

void CreateForumMsg::setupViewActions()
{
    QMenu *menu = new QMenu(tr("&View"), this);
    menuBar()->addMenu(menu);

    //QAction *a;

    /*a = new QAction(QIcon(""), tr("&Contacts Sidebar"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleContacts()));
    menu->addAction(a);*/

}

void CreateForumMsg::setupInsertActions()
{
    QMenu *menu = new QMenu(tr("&Insert"), this);
    menuBar()->addMenu(menu);

    //QAction *a;

    /*a = new QAction(QIcon(""), tr("&Image"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(addImage()));
    menu->addAction(a);*/

}

