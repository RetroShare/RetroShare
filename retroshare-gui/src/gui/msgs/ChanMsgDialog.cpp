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
#include "ChanMsgDialog.h"

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsmsgs.h"

#include <config/rsharesettings.h>

#include <sstream>

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
ChanMsgDialog::ChanMsgDialog(bool msg, QWidget *parent, Qt::WFlags flags)
: mIsMsg(msg), QMainWindow(parent, flags)
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
  
  //connect( ui.channelstreeView, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( channelstreeViewCostumPopupMenu( QPoint ) ) );
  //
 
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
  connect(ui.actionSaveas, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
  
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

  /* if Msg */
  if (mIsMsg)
  {
    connect(ui.msgSendList, SIGNAL(itemChanged( QTreeWidgetItem *, int ) ),
      this, SLOT(togglePersonItem( QTreeWidgetItem *, int ) ));
  }
  else
  {
    connect(ui.msgSendList, SIGNAL(itemChanged( QTreeWidgetItem *, int ) ),
      this, SLOT(toggleChannelItem( QTreeWidgetItem *, int ) ));
  }

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
    
    ui.boldbtn->setIcon(QIcon(QString(":/images/textedit/textbold.png")));
    ui.underlinebtn->setIcon(QIcon(QString(":/images/textedit/textitalic.png")));
    ui.italicbtn->setIcon(QIcon(QString(":/images/textedit/textunder.png")));
    ui.textalignmentbtn->setIcon(QIcon(QString(":/images/textedit/textcenter.png")));
    ui.imagebtn->setIcon(QIcon(QString(":/images/lphoto24.png")));
    ui.actionContactsView->setIcon(QIcon(":/images/contacts24.png"));
    ui.actionSaveas->setIcon(QIcon(":/images/save24.png"));
    
    /* ToolTips */
    ui.actionSend->setStatusTip(tr("Send this message now"));
    ui.actionContactsView->setStatusTip(tr("Toggle Contacts View"));
    ui.actionSaveas->setStatusTip(tr("Save this message"));
    
       
    QMenu * alignmentmenu = new QMenu();
    alignmentmenu->addAction(actionAlignLeft);
    alignmentmenu->addAction(actionAlignCenter);
    alignmentmenu->addAction(actionAlignRight);
    alignmentmenu->addAction(actionAlignJustify);
    ui.textalignmentbtn->setMenu(alignmentmenu);
    
	QPixmap pxm(24,24);
	pxm.fill(Qt::black);
	ui.colorbtn->setIcon(pxm);

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}


void ChanMsgDialog::channelstreeViewCostumPopupMenu( QPoint point )
{

      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

      deletechannelAct = new QAction( tr( "Delete Channel" ), this );
      connect( deletechannelAct , SIGNAL( triggered() ), this, SLOT( deletechannel() ) );
      
      createchannelmsgAct = new QAction( tr( "Create Channel MSG" ), this );
      connect( createchannelmsgAct , SIGNAL( triggered() ), this, SLOT( createchannelmsg() ) );


      contextMnu.clear();
      contextMnu.addAction( deletechannelAct);
      contextMnu.addAction( createchannelmsgAct);
      contextMnu.exec( mevent->globalPos() );
}

void ChanMsgDialog::closeEvent (QCloseEvent * event)
{
    event->accept();
    return;

    /* We can save to Drafts.... but we'll do this later.
     * ... no auto saving for the moment,
     */

#if 0
    if (maybeSave())
    {     
       event->accept();
    }
    else    
    {
		event->ignore();
		hide();
    
		RshareSettings config;
		config.saveWidgetInformation(this);
    }
#endif

}

void ChanMsgDialog::deletechannel()
{


}


void ChanMsgDialog::createchannelmsg()
{


}



void  ChanMsgDialog::insertSendList()
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
		item -> setText(0, QString::fromStdString(detail.name));
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


