/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,2007 RetroShare Team
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

#include "rshare.h"
#include "MessageComposer.h"

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsmsgs.h"

#include <gui/settings/rsharesettings.h>
#include "gui/feeds/AttachFileItem.h"
#include "textformat.h"
#include "util/misc.h"

#include <sstream>
#include <QtGui>



/** Constructor */
MessageComposer::MessageComposer(QWidget *parent, Qt::WFlags flags)
: QMainWindow(parent, flags), mCheckAttachment(true)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);
    
    setupFileActions();
    setupEditActions();
    setupViewActions();
    setupInsertActions();
  
    Settings->loadWidgetInformation(this);

    setAttribute ( Qt::WA_DeleteOnClose, true );
   
    // connect up the buttons. 
    connect( ui.actionSend, SIGNAL( triggered (bool)), this, SLOT( sendMessage( ) ) );
    //connect( ui.actionReply, SIGNAL( triggered (bool)), this, SLOT( replyMessage( ) ) );
    connect(ui.boldbtn, SIGNAL(clicked()), this, SLOT(textBold()));
    connect(ui.underlinebtn, SIGNAL(clicked()), this, SLOT(textUnderline()));
    connect(ui.italicbtn, SIGNAL(clicked()), this, SLOT(textItalic()));
    connect(ui.colorbtn, SIGNAL(clicked()), this, SLOT(textColor()));
    connect(ui.imagebtn, SIGNAL(clicked()), this, SLOT(addImage()));
    //connect(ui.linkbtn, SIGNAL(clicked()), this, SLOT(insertLink()));
    connect(ui.actionContactsView, SIGNAL(triggered()), this, SLOT(toggleContacts()));
    connect(ui.actionSaveas, SIGNAL(triggered()), this, SLOT(saveasDraft()));
    connect(ui.actionAttach, SIGNAL(triggered()), this, SLOT(attachFile()));
  
    connect(ui.sizeincreaseButton, SIGNAL (clicked()), this, SLOT (fontSizeIncrease()));
    connect(ui.sizedecreaseButton, SIGNAL (clicked()), this, SLOT (fontSizeDecrease()));
    connect(ui.actionQuote, SIGNAL(triggered()), this, SLOT(blockQuote()));
    connect(ui.codeButton, SIGNAL (clicked()), this, SLOT (toggleCode()));
    connect(ui.splitPostButton, SIGNAL (clicked()), this, SLOT (addPostSplitter()));

    connect(ui.msgText, SIGNAL( checkSpellingChanged( bool ) ), this, SLOT( spellChecking( bool ) ) );
  
    connect(ui.msgText, SIGNAL(currentCharFormatChanged(const QTextCharFormat &)),
            this, SLOT(currentCharFormatChanged(const QTextCharFormat &)));
    connect(ui.msgText, SIGNAL(cursorPositionChanged()),
            this, SLOT(cursorPositionChanged()));
            
    connect(ui.msgText->document(), SIGNAL(modificationChanged(bool)),
            actionSave, SLOT(setEnabled(bool)));
    connect(ui.msgText->document(), SIGNAL(modificationChanged(bool)),
            this, SLOT(setWindowModified(bool)));
    connect(ui.msgText->document(), SIGNAL(undoAvailable(bool)),
            actionUndo, SLOT(setEnabled(bool)));
    connect(ui.msgText->document(), SIGNAL(redoAvailable(bool)),
            actionRedo, SLOT(setEnabled(bool)));

    setWindowModified(ui.msgText->document()->isModified());
    actionSave->setEnabled(ui.msgText->document()->isModified());
    actionUndo->setEnabled(ui.msgText->document()->isUndoAvailable());
    actionRedo->setEnabled(ui.msgText->document()->isRedoAvailable());

    connect(actionUndo, SIGNAL(triggered()), ui.msgText, SLOT(undo()));
    connect(actionRedo, SIGNAL(triggered()), ui.msgText, SLOT(redo()));

    actionCut->setEnabled(false);
    actionCopy->setEnabled(false);

    connect(actionCut, SIGNAL(triggered()), ui.msgText, SLOT(cut()));
    connect(actionCopy, SIGNAL(triggered()), ui.msgText, SLOT(copy()));
    connect(actionPaste, SIGNAL(triggered()), ui.msgText, SLOT(paste()));

    connect(ui.msgText, SIGNAL(copyAvailable(bool)), actionCut, SLOT(setEnabled(bool)));
    connect(ui.msgText, SIGNAL(copyAvailable(bool)), actionCopy, SLOT(setEnabled(bool)));

    connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(clipboardDataChanged()));

    connect(ui.msgSendList, SIGNAL(itemChanged( QTreeWidgetItem *, int ) ), this, SLOT(togglePersonItem( QTreeWidgetItem *, int ) ));

    connect(ui.msgFileList, SIGNAL(itemChanged( QTreeWidgetItem *, int ) ),
      this, SLOT(toggleRecommendItem( QTreeWidgetItem *, int ) ));
    
    /* hide the Tree +/- */
    ui.msgSendList -> setRootIsDecorated( false );
    ui.msgFileList -> setRootIsDecorated( false );
  
    /* to hide the header  */
    //ui.msgSendList->header()->hide(); 
  
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
    
    ui.textalignmentbtn->setIcon(QIcon(QString(":/images/textedit/textcenter.png"))); 
       
    QMenu * alignmentmenu = new QMenu();
    alignmentmenu->addAction(actionAlignLeft);
    alignmentmenu->addAction(actionAlignCenter);
    alignmentmenu->addAction(actionAlignRight);
    alignmentmenu->addAction(actionAlignJustify);
    ui.textalignmentbtn->setMenu(alignmentmenu);
    
    QPixmap pxm(24,24);
    pxm.fill(Qt::black);
    ui.colorbtn->setIcon(pxm);
	
	  /* Set header resize modes and initial section sizes */
    ui.msgFileList->setColumnCount(5);
     
    QHeaderView * _smheader = ui.msgFileList->header () ;   
  
    _smheader->resizeSection ( 0, 200 );
    _smheader->resizeSection ( 1, 60 );
    _smheader->resizeSection ( 2, 60 );
    _smheader->resizeSection ( 3, 220 );
    _smheader->resizeSection ( 4, 10 );
    
    QPalette palette = QApplication::palette();
    codeBackground = palette.color( QPalette::Active, QPalette::Midlight );


  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void MessageComposer::closeEvent (QCloseEvent * event)
{
    bool bClose = true;

    /* Save to Drafts? */

    if (ui.msgText->document()->isModified()) {
        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, tr("Save Message"),
                                   tr("Message has not been Sent.\n"
                                      "Do you want to save message to draft box?"),
                                   QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        switch (ret) {
        case QMessageBox::Yes:
            sendMessage_internal(true);
            break;
        case QMessageBox::Cancel:
            bClose = false;
            break;
        default:
            break;
        }
    }

    if (bClose) {
        Settings->saveWidgetInformation(this);

        //=== uncheck all repecient's boxes =======
        QTreeWidget *sendWidget = ui.msgSendList;

        for (int i=0;i<sendWidget->topLevelItemCount();++i)
            sendWidget->topLevelItem(i)->setCheckState(0,Qt::Unchecked) ;

        QMainWindow::closeEvent(event);
    } else {
        event->ignore();
    }
}

void  MessageComposer::insertSendList()
{
	if (!rsPeers)
	{
		/* not ready yet! */
		return;
	}
		
	std::list<std::string> peers;
	std::list<std::string>::iterator it;
		
	rsPeers->getFriendList(peers);
		
        /* get a link to the table */
        QTreeWidget *sendWidget = ui.msgSendList;

        QList<QTreeWidgetItem *> items;
	for(it = peers.begin(); it != peers.end(); it++)
	{
		
		RsPeerDetails detail;
		if (!rsPeers->getPeerDetails(*it, detail))
		{
			continue; /* BAD */
		}
		
		/* make a widget per friend */
           	QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);


		/* add all the labels */
		/* (0) Person */
                item -> setText(0, QString::fromStdString(detail.name) + " - " + QString::fromStdString(detail.location));
                if (detail.state & RS_PEER_STATE_CONNECTED) {
                    item -> setTextColor(0,(Qt::darkBlue));
                }
		/* () Org */
		//item -> setText(1, QString::fromStdString(detail.org));
		/* () Location */
		//item -> setText(2, QString::fromStdString(detail.location));
		/* () Country */
		//item -> setText(3, QString::fromStdString(detail.email));
		/* () Id */
		item -> setText(1, QString::fromStdString(detail.id));

		item -> setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		item -> setCheckState(0, Qt::Unchecked);

		if (rsicontrol->IsInMsg(detail.id))
		{
			item -> setCheckState(0, Qt::Checked);
		}
		else
		{
			item -> setCheckState(0, Qt::Unchecked);
		}

		/* add to the list */
		items.append(item);
	}

        /* remove old items ??? */
	sendWidget->clear();
	sendWidget->setColumnCount(1);

	/* add the items in! */
	sendWidget->insertTopLevelItems(0, items);

	sendWidget->update(); /* update display */
}