void  ChanMsgDialog::insertChannelSendList()
{
#if 0
        rsiface->lockData(); /* Lock Interface */

        std::map<RsChanId,ChannelInfo>::const_iterator it;
        const std::map<RsChanId,ChannelInfo> &chans =
                                rsiface->getChannels();

        /* get a link to the table */
        QTreeWidget *sendWidget = ui.msgSendList;

        /* remove old items ??? */
	sendWidget->clear();
	sendWidget->setColumnCount(4);

        QList<QTreeWidgetItem *> items;
	for(it = chans.begin(); it != chans.end(); it++)
	{
		/* make a widget per friend */
           	QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

		/* (0) Title */
		item -> setText(0, QString::fromStdString(it->second.chanName));
		if (it->second.publisher)
		{
			item -> setText(1, "Publisher");
		}
		else
		{
			item -> setText(1, "Listener");
		}

		{
			std::ostringstream out;
			out << it->second.chanId;
			item -> setText(2, QString::fromStdString(out.str()));
		}

		item -> setText(3, "Channel");

		item -> setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		item -> setCheckState(0, Qt::Unchecked);
	
		if (it -> second.inBroadcast)
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

	/* add the items in! */
	sendWidget->insertTopLevelItems(0, items);


	rsiface->unlockData(); /* UnLock Interface */

	sendWidget->update(); /* update display */
#endif
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
void  ChanMsgDialog::insertFileList()
{
	rsiface->lockData();   /* Lock Interface */


        const std::list<FileInfo> &recList = rsiface->getRecommendList();
	std::list<FileInfo>::const_iterator it;

	/* get a link to the table */
	QTreeWidget *tree = ui.msgFileList;

        tree->clear(); 
        tree->setColumnCount(5); 

        QList<QTreeWidgetItem *> items;
	for(it = recList.begin(); it != recList.end(); it++)
	{
		/* make a widget per person */
           	QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);
		/* (0) Filename */
		item -> setText(0, QString::fromStdString(it->fname));
			
		/* (1) Size */
		{
			std::ostringstream out;
			out << it->size;
			item -> setText(1, QString::fromStdString(out.str()));
		}
		/* (2) Rank */
		{
			std::ostringstream out;
			out << it->rank;
			item -> setText(2, QString::fromStdString(out.str()));
		}
			
		item -> setText(3, QString::fromStdString(it->hash));
			
		item -> setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);

		if (it -> inRecommend)
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

	/* add the items in! */
	tree->insertTopLevelItems(0, items);

	rsiface->unlockData(); /* UnLock Interface */

	tree->update(); /* update display */
}


void  ChanMsgDialog::newMsg()
{
	/* clear all */
	ui.titleEdit->setText("No Title");
	ui.msgText->setText("");

        /* worker fns */
	if (mIsMsg)
	{
		insertSendList();
	}
	else
	{
		insertChannelSendList();
	}

	insertFileList();
}

void  ChanMsgDialog::insertTitleText(std::string title)
{
	ui.titleEdit->setText(QString::fromStdString(title));
}


void  ChanMsgDialog::insertMsgText(std::string msg)
{
	ui.msgText->setText(QString::fromStdString(msg));
}


void  ChanMsgDialog::sendMessage()
{


	/* construct a message */
	MessageInfo mi;

	mi.title = ui.titleEdit->text().toStdWString();
	mi.msg =   ui.msgText->toHtml().toStdWString();
	

	rsiface->lockData();   /* Lock Interface */

        const std::list<FileInfo> &recList = rsiface->getRecommendList();
	std::list<FileInfo>::const_iterator it;
	for(it = recList.begin(); it != recList.end(); it++)
	{
		if (it -> inRecommend)
		{
			mi.files.push_back(*it);
		}
	}

	rsiface->unlockData(); /* UnLock Interface */

	/* get the ids from the send list */
	std::list<std::string> peers;
	std::list<std::string> msgto;
	std::list<std::string>::iterator iit;

	rsPeers->getFriendList(peers);
		
	for(iit = peers.begin(); iit != peers.end(); iit++)
	{
		if (rsicontrol->IsInMsg(*iit))
		{
			mi.msgto.push_back(*iit);
		}
	}

	rsMsgs->MessageSend(mi);

	close();
	return;
}


void  ChanMsgDialog::cancelMessage()
{
	close();
	return;
}



/* Toggling .... Check Boxes.....
 * This is dependent on whether we are a 
 *
 * Chan or Msg Dialog.
 */


/* First the Msg (People) ones */
void ChanMsgDialog::togglePersonItem( QTreeWidgetItem *item, int col )
{
        std::cerr << "TogglePersonItem()" << std::endl;

        /* extract id */
        std::string id = (item -> text(1)).toStdString();

        /* get state */
        bool inMsg = (Qt::Checked == item -> checkState(0)); /* alway column 0 */

        /* call control fns */

        rsicontrol -> SetInMsg(id, inMsg);
        return;
}

/* Second  the Channel ones */
void ChanMsgDialog::toggleChannelItem( QTreeWidgetItem *item, int col )
{
        std::cerr << "ToggleChannelItem()" << std::endl;

        /* extract id */
        std::string id = (item -> text(2)).toStdString();

        /* get state */
        bool inBroad = (Qt::Checked == item -> checkState(0)); /* alway column 0 */

        /* call control fns */

        rsicontrol -> SetInBroadcast(id, inBroad);
        return;
}

/* This is actually for both */
void ChanMsgDialog::toggleRecommendItem( QTreeWidgetItem *item, int col )
{
        std::cerr << "ToggleRecommendItem()" << std::endl;

        /* extract name */
        std::string id = (item -> text(0)).toStdString();

        /* get state */
        bool inRecommend = (Qt::Checked == item -> checkState(0)); /* alway column 0 */

        /* call control fns */

        rsicontrol -> SetInRecommend(id, inRecommend);
        return;
}


void ChanMsgDialog::setupFileActions()
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

void ChanMsgDialog::setupEditActions()
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

void ChanMsgDialog::setupViewActions()
{
    QMenu *menu = new QMenu(tr("&View"), this);
    menuBar()->addMenu(menu);

    QAction *a;

    a = new QAction(QIcon(""), tr("&Contacts Sidebar"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleContacts()));
    menu->addAction(a);

}

void ChanMsgDialog::setupInsertActions()
{
    QMenu *menu = new QMenu(tr("&Insert"), this);
    menuBar()->addMenu(menu);

    QAction *a;

    a = new QAction(QIcon(""), tr("&Image"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(addImage()));
    menu->addAction(a);

}

void ChanMsgDialog::textBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(ui.boldbtn->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}

void ChanMsgDialog::textUnderline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(ui.underlinebtn->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void ChanMsgDialog::textItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(ui.italicbtn->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void ChanMsgDialog::textFamily(const QString &f)
{
    QTextCharFormat fmt;
    fmt.setFontFamily(f);
    mergeFormatOnWordOrSelection(fmt);
}

void ChanMsgDialog::textSize(const QString &p)
{
    QTextCharFormat fmt;
    fmt.setFontPointSize(p.toFloat());
    mergeFormatOnWordOrSelection(fmt);
}

void ChanMsgDialog::textStyle(int styleIndex)
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

void ChanMsgDialog::textColor()
{
    QColor col = QColorDialog::getColor(ui.msgText->textColor(), this);
    if (!col.isValid())
        return;
    QTextCharFormat fmt;
    fmt.setForeground(col);
    mergeFormatOnWordOrSelection(fmt);
    colorChanged(col);
}

void ChanMsgDialog::textAlign(QAction *a)
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

void ChanMsgDialog::currentCharFormatChanged(const QTextCharFormat &format)
{
    fontChanged(format.font());
    colorChanged(format.foreground().color());
}

void ChanMsgDialog::cursorPositionChanged()
{
    alignmentChanged(ui.msgText->alignment());
}

void ChanMsgDialog::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = ui.msgText->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);
    ui.msgText->mergeCurrentCharFormat(format);
}

void ChanMsgDialog::fontChanged(const QFont &f)
{
    ui.comboFont->setCurrentIndex(ui.comboFont->findText(QFontInfo(f).family()));
    ui.comboSize->setCurrentIndex(ui.comboSize->findText(QString::number(f.pointSize())));
    ui.boldbtn->setChecked(f.bold());
    ui.italicbtn->setChecked(f.italic());
    ui.underlinebtn->setChecked(f.underline());
}

void ChanMsgDialog::colorChanged(const QColor &c)
{
    QPixmap pix(16, 16);
    pix.fill(c);
    ui.colorbtn->setIcon(pix);
}

void ChanMsgDialog::alignmentChanged(Qt::Alignment a)
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

void ChanMsgDialog::clipboardDataChanged()
{
    actionPaste->setEnabled(!QApplication::clipboard()->text().isEmpty());
}

void ChanMsgDialog::fileNew()
{
    if (maybeSave()) {
        ui.msgText->clear();
        //setCurrentFileName(QString());
    }
}

void ChanMsgDialog::fileOpen()
{
    QString fn = QFileDialog::getOpenFileName(this, tr("Open File..."),
                                              QString(), tr("HTML-Files (*.htm *.html);;All Files (*)"));
    if (!fn.isEmpty())
        load(fn);
}

bool ChanMsgDialog::fileSave()
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

bool ChanMsgDialog::fileSaveAs()
{
    QString fn = QFileDialog::getSaveFileName(this, tr("Save as..."),
                                              QString(), tr("HTML-Files (*.htm *.html);;All Files (*)"));
    if (fn.isEmpty())
        return false;
    setCurrentFileName(fn);
    return fileSave();
}

void ChanMsgDialog::filePrint()
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

/*void TextEdit::filePrintPreview()
{
    PrintPreview *preview = new PrintPreview(textEdit->document(), this);
    preview->setWindowModality(Qt::WindowModal);
    preview->setAttribute(Qt::WA_DeleteOnClose);
    preview->show();
}*/

void ChanMsgDialog::filePrintPdf()
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

void ChanMsgDialog::setCurrentFileName(const QString &fileName)
{
    this->fileName = fileName;
    ui.msgText->document()->setModified(false);

    setWindowModified(false);
}

bool ChanMsgDialog::load(const QString &f)
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


bool ChanMsgDialog::maybeSave()
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


void ChanMsgDialog::toggleContacts()
{
	ui.contactsdockWidget->setVisible(!ui.contactsdockWidget->isVisible());
}

void  ChanMsgDialog::addImage()
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

void  ChanMsgDialog::Create_New_Image_Tag( const QString urlremoteorlocal )
{
   /*if (image_extension(urlremoteorlocal)) {*/
       QString subtext = QString("<p><img src=\"%1\" />").arg(urlremoteorlocal);
               ///////////subtext.append("<br/><br/>Description on image.</p>");
       QTextDocumentFragment fragment = QTextDocumentFragment::fromHtml(subtext);
       ui.msgText->textCursor().insertFragment(fragment);
       //emit statusMessage(QString("Image new :").arg(urlremoteorlocal));
   //}
}