/* Utility Fns */
/***
RsCertId getSenderRsCertId(QTreeWidgetItem *i)
{
	RsCertId id = (i -> text(3)).toStdString();
	return id;
}
***/


	/* get the list of peers from the RsIface.  */
void  MessageComposer::insertFileList(const std::list<DirDetails>& dir_info)
{
	std::list<FileInfo> files_info;
	std::list<DirDetails>::const_iterator it;

	/* convert dir_info to files_info */
	for(it = dir_info.begin(); it != dir_info.end(); it++)
	{
		FileInfo info ;
		info.fname = it->name ;
		info.hash = it->hash ;
		info.rank = 0;//it->rank ;
		info.size = it->count ;
		info.inRecommend = true;
		files_info.push_back(info) ;
	}

	insertFileList(files_info);
}

void  MessageComposer::insertFileList(const std::list<FileInfo>& files_info)
{
//	rsiface->lockData();   /* Lock Interface */

	_recList.clear() ;

//	const std::list<FileInfo> &recList = rsiface->getRecommendList();
	std::list<FileInfo>::const_iterator it;

	/* get a link to the table */
	QTreeWidget *tree = ui.msgFileList;

	tree->clear();
	tree->setColumnCount(5);

	QList<QTreeWidgetItem *> items;
	for(it = files_info.begin(); it != files_info.end(); it++)
	{
		_recList.push_back(*it) ;

		/* make a widget per person */
		QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

		item->setText(0, QString::fromStdString(it->fname));			/* (0) Filename */
		item->setText(1, misc::friendlyUnit(it->size));			 		/* (1) Size */
		item->setText(2, QString::number(0)) ;//it->rank));
		item->setText(3, QString::fromStdString(it->hash));
		item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		item->setCheckState(0, it->inRecommend ? Qt::Checked : Qt::Unchecked);

		/* add to the list */
		items.append(item);
	}

	/* add the items in! */
	tree->insertTopLevelItems(0, items);

//	rsiface->unlockData(); /* UnLock Interface */

	tree->update(); /* update display */
}

void  MessageComposer::newMsg(std::string msgId /*= ""*/)
{
    /* clear all */
    QString titlestring = ui.titleEdit->text();

    setWindowTitle(tr("Compose: ") + titlestring );
    ui.titleEdit->setText("No Title");

    ui.msgText->setText("");

    /* worker fns */
    insertSendList();

    m_sMsgId = msgId;

    if (m_sMsgId.empty() == false) {
        // fill existing message
        MessageInfo msgInfo;
        if (!rsMsgs->getMessage(m_sMsgId, msgInfo)) {
            std::cerr << "MessageComposer::newMsg() Couldn't find Msg" << std::endl;
            m_sMsgId.clear();
            return;
        }

        insertTitleText( QString::fromStdWString(msgInfo.title).toStdString());
        setWindowTitle( tr ("Compose: ") + QString::fromStdWString(msgInfo.title));

        insertMsgText(QString::fromStdWString(msgInfo.msg).toStdString());

        insertFileList(msgInfo.files);

        std::list<std::string>::iterator to;
        for (to = msgInfo.msgto.begin(); to != msgInfo.msgto.end(); to++ ) {
            addRecipient(*to) ;
        }

        ui.msgText->document()->setModified(false);
    }
}

void  MessageComposer::insertTitleText(std::string title)
{
	ui.titleEdit->setText(QString::fromStdString(title));
}

void  MessageComposer::insertPastedText(std::string msg)
{
    std::string::size_type i=0 ;
    while( (i=msg.find_first_of('\n',i+1)) < msg.size())
        msg.replace(i,1,std::string("\n<BR/>> ")) ;

    ui.msgText->setHtml(QString("<HTML><font color=\"blue\">")+QString::fromStdString(std::string("> ") + msg)+"</font><br/><br/></HTML>") ;

    ui.msgText->setFocus( Qt::OtherFocusReason );

    QTextCursor c =  ui.msgText->textCursor();
    c.movePosition(QTextCursor::End);
    ui.msgText->setTextCursor(c);

    ui.msgText->document()->setModified(true);
}

void  MessageComposer::insertForwardPastedText(std::string msg)
{
    std::string::size_type i=0 ;
    while( (i=msg.find_first_of('\n',i+1)) < msg.size())
        msg.replace(i,1,std::string("\n<BR/>> ")) ;

    ui.msgText->setHtml(QString("<HTML><blockquote [type=cite]><font color=\"blue\">")+QString::fromStdString(std::string("") + msg)+"</font><br/><br/></blockquote></HTML>") ;

    ui.msgText->setFocus( Qt::OtherFocusReason );

    QTextCursor c =  ui.msgText->textCursor();
    c.movePosition(QTextCursor::End);
    ui.msgText->setTextCursor(c);

    ui.msgText->document()->setModified(true);
}

void  MessageComposer::insertMsgText(std::string msg)
{
    ui.msgText->setText(QString::fromStdString(msg));

    ui.msgText->setFocus( Qt::OtherFocusReason );

    QTextCursor c =  ui.msgText->textCursor();
    c.movePosition(QTextCursor::End);
    ui.msgText->setTextCursor(c);

    ui.msgText->document()->setModified(true);
}

void  MessageComposer::insertHtmlText(std::string msg)
{
    ui.msgText->setHtml(QString("<a href='") + QString::fromStdString(std::string(msg + "'> ") ) + QString::fromStdString(std::string(msg)) + "</a>") ;

    ui.msgText->document()->setModified(true);
}

void  MessageComposer::sendMessage()
{
    sendMessage_internal(false);
    close();
}

void MessageComposer::sendMessage_internal(bool bDraftbox)
{
    /* construct a message */
    MessageInfo mi;

    mi.title = ui.titleEdit->text().toStdWString();
    mi.msg =   ui.msgText->toHtml().toStdWString();

    rsiface->lockData();   /* Lock Interface */

//	const std::list<FileInfo>& recList = rsiface->getRecommendList();
    for(std::list<FileInfo>::const_iterator it(_recList.begin()); it != _recList.end(); ++it)
        if (it -> inRecommend)
            mi.files.push_back(*it);

    rsiface->unlockData(); /* UnLock Interface */

    /* get the ids from the send list */
    std::list<std::string> peers;
    std::list<std::string>::iterator iit;

    rsPeers->getFriendList(peers);

    for(iit = peers.begin(); iit != peers.end(); iit++) {
        if (rsicontrol->IsInMsg(*iit)) {
            mi.msgto.push_back(*iit);
        }
    }

    if (bDraftbox) {
        mi.msgId = m_sMsgId;
        rsMsgs->MessageToDraft(mi);
    } else {
        rsMsgs->MessageSend(mi);
    }

    ui.msgText->document()->setModified(false);
}

void  MessageComposer::cancelMessage()
{
	close();
	return;
}



/* Toggling .... Check Boxes.....
 * This is dependent on whether we are a 
 *
 * Chan or Msg Dialog.
 */

void MessageComposer::addRecipient(std::string id) 
{
	QTreeWidget *sendWidget = ui.msgSendList;

	for(int i=0;i<sendWidget->topLevelItemCount();++i)
		if(sendWidget->topLevelItem(i)->text(1).toStdString() == id)
			sendWidget->topLevelItem(i)->setCheckState(0,Qt::Checked) ;
		//else
			//sendWidget->topLevelItem(i)->setCheckState(0,Qt::Unchecked) ;
}

/* First the Msg (People) ones */
void MessageComposer::togglePersonItem( QTreeWidgetItem *item, int col )
{
        //std::cerr << "TogglePersonItem()" << std::endl;

        /* extract id */
        std::string id = (item -> text(1)).toStdString();

        /* get state */
        bool inMsg = (Qt::Checked == item -> checkState(0)); /* alway column 0 */

        /* call control fns */

        rsicontrol -> SetInMsg(id, inMsg);
        return;
}

void MessageComposer::toggleRecommendItem( QTreeWidgetItem *item, int col )
{
        //std::cerr << "ToggleRecommendItem()" << std::endl;

        /* extract name */
        std::string id = (item -> text(0)).toStdString();

        /* get state */
        bool inRecommend = (Qt::Checked == item -> checkState(0)); /* alway column 0 */

        /* call control fns */

        rsicontrol -> SetInRecommend(id, inRecommend);
        return;
}


void MessageComposer::setupFileActions()
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
    connect(a, SIGNAL(triggered()), this, SLOT(saveasDraft()));
    a->setEnabled(false);
    menu->addAction(a);

    a = new QAction(tr("Save &As File"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
    menu->addAction(a);
    
    a = new QAction(tr("Save &As Draft"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(saveasDraft()));
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

void MessageComposer::setupEditActions()
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

void MessageComposer::setupViewActions()
{
    QMenu *menu = new QMenu(tr("&View"), this);
    menuBar()->addMenu(menu);

    QAction *a;

    a = new QAction(QIcon(""), tr("&Contacts Sidebar"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleContacts()));
    menu->addAction(a);

}

void MessageComposer::setupInsertActions()
{
    QMenu *menu = new QMenu(tr("&Insert"), this);
    menuBar()->addMenu(menu);

    QAction *a;

    a = new QAction(QIcon(""), tr("&Image"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(addImage()));
    menu->addAction(a);

}

void MessageComposer::textBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(ui.boldbtn->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}

void MessageComposer::textUnderline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(ui.underlinebtn->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void MessageComposer::textItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(ui.italicbtn->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void MessageComposer::textFamily(const QString &f)
{
    QTextCharFormat fmt;
    fmt.setFontFamily(f);
    mergeFormatOnWordOrSelection(fmt);
}

void MessageComposer::textSize(const QString &p)
{
    qreal pointSize = p.toFloat();
    if (p.toFloat() > 0) {
        QTextCharFormat fmt;
        fmt.setFontPointSize(pointSize);
        mergeFormatOnWordOrSelection(fmt);
    }
}

void MessageComposer::textStyle(int styleIndex)
{
    QTextCursor cursor = ui.msgText->textCursor();

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

void MessageComposer::textColor()
{
    QColor col = QColorDialog::getColor(ui.msgText->textColor(), this);
    if (!col.isValid())
        return;
    QTextCharFormat fmt;
    fmt.setForeground(col);
    mergeFormatOnWordOrSelection(fmt);
    colorChanged(col);
}

void MessageComposer::textAlign(QAction *a)
{
    if (a == actionAlignLeft)
        ui.msgText->setAlignment(Qt::AlignLeft);
    else if (a == actionAlignCenter)
        ui.msgText->setAlignment(Qt::AlignHCenter);
    else if (a == actionAlignRight)
        ui.msgText->setAlignment(Qt::AlignRight);
    else if (a == actionAlignJustify)
        ui.msgText->setAlignment(Qt::AlignJustify);
}

void MessageComposer::currentCharFormatChanged(const QTextCharFormat &format)
{
    fontChanged(format.font());
    colorChanged(format.foreground().color());
}

void MessageComposer::cursorPositionChanged()
{
    alignmentChanged(ui.msgText->alignment());
}

void MessageComposer::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = ui.msgText->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);
    ui.msgText->mergeCurrentCharFormat(format);
}

void MessageComposer::fontChanged(const QFont &f)
{
    ui.comboFont->setCurrentIndex(ui.comboFont->findText(QFontInfo(f).family()));
    ui.comboSize->setCurrentIndex(ui.comboSize->findText(QString::number(f.pointSize())));
    ui.boldbtn->setChecked(f.bold());
    ui.italicbtn->setChecked(f.italic());
    ui.underlinebtn->setChecked(f.underline());
}

void MessageComposer::colorChanged(const QColor &c)
{
    QPixmap pix(16, 16);
    pix.fill(c);
    ui.colorbtn->setIcon(pix);
}

void MessageComposer::alignmentChanged(Qt::Alignment a)
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

void MessageComposer::clipboardDataChanged()
{
    actionPaste->setEnabled(!QApplication::clipboard()->text().isEmpty());
}

void MessageComposer::fileNew()
{
    if (maybeSave()) {
        ui.msgText->clear();
        //setCurrentFileName(QString());
    }
}

void MessageComposer::fileOpen()
{
    QString fn = QFileDialog::getOpenFileName(this, tr("Open File..."),
                                              QString(), tr("HTML-Files (*.htm *.html);;All Files (*)"));
    if (!fn.isEmpty())
        load(fn);
}

bool MessageComposer::fileSave()
{
    if (fileName.isEmpty())
        return fileSaveAs();

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly))
        return false;
    QTextStream ts(&file);
    ts.setCodec(QTextCodec::codecForName("UTF-8"));
    ts << ui.msgText->document()->toHtml("UTF-8");
    ui.msgText->document()->setModified(false);
    return true;
}

bool MessageComposer::fileSaveAs()
{
    QString fn = QFileDialog::getSaveFileName(this, tr("Save as..."),
                                              QString(), tr("HTML-Files (*.htm *.html);;All Files (*)"));
    if (fn.isEmpty())
        return false;
    setCurrentFileName(fn);
    return fileSave();
}

void MessageComposer::saveasDraft()
{
    sendMessage_internal(true);
}


void MessageComposer::filePrint()
{
#ifndef QT_NO_PRINTER
    QPrinter printer(QPrinter::HighResolution);
    printer.setFullPage(true);
    QPrintDialog *dlg = new QPrintDialog(&printer, this);
    if (ui.msgText->textCursor().hasSelection())
        dlg->addEnabledOption(QAbstractPrintDialog::PrintSelection);
    dlg->setWindowTitle(tr("Print Document"));
    if (dlg->exec() == QDialog::Accepted) {
        ui.msgText->print(&printer);
    }
    delete dlg;
#endif
}

void MessageComposer::filePrintPdf()
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
        ui.msgText->document()->print(&printer);
    }
#endif
}

void MessageComposer::setCurrentFileName(const QString &fileName)
{
    this->fileName = fileName;
    ui.msgText->document()->setModified(false);

    setWindowModified(false);
}

bool MessageComposer::load(const QString &f)
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
        ui.msgText->setHtml(str);
    } else {
        str = QString::fromLocal8Bit(data);
        ui.msgText->setPlainText(str);
    }

    setCurrentFileName(f);
    return true;
}


bool MessageComposer::maybeSave()
{
    if (!ui.msgText->document()->isModified())
        return true;
    if (fileName.startsWith(QLatin1String(":/")))
        return true;
    QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, tr("Save Message"),
                               tr("Message has not been Sent.\n"
                                  "Do you want to save message ?"),
                               QMessageBox::Save | QMessageBox::Discard
                               | QMessageBox::Cancel);
    if (ret == QMessageBox::Save)
        return fileSave();
    else if (ret == QMessageBox::Cancel)
        return false;
    return true;
}


void MessageComposer::toggleContacts()
{
	ui.contactsdockWidget->setVisible(!ui.contactsdockWidget->isVisible());
}

void  MessageComposer::addImage()
{
  
	QString fileimg = QFileDialog::getOpenFileName( this, tr( "Choose Image" ), 
	QString(setter.value("LastDir").toString()) ,tr("Image Files supported (*.png *.jpeg *.jpg *.gif)"));
    
    if ( fileimg.isEmpty() ) {
     return; 
    }
    
    QImage base(fileimg);
    
    QString pathimage = fileimg.left(fileimg.lastIndexOf("/"))+"/";
    setter.setValue("LastDir",pathimage);   
    
    Create_New_Image_Tag(fileimg);
}

void  MessageComposer::Create_New_Image_Tag( const QString urlremoteorlocal )
{
   /*if (image_extension(urlremoteorlocal)) {*/
       QString subtext = QString("<p><img src=\"%1\" />").arg(urlremoteorlocal);
               ///////////subtext.append("<br/><br/>Description on image.</p>");
       QTextDocumentFragment fragment = QTextDocumentFragment::fromHtml(subtext);
       ui.msgText->textCursor().insertFragment(fragment);
       //emit statusMessage(QString("Image new :").arg(urlremoteorlocal));
   //}
}

void MessageComposer::fontSizeIncrease()
{
    if ( !( ui.msgText->textCursor().blockFormat().hasProperty( TextFormat::HtmlHeading ) &&
        ui.msgText->textCursor().blockFormat().intProperty( TextFormat::HtmlHeading ) ) ) {
        QTextCharFormat format;
        int idx = ui.msgText->currentCharFormat().intProperty( QTextFormat::FontSizeAdjustment );
        if ( idx < 3 ) {
            format.setProperty( QTextFormat::FontSizeAdjustment, QVariant( ++idx ) );
            ui.msgText->textCursor().mergeCharFormat( format );
        }
    }
    ui.msgText->setFocus( Qt::OtherFocusReason );
}

void MessageComposer::fontSizeDecrease()
{
    if ( !( ui.msgText->textCursor().blockFormat().hasProperty( TextFormat::HtmlHeading ) &&
        ui.msgText->textCursor().blockFormat().intProperty( TextFormat::HtmlHeading ) ) ) {
        QTextCharFormat format;
        int idx = ui.msgText->currentCharFormat().intProperty( QTextFormat::FontSizeAdjustment );
        if ( idx > -1 ) {
            format.setProperty( QTextFormat::FontSizeAdjustment, QVariant( --idx ) );
            ui.msgText->textCursor().mergeCharFormat( format );
        }
    }
    ui.msgText->setFocus( Qt::OtherFocusReason );
}

void MessageComposer::blockQuote()
{
    QTextBlockFormat blockFormat = ui.msgText->textCursor().blockFormat();
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
    ui.msgText->textCursor().mergeBlockFormat( f );
}

void MessageComposer::toggleCode()
{
    static QString preFontFamily;

    QTextCharFormat charFormat = ui.msgText->currentCharFormat();
    QTextCharFormat f;
    
    if ( charFormat.hasProperty( TextFormat::HasCodeStyle ) &&
         charFormat.boolProperty( TextFormat::HasCodeStyle ) ) {
        f.setProperty( TextFormat::HasCodeStyle, QVariant( false ) );
        f.setBackground( defaultCharFormat.background() );
        f.setFontFamily( preFontFamily );
        ui.msgText->textCursor().mergeCharFormat( f );

    } else {
        preFontFamily = ui.msgText->fontFamily();
        f.setProperty( TextFormat::HasCodeStyle, QVariant( true ) );
        f.setBackground( codeBackground );
        f.setFontFamily( "Dejavu Sans Mono" );
        ui.msgText->textCursor().mergeCharFormat( f );
    }
    ui.msgText->setFocus( Qt::OtherFocusReason );
}

void MessageComposer::addPostSplitter()
{
    QTextBlockFormat f = ui.msgText->textCursor().blockFormat();
    QTextBlockFormat f1 = f;

    f.setProperty( TextFormat::IsHtmlTagSign, true );
    f.setProperty( QTextFormat::BlockTrailingHorizontalRulerWidth, 
             QTextLength( QTextLength::PercentageLength, 80 ) );
    if ( ui.msgText->textCursor().block().text().isEmpty() ) {
        ui.msgText->textCursor().mergeBlockFormat( f );
    } else {
        ui.msgText->textCursor().insertBlock( f );
    }
    ui.msgText->textCursor().insertBlock( f1 );
}

void MessageComposer::attachFile()
{
	// select a file
	QString qfile = QFileDialog::getOpenFileName(this, tr("Add Extra File"), "", "", 0,
				QFileDialog::DontResolveSymlinks);
	std::string filePath = qfile.toStdString();
	if (filePath != "")
	{
	    MessageComposer::addAttachment(filePath);
	}
}

void MessageComposer::addAttachment(std::string filePath) {
	    /* add a AttachFileItem to the attachment section */
	    std::cerr << "MessageComposer::addFile() hashing file.";
	    std::cerr << std::endl;

	    /* add widget in for new destination */
	    AttachFileItem *file = new AttachFileItem(filePath);
	    //file->
	    	    
	    ui.verticalLayout->addWidget(file, 1, 0);

	    //when the file is local or is finished hashing, call the fileHashingFinished method to send a chat message
	    if (file->getState() == AFI_STATE_LOCAL) {
		fileHashingFinished(file);
	    } else {
		QObject::connect(file,SIGNAL(fileFinished(AttachFileItem *)),this, SLOT(fileHashingFinished(AttachFileItem *))) ;
	    }
	mAttachments.push_back(file); 
	   
	if (mCheckAttachment)
	{
		checkAttachmentReady();
	}
	    
}

void MessageComposer::fileHashingFinished(AttachFileItem* file) {
	std::cerr << "MessageComposer::fileHashingFinished() started.";
	std::cerr << std::endl;

	//check that the file is ok tos end
	if (file->getState() == AFI_STATE_ERROR) {
	#ifdef CHAT_DEBUG
		    std::cerr << "MessageComposer::fileHashingFinished error file is not hashed.";
	#endif
	    return;
	}

	//convert fileSize from uint_64 to string for html link
	char fileSizeChar [100];
	sprintf(fileSizeChar, "%lld", file->FileSize());
	std::string fileSize = *(&fileSizeChar);

	std::string mesgString = "<a href='retroshare://file|" + (file->FileName()) + "|" + fileSize + "|" + (file->FileHash()) + "'>" 
	+ "retroshare://file|" + (file->FileName()) + "|" + fileSize +  "|" + (file->FileHash())  + "</a>" + "<br>";
#ifdef CHAT_DEBUG
	    std::cerr << "MessageComposer::anchorClicked mesgString : " << mesgString << std::endl;
#endif

	ui.msgText->textCursor().insertHtml(QString::fromStdString(mesgString));
	ui.msgText->setFocus( Qt::OtherFocusReason );
}

void MessageComposer::checkAttachmentReady()
{
	std::list<AttachFileItem *>::iterator fit;

	mCheckAttachment = false;

	for(fit = mAttachments.begin(); fit != mAttachments.end(); fit++)
	{
		if (!(*fit)->isHidden())
		{
			if (!(*fit)->ready())
			{
				/* 
				 */
				ui.actionSend->setEnabled(false);
				break;


				return;
			}
		}
	}

	if (fit == mAttachments.end())
	{
		ui.actionSend->setEnabled(true);
	}

	/* repeat... */
	int msec_rate = 1000;
	QTimer::singleShot( msec_rate, this, SLOT(checkAttachmentReady(void)));
}
