/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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


#include "MessagesDialog.h"
#include "msgs/MessageComposer.h"
#include "gui/RetroShareLink.h"
#include "util/printpreview.h"
#include "util/misc.h"

#include "rsiface/rsinit.h"
#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsfiles.h"

#include <QtGui>

// Thunder: need a static msgId
//#define STATIC_MSGID

/* Images for context menu icons */
#define IMAGE_MESSAGE		   ":/images/folder-draft.png"
#define IMAGE_MESSAGEREPLY	   ":/images/mail_reply.png"
#define IMAGE_MESSAGEREPLYALL      ":/images/mail_replyall.png"
#define IMAGE_MESSAGEFORWARD	   ":/images/mail_forward.png"
#define IMAGE_MESSAGEREMOVE 	   ":/images/message-mail-imapdelete.png"
#define IMAGE_DOWNLOAD    	   ":/images/start.png"
#define IMAGE_DOWNLOADALL          ":/images/startall.png"

#define COLUMN_COUNT         8
#define COLUMN_ATTACHEMENTS  0
#define COLUMN_SUBJECT       1
#define COLUMN_READ          2
#define COLUMN_FROM          3
#define COLUMN_DATE          4
#define COLUMN_SRCID         5
#define COLUMN_MSGID         6
#define COLUMN_CONTENT       7

#define CONFIG_SECTION_UNREAD   "Unread"

class MyItemDelegate : public QItemDelegate
{
public:
    MyItemDelegate(QObject *parent = 0) : QItemDelegate(parent)
    {
    }

    void paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QStyleOptionViewItem ownOption (option);

        if (index.column() == COLUMN_READ) {
            ownOption.state &= ~QStyle::State_HasFocus; // don't show text and focus rectangle
        }

        QItemDelegate::paint (painter, ownOption, index);
    }

    QSize sizeHint (const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QStyleOptionViewItem ownOption (option);

        if (index.column() == COLUMN_READ) {
            ownOption.state &= ~QStyle::State_HasFocus; // don't show text and focus rectangle
        }

        return QItemDelegate::sizeHint(ownOption, index);
    }
};

static int FilterColumnFromComboBox(int nIndex)
{
    switch (nIndex) {
    case 0:
        return COLUMN_ATTACHEMENTS;
    case 1:
        return COLUMN_SUBJECT;
    case 2:
        return COLUMN_FROM;
    case 3:
        return COLUMN_DATE;
    case 4:
        return COLUMN_CONTENT;
    }

    return COLUMN_SUBJECT;
}

static int FilterColumnToComboBox(int nIndex)
{
    switch (nIndex) {
    case COLUMN_ATTACHEMENTS:
        return 0;
    case COLUMN_SUBJECT:
        return 1;
    case COLUMN_FROM:
        return 2;
    case COLUMN_DATE:
        return 3;
    case COLUMN_CONTENT:
        return 4;
    }

    return FilterColumnToComboBox(COLUMN_SUBJECT);
}

/** Constructor */
MessagesDialog::MessagesDialog(QWidget *parent)
: MainPage(parent)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    m_bProcessSettings = false;
    m_pConfig = new RSettings (RsInit::RsProfileConfigDirectory() + "/msg_locale.cfg");

    connect( ui.messagestreeView, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( messageslistWidgetCostumPopupMenu( QPoint ) ) );
    connect( ui.msgList, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( msgfilelistWidgetCostumPopupMenu( QPoint ) ) );
    connect( ui.messagestreeView, SIGNAL(clicked ( const QModelIndex &) ) , this, SLOT( clicked( const QModelIndex & ) ) );
    connect( ui.messagestreeView, SIGNAL(doubleClicked ( const QModelIndex& ) ) , this, SLOT( doubleClicked( const QModelIndex & ) ) );
    connect( ui.listWidget, SIGNAL( currentRowChanged ( int) ), this, SLOT( changeBox ( int) ) );

    connect(ui.newmessageButton, SIGNAL(clicked()), this, SLOT(newmessage()));
    connect(ui.removemessageButton, SIGNAL(clicked()), this, SLOT(removemessage()));
    connect(ui.replymessageButton, SIGNAL(clicked()), this, SLOT(replytomessage()));
    connect(ui.replyallmessageButton, SIGNAL(clicked()), this, SLOT(replyallmessage()));
    connect(ui.forwardmessageButton, SIGNAL(clicked()), this, SLOT(forwardmessage()));

    connect(ui.actionPrint, SIGNAL(triggered()), this, SLOT(print()));
    ui.actionPrint->setDisabled(true);
    connect(ui.actionPrintPreview, SIGNAL(triggered()), this, SLOT(printpreview()));
    ui.actionPrintPreview->setDisabled(true);
    connect(ui.printbutton, SIGNAL(clicked()), this, SLOT(print()));

    connect(ui.expandFilesButton, SIGNAL(clicked()), this, SLOT(togglefileview()));
    connect(ui.downloadButton, SIGNAL(clicked()), this, SLOT(getcurrentrecommended()));

    connect( ui.msgText, SIGNAL( anchorClicked(const QUrl &)), SLOT(anchorClicked(const QUrl &)));

    connect(ui.actionTextBesideIcon, SIGNAL(triggered()), this, SLOT(buttonstextbesideicon()));
    connect(ui.actionIconOnly, SIGNAL(triggered()), this, SLOT(buttonsicononly()));
    connect(ui.actionTextUnderIcon, SIGNAL(triggered()), this, SLOT(buttonstextundericon()));

    connect(ui.actionSave_as, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
    ui.actionSave_as->setDisabled(true);

    connect( ui.clearButton, SIGNAL(clicked()), this, SLOT(clearFilter()));
    connect( ui.filterPatternLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(filterRegExpChanged()));

    connect(ui.filterColumnComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(filterColumnChanged()));

    mCurrCertId = "";
    mCurrMsgId  = "";

    // Set the QStandardItemModel
    MessagesModel = new QStandardItemModel(0, COLUMN_COUNT);
    MessagesModel->setHeaderData(COLUMN_ATTACHEMENTS,  Qt::Horizontal, QIcon(":/images/attachment.png"), Qt::DecorationRole);
    MessagesModel->setHeaderData(COLUMN_SUBJECT,       Qt::Horizontal, tr("Subject"));
    MessagesModel->setHeaderData(COLUMN_READ,          Qt::Horizontal, QIcon(":/images/message-mail-state-header.png"), Qt::DecorationRole);
    MessagesModel->setHeaderData(COLUMN_FROM,          Qt::Horizontal, tr("From"));
    MessagesModel->setHeaderData(COLUMN_DATE,          Qt::Horizontal, tr("Date"));
    MessagesModel->setHeaderData(COLUMN_SRCID,         Qt::Horizontal, tr("SRCID"));
    MessagesModel->setHeaderData(COLUMN_MSGID,         Qt::Horizontal, tr("MSGID"));
    MessagesModel->setHeaderData(COLUMN_CONTENT,       Qt::Horizontal, tr("Content"));

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSourceModel(MessagesModel);
    proxyModel->setSortRole(Qt::UserRole);
    proxyModel->sort (COLUMN_DATE, Qt::DescendingOrder);
    ui.messagestreeView->setModel(proxyModel);
    ui.messagestreeView->setSelectionBehavior(QTreeView::SelectRows);

    QItemDelegate *pDelegate = new MyItemDelegate(this);
    ui.messagestreeView->setItemDelegate(pDelegate);

    ui.messagestreeView->setRootIsDecorated(false);
    ui.messagestreeView->setSortingEnabled(true);
    ui.messagestreeView->sortByColumn(COLUMN_DATE, Qt::DescendingOrder);

    // connect after setting model
    connect( ui.messagestreeView->selectionModel(), SIGNAL(currentChanged ( QModelIndex, QModelIndex ) ) , this, SLOT( currentChanged( const QModelIndex & ) ) );

    // workaround for Qt bug, should be solved in next Qt release 4.7.0
    // http://bugreports.qt.nokia.com/browse/QTBUG-8270
    QShortcut *Shortcut = new QShortcut(QKeySequence (Qt::Key_Delete), ui.messagestreeView, 0, 0, Qt::WidgetShortcut);
    connect(Shortcut, SIGNAL(activated()), this, SLOT( removemessage ()));

    /* hide the Tree +/- */
    ui.msgList->setRootIsDecorated( false );
    ui.msgList->setSelectionMode( QAbstractItemView::ExtendedSelection );

    /* Set header initial section sizes */
    QHeaderView * msgwheader = ui.messagestreeView->header () ;
    msgwheader->resizeSection (COLUMN_ATTACHEMENTS, 24);
    msgwheader->resizeSection (COLUMN_SUBJECT,      250);
    msgwheader->resizeSection (COLUMN_READ,         16);
    msgwheader->resizeSection (COLUMN_FROM,         140);
    msgwheader->resizeSection (COLUMN_DATE,         140);

    /* Set header resize modes and initial section sizes */
    QHeaderView * msglheader = ui.msgList->header () ;
    msglheader->setResizeMode (0, QHeaderView::Interactive);
    msglheader->setResizeMode (1, QHeaderView::Interactive);
    msglheader->setResizeMode (2, QHeaderView::Interactive);
    msglheader->setResizeMode (3, QHeaderView::Interactive);

    msglheader->resizeSection (0, 200);
    msglheader->resizeSection (1, 100);
    msglheader->resizeSection (2, 100);
    msglheader->resizeSection (3, 200);

    ui.forwardmessageButton->setToolTip(tr("Forward selected Message"));
    ui.replyallmessageButton->setToolTip(tr("Reply to All"));

    QMenu * printmenu = new QMenu();
    printmenu->addAction(ui.actionPrint);
    printmenu->addAction(ui.actionPrintPreview);
    ui.printbutton->setMenu(printmenu);

    QMenu * viewmenu = new QMenu();
    viewmenu->addAction(ui.actionTextBesideIcon);
    viewmenu->addAction(ui.actionIconOnly);
    //viewmenu->addAction(ui.actionTextUnderIcon);
    ui.viewtoolButton->setMenu(viewmenu);

    loadToolButtonsettings();

    mFont = QFont("Arial", 10, QFont::Bold);
    ui.subjectText->setFont(mFont);

    //setting default filter by column as subject
    proxyModel->setFilterKeyColumn(FilterColumnFromComboBox(ui.filterColumnComboBox->currentIndex()));

    ui.clearButton->hide();

    // load settings
    processSettings(true);

    /* Set header sizes for the fixed columns and resize modes, must be set after processSettings */
    msgwheader->setResizeMode (COLUMN_ATTACHEMENTS, QHeaderView::Fixed);
    msgwheader->setResizeMode (COLUMN_DATE, QHeaderView::Interactive);
    msgwheader->setResizeMode (COLUMN_READ, QHeaderView::Fixed);
    msgwheader->resizeSection (COLUMN_READ, 24);

    // fill folder list
    updateMessageSummaryList();
    ui.listWidget->setCurrentRow(0);

    // create timer for navigation
    timer = new QTimer(this);
    timer->setInterval(300);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateCurrentMessage()));

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

MessagesDialog::~MessagesDialog()
{
    // stop and delete timer
    timer->stop();
    delete(timer);

    // save settings
    processSettings(false);

    delete (m_pConfig);
}

void MessagesDialog::processSettings(bool bLoad)
{
    m_bProcessSettings = true;

    QHeaderView *msgwheader = ui.messagestreeView->header () ;

    Settings->beginGroup(QString("MessageDialog"));

    if (bLoad) {
        // load settings

        // expandFiles
        bool bValue = Settings->value("expandFiles", true).toBool();
        ui.expandFilesButton->setChecked(bValue);
        ui.msgList->setVisible(bValue);
        togglefileview_internal();

        // filterColumn
        int nValue = FilterColumnToComboBox(Settings->value("filterColumn", true).toInt());
        ui.filterColumnComboBox->setCurrentIndex(nValue);

        // state of message tree
        msgwheader->restoreState(Settings->value("MessageTree").toByteArray());

        // state of splitter
        ui.msgSplitter->restoreState(Settings->value("Splitter").toByteArray());
        ui.msgSplitter_2->restoreState(Settings->value("Splitter2").toByteArray());
    } else {
        // save settings

        // state of message tree
        Settings->setValue("MessageTree", msgwheader->saveState());

        // state of splitter
        Settings->setValue("Splitter", ui.msgSplitter->saveState());
        Settings->setValue("Splitter2", ui.msgSplitter_2->saveState());
    }

    Settings->endGroup();

    m_bProcessSettings = false;
}

// replaced by shortcut
//void MessagesDialog::keyPressEvent(QKeyEvent *e)
//{
//	if(e->key() == Qt::Key_Delete)
//	{
//		removemessage() ;
//		e->accept() ;
//	}
//	else
//		MainPage::keyPressEvent(e) ;
//}

int MessagesDialog::getSelectedMsgCount (QList<int> *pRowsRead, QList<int> *pRowsUnread)
{
    if (pRowsRead) pRowsRead->clear();
    if (pRowsUnread) pRowsUnread->clear();

    //To check if the selection has more than one row.
    QList<QModelIndex> selectedIndexList = ui.messagestreeView->selectionModel() -> selectedIndexes ();
    QList<int> rowList;
    for(QList<QModelIndex>::iterator it = selectedIndexList.begin(); it != selectedIndexList.end(); it++)
    {
        int row = it->row();
        if (rowList.contains(row) == false)
        {
            rowList.append(row);

            if (pRowsRead || pRowsUnread) {
                int mappedRow = proxyModel->mapToSource(*it).row();
                if (MessagesModel->item(mappedRow, COLUMN_SUBJECT)->font().bold()) {
                    if (pRowsUnread) pRowsUnread->append(mappedRow);
                } else {
                    if (pRowsRead) pRowsRead->append(mappedRow);
                }
            }
        }
    }

    return rowList.size();
}

bool MessagesDialog::isMessageRead(int nRow)
{
    QStandardItem *item;
    item = MessagesModel->item(nRow,COLUMN_SUBJECT);
    return !item->font().bold();
}

void MessagesDialog::messageslistWidgetCostumPopupMenu( QPoint point )
{
    QMenu contextMnu( this );

    /** Defines the actions for the context menu */
    QAction* newmsgAct = NULL;
    QAction* replytomsgAct = NULL;
    QAction* replyallmsgAct = NULL;
    QAction* forwardmsgAct = NULL;
    QAction* removemsgAct = NULL;
    QAction* markAsRead = NULL;
    QAction* markAsUnread = NULL;

    replytomsgAct = new QAction(QIcon(IMAGE_MESSAGEREPLY), tr( "Reply to Message" ), this );
    connect( replytomsgAct , SIGNAL( triggered() ), this, SLOT( replytomessage() ) );
    contextMnu.addAction( replytomsgAct);

    replyallmsgAct = new QAction(QIcon(IMAGE_MESSAGEREPLYALL), tr( "Reply to All" ), this );
    connect( replyallmsgAct , SIGNAL( triggered() ), this, SLOT( replyallmessage() ) );
    contextMnu.addAction( replyallmsgAct);

    forwardmsgAct = new QAction(QIcon(IMAGE_MESSAGEFORWARD), tr( "Forward Message" ), this );
    connect( forwardmsgAct , SIGNAL( triggered() ), this, SLOT( forwardmessage() ) );
    contextMnu.addAction( forwardmsgAct);

    contextMnu.addSeparator();

    QList<int> RowsRead;
    QList<int> RowsUnread;
    int nCount = getSelectedMsgCount (&RowsRead, &RowsUnread);
    
#ifdef STATIC_MSGID
    markAsRead = new QAction(QIcon(":/images/message-mail-read.png"), tr( "Mark as read" ), this);
    connect(markAsRead , SIGNAL(triggered()), this, SLOT(markAsRead()));
    contextMnu.addAction(markAsRead);
    if (RowsUnread.size() == 0) {
        markAsRead->setDisabled(true);
    }

    markAsUnread = new QAction(QIcon(":/images/message-mail.png"), tr( "Mark as unread" ), this);
    connect(markAsUnread , SIGNAL(triggered()), this, SLOT(markAsUnread()));
    contextMnu.addAction(markAsUnread);
    if (RowsRead.size() == 0) {
        markAsUnread->setDisabled(true);
    }

    contextMnu.addSeparator();
#endif

    if (nCount > 1) {
        removemsgAct = new QAction(QIcon(IMAGE_MESSAGEREMOVE), tr( "Remove Messages" ), this );
    } else {
        removemsgAct = new QAction(QIcon(IMAGE_MESSAGEREMOVE), tr( "Remove Message" ), this );
    }

    connect( removemsgAct , SIGNAL( triggered() ), this, SLOT( removemessage() ) );
    contextMnu.addAction( removemsgAct);

    contextMnu.addAction( ui.actionSave_as);
    contextMnu.addAction( ui.actionPrintPreview);
    contextMnu.addAction( ui.actionPrint);
    contextMnu.addSeparator();

    newmsgAct = new QAction(QIcon(IMAGE_MESSAGE), tr( "New Message" ), this );
    connect( newmsgAct , SIGNAL( triggered() ), this, SLOT( newmessage() ) );
    contextMnu.addAction( newmsgAct);

    if (nCount != 1) {
        replytomsgAct->setDisabled(true);
        replyallmsgAct->setDisabled(true);
        forwardmsgAct->setDisabled(true);
    }
    if (nCount == 0) {
        removemsgAct->setDisabled(true);
    }

    contextMnu.exec(QCursor::pos());
}


void MessagesDialog::msgfilelistWidgetCostumPopupMenu( QPoint point )
{
    QAction* getRecAct = NULL;
//    QAction* getAllRecAct = NULL;

    QMenu contextMnu( this );

    getRecAct = new QAction(QIcon(IMAGE_DOWNLOAD), tr( "Download" ), this );
    connect( getRecAct , SIGNAL( triggered() ), this, SLOT( getcurrentrecommended() ) );

//   getAllRecAct = new QAction(QIcon(IMAGE_DOWNLOADALL), tr( "Download" ), this );
//   connect( getAllRecAct , SIGNAL( triggered() ), this, SLOT( getallrecommended() ) );

    contextMnu.addAction( getRecAct);
//    contextMnu.addAction( getAllRecAct);
    contextMnu.exec(QCursor::pos());
}

void MessagesDialog::newmessage()
{
    MessageComposer *nMsgDialog = new MessageComposer();

    /* fill it in */
    //std::cerr << "MessagesDialog::newmessage()" << std::endl;
    nMsgDialog->newMsg();
    nMsgDialog->show();
    nMsgDialog->activateWindow();


    /* window will destroy itself! */
}

void MessagesDialog::replytomessage()
{
    /* put msg on msgBoard, and switch to it. */

    std::string cid;
    std::string mid;

    if(!getCurrentMsg(cid, mid))
        return ;

    mCurrCertId = cid;
    mCurrMsgId  = mid;

    MessageInfo msgInfo;
    if (!rsMsgs -> getMessage(mid, msgInfo))
        return ;

    MessageComposer *nMsgDialog = new MessageComposer();
    /* fill it in */
    //std::cerr << "MessagesDialog::newmessage()" << std::endl;
    nMsgDialog->newMsg();

    QString text = QString::fromStdWString(msgInfo.title);

    if (text.startsWith("Re:", Qt::CaseInsensitive))
    {
	nMsgDialog->insertTitleText( QString::fromStdWString(msgInfo.title).toStdString()) ;
    }
    else
    {
	nMsgDialog->insertTitleText( (QString("Re: ") + QString::fromStdWString(msgInfo.title)).toStdString()) ;
    }

    nMsgDialog->setWindowTitle( tr ("Compose: ") + tr("Re: ") + QString::fromStdWString(msgInfo.title) ) ;


    QTextDocument doc ;
    doc.setHtml(QString::fromStdWString(msgInfo.msg)) ;
    std::string cited_text(doc.toPlainText().toStdString()) ;

    nMsgDialog->insertPastedText(cited_text) ;
    nMsgDialog->addRecipient( msgInfo.srcId ) ;
    nMsgDialog->show();
    nMsgDialog->activateWindow();

    /* window will destroy itself! */
}

void MessagesDialog::replyallmessage()
{
    /* put msg on msgBoard, and switch to it. */

    std::string cid;
    std::string mid;

    if(!getCurrentMsg(cid, mid))
        return ;

    mCurrCertId = cid;
    mCurrMsgId  = mid;

    MessageInfo msgInfo;
    if (!rsMsgs -> getMessage(mid, msgInfo))
        return ;

    MessageComposer *nMsgDialog = new MessageComposer();
    /* fill it in */
    //std::cerr << "MessagesDialog::newmessage()" << std::endl;
    nMsgDialog->newMsg();

    QString text = QString::fromStdWString(msgInfo.title);

    if (text.startsWith("Re:", Qt::CaseInsensitive))
    {
	nMsgDialog->insertTitleText( QString::fromStdWString(msgInfo.title).toStdString()) ;
    }
    else
    {
	nMsgDialog->insertTitleText( (QString("Re: ") + QString::fromStdWString(msgInfo.title)).toStdString()) ;
    }
    nMsgDialog->setWindowTitle( tr ("Compose: ") + tr("Re: ") + QString::fromStdWString(msgInfo.title) ) ;


    QTextDocument doc ;
    doc.setHtml(QString::fromStdWString(msgInfo.msg)) ;
    std::string cited_text(doc.toPlainText().toStdString()) ;

    nMsgDialog->insertPastedText(cited_text) ;
    nMsgDialog->addRecipient( msgInfo.srcId ) ;

    std::list<std::string> tl ( msgInfo.msgto );

    for ( std::list<std::string>::iterator tli = tl.begin(); tli!= tl.end(); tli++ )
    {
        nMsgDialog->addRecipient( *tli ) ;
    }

    nMsgDialog->show();
    nMsgDialog->activateWindow();

    /* window will destroy itself! */
}

void MessagesDialog::forwardmessage()
{
    /* put msg on msgBoard, and switch to it. */

    std::string cid;
    std::string mid;

    if(!getCurrentMsg(cid, mid))
        return ;

    mCurrCertId = cid;
    mCurrMsgId  = mid;

    MessageInfo msgInfo;
    if (!rsMsgs -> getMessage(mid, msgInfo))
        return ;

    MessageComposer *nMsgDialog = new MessageComposer();
    /* fill it in */
    //std::cerr << "MessagesDialog::newmessage()" << std::endl;
    nMsgDialog->newMsg();

    QString text = QString::fromStdWString(msgInfo.title);

    if (text.startsWith("Fwd:", Qt::CaseInsensitive))
    {
	nMsgDialog->insertTitleText( QString::fromStdWString(msgInfo.title).toStdString()) ;
    }
    else
    {
	nMsgDialog->insertTitleText( (QString("Fwd: ") + QString::fromStdWString(msgInfo.title)).toStdString()) ;
    }

    nMsgDialog->setWindowTitle( tr ("Compose: ") + tr("Fwd: ") + QString::fromStdWString(msgInfo.title) ) ;


    QTextDocument doc ;
    doc.setHtml(QString::fromStdWString(msgInfo.msg)) ;
    std::string cited_text(doc.toPlainText().toStdString()) ;

    nMsgDialog->insertForwardPastedText(cited_text) ;

    std::list<FileInfo>& files_info = msgInfo.files;

    /* enable all files for sending */
    std::list<FileInfo>::iterator it;
    for(it = files_info.begin(); it != files_info.end(); it++)
    {
        it->inRecommend = true;
    }

    nMsgDialog->insertFileList(files_info);
    //nMsgDialog->addRecipient( msgInfo.srcId ) ;
    nMsgDialog->show();
    nMsgDialog->activateWindow();

    /* window will destroy itself! */
}

void MessagesDialog::togglefileview_internal()
{
    /* if msg header visible -> change icon and tooltip
    * three widgets...
    */

    if (ui.expandFilesButton->isChecked()) {
        ui.expandFilesButton->setIcon(QIcon(QString(":/images/edit_remove24.png")));
        ui.expandFilesButton->setToolTip(tr("Hide"));
    } else {
        ui.expandFilesButton->setIcon(QIcon(QString(":/images/edit_add24.png")));
        ui.expandFilesButton->setToolTip(tr("Expand"));
    }
}

void MessagesDialog::togglefileview()
{
    // save state of files view
    Settings->setValueToGroup("MessageDialog", "expandFiles", ui.expandFilesButton->isChecked());

    togglefileview_internal();
}


/* download the recommendations... */
void MessagesDialog::getcurrentrecommended()
{
    MessageInfo msgInfo;
    if (!rsMsgs -> getMessage(mCurrMsgId, msgInfo))
        return;

    std::list<std::string> srcIds;
    srcIds.push_back(msgInfo.srcId);

    QModelIndexList list = ui.msgList->selectionModel()->selectedIndexes();

    std::map<int,FileInfo> files ;

    for(QModelIndexList::const_iterator it(list.begin());it!=list.end();++it)
    {
        FileInfo& f(files[it->row()]) ;

        switch(it->column())
        {
        case 0: f.fname = it->data().toString().toStdString() ;
            break ;
        case 1: f.size = it->data().toULongLong() ;
            break ;
        case 3: f.hash = it->data().toString().toStdString() ;
            break ;
        default: ;
        }
    }

    for(std::map<int,FileInfo>::const_iterator it(files.begin());it!=files.end();++it)
    {
        const FileInfo& f(it->second) ;
        std::cout << "Requesting file " << f.fname << ", size=" << f.size << ", hash=" << f.hash << std::endl ;
        rsFiles -> FileRequest(it->second.fname,it->second.hash,it->second.size, "", RS_FILE_HINTS_NETWORK_WIDE, srcIds);
    }
}

#if 0
void MessagesDialog::getallrecommended()
{
    /* get Message */
    MessageInfo msgInfo;
    if (!rsMsgs -> getMessage(mCurrMsgId, msgInfo))
    {
        return;
    }

    const std::list<FileInfo> &recList = msgInfo.files;
    std::list<FileInfo>::const_iterator it;

    std::list<std::string> fnames;
    std::list<std::string> hashes;
    std::list<int>         sizes;

    for(it = recList.begin(); it != recList.end(); it++)
    {
        fnames.push_back(it->fname);
        hashes.push_back(it->hash);
        sizes.push_back(it->size);
    }

    /* now do requests */
    std::list<std::string>::const_iterator fit;
    std::list<std::string>::const_iterator hit;
    std::list<int>::const_iterator sit;

    for(fit = fnames.begin(), hit = hashes.begin(), sit = sizes.begin(); fit != fnames.end(); fit++, hit++, sit++)
    {
        std::cerr << "MessagesDialog::getallrecommended() Calling File Request";
        std::cerr << std::endl;
        std::list<std::string> srcIds;
        srcIds.push_back(msgInfo.srcId);
        rsFiles -> FileRequest(*fit, *hit, *sit, "", 0, srcIds);
    }
}
#endif

void MessagesDialog::changeBox(int)
{
    MessagesModel->removeRows (0, MessagesModel->rowCount());

    insertMessages();
    insertMsgTxtAndFiles();
}

static void InitIconAndFont(RSettings *pConfig, QStandardItem *pItem [COLUMN_COUNT], int nFlag)
{
    QString sText = pItem [COLUMN_SUBJECT]->text();
    QString mid = pItem [COLUMN_MSGID]->text();

    bool bNew = (nFlag & RS_MSG_NEW);

    // show the real "New" state
    if (bNew) {
        if (sText.startsWith("Re:", Qt::CaseInsensitive)) {
            pItem[COLUMN_SUBJECT]->setIcon(QIcon(":/images/message-mail-replied.png"));
        } else if (sText.startsWith("Fwd:", Qt::CaseInsensitive)) {
            pItem[COLUMN_SUBJECT]->setIcon(QIcon(":/images/message-mail-forwarded.png"));
        } else {
            pItem[COLUMN_SUBJECT]->setIcon(QIcon(":/images/message-mail.png"));
        }
    } else {
        // Change Message icon when Subject is Re: or Fwd:
        if (sText.startsWith("Re:", Qt::CaseInsensitive)) {
            pItem[COLUMN_SUBJECT]->setIcon(QIcon(":/images/message-mail-replied-read.png"));
        } else if (sText.startsWith("Fwd:", Qt::CaseInsensitive)) {
            pItem[COLUMN_SUBJECT]->setIcon(QIcon(":/images/message-mail-forwarded-read.png"));
        } else {
            pItem[COLUMN_SUBJECT]->setIcon(QIcon(":/images/message-mail-read.png"));
        }
    }

#ifdef STATIC_MSGID
    // show the locale "New" state
    if (bNew == false) {
        // check locale config
        pConfig->beginGroup(CONFIG_SECTION_UNREAD);
        bNew = pConfig->value(mid, false).toBool();
        pConfig->endGroup();
    }

    if (bNew) {
        pItem[COLUMN_READ]->setIcon(QIcon(":/images/message-mail-state-unread.png"));
    } else {
        pItem[COLUMN_READ]->setIcon(QIcon(":/images/message-mail-state-read.png"));
    }
#endif

    // set font
    for (int i = 0; i < COLUMN_COUNT; i++) {
        QFont qf = pItem[i]->font();
        qf.setBold(bNew);
        pItem[i]->setFont(qf);
    }
}

void MessagesDialog::insertMessages()
{
    std::cerr <<"MessagesDialog::insertMessages called";
    fflush(0);

    std::list<MsgInfoSummary> msgList;
    std::list<MsgInfoSummary>::const_iterator it;
    MessageInfo msgInfo;
    bool bGotInfo;
    QString text;

    rsMsgs -> getMessageSummaries(msgList);

    int listrow = ui.listWidget -> currentRow();

    std::cerr << "MessagesDialog::insertMessages()" << std::endl;
    std::cerr << "Current Row: " << listrow << std::endl;
    fflush(0);

    int i;
    int nFilterColumn = FilterColumnFromComboBox(ui.filterColumnComboBox->currentIndex());

    /* check the mode we are in */
    unsigned int msgbox = 0;
    bool bFill = true;
    switch(listrow)
    {
    case 3:
        msgbox = RS_MSG_SENTBOX;
        break;
    case 2:
        msgbox = RS_MSG_DRAFTBOX;
        break;
    case 1:
        msgbox = RS_MSG_OUTBOX;
        break;
    case 0:
        msgbox = RS_MSG_INBOX;
        break;
    default:
        bFill = false;
    }

    if (msgbox == RS_MSG_INBOX) {
        MessagesModel->setHeaderData(COLUMN_FROM, Qt::Horizontal, tr("From"));
    } else {
        MessagesModel->setHeaderData(COLUMN_FROM, Qt::Horizontal, tr("To"));
    }

    if (bFill) {
        /* remove old items */
        int nRowCount = MessagesModel->rowCount();
        int nRow = 0;
        for (nRow = 0; nRow < nRowCount; ) {
            for(it = msgList.begin(); it != msgList.end(); it++) {
                if ((it->msgflags & RS_MSG_BOXMASK) != msgbox) {
                    continue;
                }

                if (it->msgId == MessagesModel->item(nRow, COLUMN_MSGID)->text().toStdString()) {
                    break;
                }
            }

            if (it == msgList.end ()) {
                MessagesModel->removeRow (nRow);
                nRowCount = MessagesModel->rowCount();
            } else {
                nRow++;
            }
        }

        for(it = msgList.begin(); it != msgList.end(); it++)
        {
            /* check the message flags, to decide which
             * group it should go in...
             *
             * InBox
             * OutBox
             * Drafts
             * Sent
             *
             * FLAGS = OUTGOING.
             * 	-> Outbox/Drafts/Sent
             * 	  + SENT -> Sent
             *	  + IN_PROGRESS -> Draft.
             *	  + nuffing -> Outbox.
             * FLAGS = INCOMING = (!OUTGOING)
             * 	-> + NEW -> Bold.
             *
             */

            if ((it -> msgflags & RS_MSG_BOXMASK) != msgbox)
            {
                //std::cerr << "Msg from other box: " << it->msgflags;
                //std::cerr << std::endl;
                continue;
            }

            bGotInfo = false;
            msgInfo = MessageInfo(); // clear

            // search exisisting items
            nRowCount = MessagesModel->rowCount();
            for (nRow = 0; nRow < nRowCount; nRow++) {
                if (it->msgId == MessagesModel->item(nRow, COLUMN_MSGID)->text().toStdString()) {
                    break;
                }
            }

            /* make a widget per friend */

            QStandardItem *item [COLUMN_COUNT];

            bool bInsert = false;

            if (nRow < nRowCount) {
                for (i = 0; i < COLUMN_COUNT; i++) {
                    item[i] = MessagesModel->item(nRow, i);
                }
            } else {
                for (i = 0; i < COLUMN_COUNT; i++) {
                    item[i] = new QStandardItem();
                }
                bInsert = true;
            }

            //set this false if you want to expand on double click
            for (i = 0; i < COLUMN_COUNT; i++) {
                item[i]->setEditable(false);
            }

            /* So Text should be:
             * (1) Msg / Broadcast
             * (1b) Person / Channel Name
             * (2) Rank
             * (3) Date
             * (4) Title
             * (5) Msg
             * (6) File Count
             * (7) File Total
             */

            QString dateString;
            // Date First.... (for sorting)
            {
                QDateTime qdatetime;
                qdatetime.setTime_t(it->ts);

                // add string to all data
                dateString = qdatetime.toString("_yyyyMMdd_hhmmss");

                //if the mail is on same date show only time.
                if (qdatetime.daysTo(QDateTime::currentDateTime()) == 0)
                {
                    QTime qtime = qdatetime.time();
                    QVariant varTime(qtime);
                    item[COLUMN_DATE]->setData(varTime, Qt::DisplayRole);
                }
                else
                {
                    QVariant varDateTime(qdatetime);
                    item[COLUMN_DATE]->setData(varDateTime, Qt::DisplayRole);
                }
                // for sorting
                item[COLUMN_DATE]->setData(qdatetime, Qt::UserRole);
            }

            //  From ....
            {
                if (msgbox == RS_MSG_INBOX || msgbox == RS_MSG_OUTBOX) {
                    text = QString::fromStdString(rsPeers->getPeerName(it->srcId));
                } else {
                    if (bGotInfo || rsMsgs->getMessage(it->msgId, msgInfo)) {
                        bGotInfo = true;

                        text.clear();

                        std::list<std::string>::const_iterator pit;
                        for (pit = msgInfo.msgto.begin(); pit != msgInfo.msgto.end(); pit++)
                        {
                            if (text.isEmpty() == false) {
                                text += ", ";
                            }

                            QString sPeer = QString::fromStdString(rsPeers->getPeerName(*pit));
                            if (sPeer.isEmpty()) {
                                text += tr("Anonymous") + "@" + QString::fromStdString(*pit);
                            } else {
                                text += sPeer;
                            }
                        }
                    } else {
                        std::cerr << "MessagesDialog::insertMsgTxtAndFiles() Couldn't find Msg" << std::endl;
                    }
                }
                item[COLUMN_FROM]->setText(text);
                item[COLUMN_FROM]->setData(text + dateString, Qt::UserRole);
            }

            // Subject
            text = QString::fromStdWString(it->title);
            item[COLUMN_SUBJECT]->setText(text);
            item[COLUMN_SUBJECT]->setData(text + dateString, Qt::UserRole);

            // internal data
            item[COLUMN_SRCID]->setText(QString::fromStdString(it->srcId));
            item[COLUMN_MSGID]->setText(QString::fromStdString(it->msgId));

            // Init icon and font
            InitIconAndFont(m_pConfig, item, it->msgflags);

            // No of Files.
            {
                std::ostringstream out;
                out << it -> count;
                item[COLUMN_ATTACHEMENTS] -> setText(QString::fromStdString(out.str()));
                item[COLUMN_ATTACHEMENTS]->setData(item[COLUMN_ATTACHEMENTS]->text() + dateString, Qt::UserRole);
                //item -> setTextAlignment( 0, Qt::AlignCenter );
            }

            if (nFilterColumn == COLUMN_CONTENT) {
                // need content for filter
                if (bGotInfo || rsMsgs->getMessage(it->msgId, msgInfo)) {
                    bGotInfo = true;
                    QTextDocument doc;
                    doc.setHtml(QString::fromStdWString(msgInfo.msg));
                    item[COLUMN_CONTENT]->setText(doc.toPlainText().replace(QString("\n"), QString(" ")));
                } else {
                    std::cerr << "MessagesDialog::insertMsgTxtAndFiles() Couldn't find Msg" << std::endl;
                    item[COLUMN_CONTENT]->setText("");
                }
            }

            if (bInsert) {
                /* add to the list */
                QList<QStandardItem *> itemList;
                for (i = 0; i < COLUMN_COUNT; i++) {
                    itemList.append(item[i]);
                }
                MessagesModel->appendRow(itemList);
            }
        }
    } else {
        MessagesModel->removeRows (0, MessagesModel->rowCount());
    }

    ui.messagestreeView->showColumn(COLUMN_ATTACHEMENTS);
    ui.messagestreeView->showColumn(COLUMN_SUBJECT);
#ifdef STATIC_MSGID
    ui.messagestreeView->showColumn(COLUMN_READ);
#else
    ui.messagestreeView->hideColumn(COLUMN_READ);
#endif
    ui.messagestreeView->showColumn(COLUMN_FROM);
    ui.messagestreeView->showColumn(COLUMN_DATE);
    ui.messagestreeView->hideColumn(COLUMN_SRCID);
    ui.messagestreeView->hideColumn(COLUMN_MSGID);
    ui.messagestreeView->hideColumn(COLUMN_CONTENT);
    updateMessageSummaryList();
}

// current row in messagestreeView has changed
void MessagesDialog::currentChanged(const QModelIndex &index )
{
    timer->stop();
    timerIndex = index;
    timer->start();
}

// click in messagestreeView
void MessagesDialog::clicked(const QModelIndex &index )
{
    if (index.isValid() == false) {
        return;
    }

    if (index.column() == COLUMN_READ) {
        int mappedRow = proxyModel->mapToSource(index).row();

        QList<int> Rows;
        Rows.append(mappedRow);
        setMsgAsReadUnread(Rows, !isMessageRead(mappedRow));
        insertMsgTxtAndFiles(index, false);
        updateMessageSummaryList();
        return;
    }

    timer->stop();
    timerIndex = index;
    // show current message directly
    updateCurrentMessage();
}

// double click in messagestreeView
void MessagesDialog::doubleClicked(const QModelIndex &index)
{
    int mappedRow = proxyModel->mapToSource(index).row();

    QStandardItem *pItem = MessagesModel->item(mappedRow, COLUMN_MSGID);
    if (pItem == NULL) {
        return;
    }

    std::string mid = pItem->text().toStdString();

    MessageInfo msgInfo;
    if (!rsMsgs->getMessage(mid, msgInfo)) {
        std::cerr << "MessagesDialog::doubleClicked() Couldn't find Msg" << std::endl;
        return;
    }

    if ((msgInfo.msgflags & RS_MSG_BOXMASK) != RS_MSG_DRAFTBOX) {
        // only draft box for now
        return;
    }

    MessageComposer *pMsgDialog = new MessageComposer();
    /* fill it in */
    pMsgDialog->newMsg(msgInfo.msgId);

    pMsgDialog->show();
    pMsgDialog->activateWindow();

    /* window will destroy itself! */

}

// show current message directly
void MessagesDialog::updateCurrentMessage()
{
    timer->stop();
    insertMsgTxtAndFiles(timerIndex);
}

void MessagesDialog::setMsgAsReadUnread(const QList<int> &Rows, bool bRead)
{
    for (int nRow = 0; nRow < Rows.size(); nRow++) {
        QStandardItem* item[COLUMN_COUNT];
        for(int nCol = 0; nCol < COLUMN_COUNT; nCol++)
        {
            item[nCol] = MessagesModel->item(Rows [nRow], nCol);
        }

        QString mid = item[COLUMN_MSGID]->text();

        m_pConfig->beginGroup(CONFIG_SECTION_UNREAD);
        if (bRead) {
            // set as read in config
#ifdef STATIC_MSGID
            m_pConfig->setValue(mid, false);
#endif
            // set message to read
            rsMsgs->MessageRead(mid.toStdString());
        } else {
            // set as unread in config
#ifdef STATIC_MSGID
            m_pConfig->setValue(mid, true);
#endif
        }
        m_pConfig->endGroup();

        InitIconAndFont(m_pConfig, item, 0);
    }
}

void MessagesDialog::markAsRead()
{
    QList<int> RowsUnread;
    getSelectedMsgCount (NULL, &RowsUnread);

    setMsgAsReadUnread (RowsUnread, true);
    updateMessageSummaryList();
}

void MessagesDialog::markAsUnread()
{
    QList<int> RowsRead;
    getSelectedMsgCount (&RowsRead, NULL);

    setMsgAsReadUnread (RowsRead, false);
    updateMessageSummaryList();
}

void MessagesDialog::insertMsgTxtAndFiles(QModelIndex Index, bool bSetToRead)
{
    std::cerr << "MessagesDialog::insertMsgTxtAndFiles()" << std::endl;

    /* get its Ids */
    std::string cid;
    std::string mid;

    QModelIndex currentIndex = proxyModel->mapToSource(Index);
    if (currentIndex.isValid() == false)
    {
        /* blank it */
        ui.dateText-> setText("");
        ui.toText->setText("");
        ui.fromText->setText("");
        ui.filesText->setText("");

        ui.subjectText->setText("");
        ui.msgList->clear();
        ui.msgText->clear();

        ui.actionSave_as->setDisabled(true);
        ui.actionPrintPreview->setDisabled(true);
        ui.actionPrint->setDisabled(true);

        return;
    }
    else
    {
        QStandardItem * item;
        item = MessagesModel->item(currentIndex.row(),COLUMN_SRCID);
        cid = item->text().toStdString();
        fflush(0);

        item = MessagesModel->item(currentIndex.row(),COLUMN_MSGID);
        mid = item->text().toStdString();
    }

    int nCount = getSelectedMsgCount (NULL, NULL);
    if (nCount == 1) {
        ui.actionSave_as->setEnabled(true);
        ui.actionPrintPreview->setEnabled(true);
        ui.actionPrint->setEnabled(true);
    } else {
        ui.actionSave_as->setDisabled(true);
        ui.actionPrintPreview->setDisabled(true);
        ui.actionPrint->setDisabled(true);
    }

    if (mCurrMsgId == mid) {
        // message doesn't changed
        return;
    }

    /* Save the Data.... for later */

    mCurrCertId = cid;
    mCurrMsgId  = mid;

    MessageInfo msgInfo;
    if (!rsMsgs -> getMessage(mid, msgInfo))
    {
        std::cerr << "MessagesDialog::insertMsgTxtAndFiles() Couldn't find Msg" << std::endl;
        return;
    }

    QList<int> Rows;
    Rows.append(currentIndex.row());

    bool bSetToReadOnActive = Settings->getMsgSetToReadOnActivate();

    if (msgInfo.msgflags & RS_MSG_NEW) {
        // set to read
        setMsgAsReadUnread(Rows, true);
        if (bSetToReadOnActive == false || bSetToRead == false) {
            // set locally to unread
            setMsgAsReadUnread(Rows, false);
        }
        updateMessageSummaryList();
    } else {
        if (bSetToRead && bSetToReadOnActive) {
            // set to read
            setMsgAsReadUnread(Rows, true);
            updateMessageSummaryList();
        }
    }

    const std::list<FileInfo> &recList = msgInfo.files;
    std::list<FileInfo>::const_iterator it;

    /* get a link to the table */
    QTreeWidget *tree = ui.msgList;

    /* get the MessageInfo */

    tree->clear();
    tree->setColumnCount(5);

    QList<QTreeWidgetItem *> items;
    for(it = recList.begin(); it != recList.end(); it++)
    {
        /* make a widget per person */
        QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);
        /* (0) Filename */
        item -> setText(0, QString::fromStdString(it->fname));
        //std::cerr << "Msg FileItem(" << it->fname.length() << ") :" << it->fname << std::endl;

        item -> setText(1, QString::number(it->size)); /* (1) Size */
        item -> setText(2, QString::number(0)); 			/* (2) Rank */ // what is this ???
        item -> setText(3, QString::fromStdString(it->hash));

        /* add to the list */
        items.append(item);
    }

    /* add the items in! */
    tree->insertTopLevelItems(0, items);

    /* iterate through the sources */
    std::list<std::string>::const_iterator pit;

    QString msgTxt;
    for(pit = msgInfo.msgto.begin(); pit != msgInfo.msgto.end(); pit++)
    {
        QString sPeer = QString::fromStdString(rsPeers->getPeerName(*pit));
        if (sPeer.isEmpty())
        {
            msgTxt += "<a style='color: black;'href='" + tr("Anonymous") + "@" + QString::fromStdString(*pit) + "'> " +  tr("Anonymous")  + "</a>" + "    ";
        }
        else
            msgTxt += "<a style='color: black;'href='" + sPeer + "@" + QString::fromStdString(*pit) + "'> " + sPeer  + "</a>" + "   ";
    }

    if (msgInfo.msgcc.size() > 0)
        msgTxt += "\nCc: ";
    for(pit = msgInfo.msgcc.begin(); pit != msgInfo.msgcc.end(); pit++)
    {
        QString sPeer = QString::fromStdString(rsPeers->getPeerName(*pit));
        if (sPeer.isEmpty())
        {
            msgTxt += "<a style='color: black;'href='" + tr("Anonymous") + "@" + QString::fromStdString(*pit) + "'> " +  tr("Anonymous")  + "</a>" + "    ";
        }
        else
            msgTxt += "<a style='color: black;'href='" + sPeer + "@" + QString::fromStdString(*pit) + "'> " + sPeer  + "</a>" + "   ";
    }

    if (msgInfo.msgbcc.size() > 0)
        msgTxt += "\nBcc: ";
    for(pit = msgInfo.msgbcc.begin(); pit != msgInfo.msgbcc.end(); pit++)
    {
        QString sPeer = QString::fromStdString(rsPeers->getPeerName(*pit));
        if (sPeer.isEmpty())
        {
            msgTxt += "<a style='color: black;'href='" + tr("Anonymous") + "@" + QString::fromStdString(*pit) + "'> " +  tr("Anonymous")  + "</a>" + "    ";
        }
        else
            msgTxt += "<a style='color: black;'href='" + sPeer + "@" + QString::fromStdString(*pit) + "'> " + sPeer  + "</a>" + "   ";
    }

    {
        QDateTime qtime;
        qtime.setTime_t(msgInfo.ts);
        QString timestamp = qtime.toString("dd.MM.yyyy hh:mm:ss");
        ui.dateText-> setText(timestamp);
    }
    ui.toText->setText(msgTxt);

    std::string sSrcId;
    if ((msgInfo.msgflags & RS_MSG_BOXMASK) == RS_MSG_OUTBOX) {
        // outgoing message are from me
        sSrcId = rsPeers->getOwnId();
    } else {
        sSrcId = msgInfo.srcId;
    }
    ui.fromText->setText("<a style='color: blue;' href='" + QString::fromStdString(rsPeers->getPeerName(sSrcId)) + "@" + QString::fromStdString(sSrcId) + "'> " + QString::fromStdString(rsPeers->getPeerName(sSrcId)) +"</a>");
    ui.fromText->setToolTip(QString::fromStdString(rsPeers->getPeerName(sSrcId)) + "@" + QString::fromStdString(sSrcId));

    ui.subjectText->setText(QString::fromStdWString(msgInfo.title));
    ui.msgText->setHtml(QString::fromStdWString(msgInfo.msg));

    {
        std::ostringstream out;
        out << "(" << msgInfo.count << " Files)";
        ui.filesText->setText(QString::fromStdString(out.str()));
    }

    std::cerr << "MessagesDialog::insertMsgTxtAndFiles() Msg Displayed OK!" << std::endl;
}

bool MessagesDialog::getCurrentMsg(std::string &cid, std::string &mid)
{
    QModelIndex currentIndex = ui.messagestreeView->currentIndex();
    currentIndex = proxyModel->mapToSource(currentIndex);
    int rowSelected = -1;

    /* get its Ids */
    if (currentIndex.isValid() == false)
    {
        //If no message is selected. assume first message is selected.
        if(MessagesModel->rowCount() == 0)
        {
            return false;
        }
        else
        {
            rowSelected = 0;
        }
    }
    else
    {
        rowSelected = currentIndex.row();
    }

    QStandardItem *item;
    item = MessagesModel->item(rowSelected,COLUMN_SRCID);
    cid = item->text().toStdString();

    item = MessagesModel->item(rowSelected,COLUMN_MSGID);
    mid = item->text().toStdString();
    return true;
}

void MessagesDialog::removemessage()
{
    QList<QModelIndex> selectedIndexList= ui.messagestreeView->selectionModel() -> selectedIndexes ();
    QList<int> rowList;
    QModelIndex selectedIndex;

    for(QList<QModelIndex>::iterator it = selectedIndexList.begin(); it != selectedIndexList.end(); it++) {
        selectedIndex = proxyModel->mapToSource(*it);
        int row = selectedIndex.row();
        if (rowList.contains(row) == false)
        {
            rowList.append(row);
        }
    }

    for(QList<int>::const_iterator it1(rowList.begin());it1!=rowList.end();++it1) {
        QString mid = MessagesModel->item((*it1),COLUMN_MSGID)->text();
        rsMsgs->MessageDelete(mid.toStdString());

        // clean locale config
        m_pConfig->beginGroup(CONFIG_SECTION_UNREAD);
        m_pConfig->remove (mid);
        m_pConfig->endGroup();
    }

    insertMessages();
    return;
}

void MessagesDialog::print()
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

void MessagesDialog::printpreview()
{
    PrintPreview *preview = new PrintPreview(ui.msgText->document(), this);
    preview->setWindowModality(Qt::WindowModal);
    preview->setAttribute(Qt::WA_DeleteOnClose);
    preview->show();
}

void MessagesDialog::anchorClicked (const QUrl& link )
{
#ifdef FORUM_DEBUG
    std::cerr << "MessagesDialog::anchorClicked link.scheme() : " << link.scheme().toStdString() << std::endl;
#endif

    if (link.scheme() == "retroshare")
    {
        RetroShareLink rslnk(link.toString()) ;

#ifdef FORUM_DEBUG
        std::cerr << "MessagesDialog::anchorClicked FileRequest : fileName : " << fileName << ". fileHash : " << fileHash << ". fileSize : " << fileSize << std::endl;
#endif

        if(rslnk.valid())
        {
            std::list<std::string> srcIds;

            if(rsFiles->FileRequest(rslnk.name().toStdString(), rslnk.hash().toStdString(), rslnk.size(), "", RS_FILE_HINTS_NETWORK_WIDE, srcIds))
            {
                QMessageBox mb(tr("File Request Confirmation"), tr("The file has been added to your download list."),QMessageBox::Information,QMessageBox::Ok,0,0);
                mb.setButtonText( QMessageBox::Ok, "OK" );
                mb.exec();
            }
            else
            {
                QMessageBox mb(tr("File Request canceled"), tr("The file has not been added to your download list, because you already have it."),QMessageBox::Information,QMessageBox::Ok,0,0);
                mb.setButtonText( QMessageBox::Ok, "OK" );
                mb.exec();
            }
        }
        else
        {
            QMessageBox mb(tr("File Request Error"), tr("The file link is malformed."),QMessageBox::Information,QMessageBox::Ok,0,0);
            mb.setButtonText( QMessageBox::Ok, "OK" );
            mb.exec();
        }
    }
    else if (link.scheme() == "http")
    {
        QDesktopServices::openUrl(link);
    }
    else if (link.scheme() == "")
    {
        //it's probably a web adress, let's add http:// at the beginning of the link
        QString newAddress = link.toString();
        newAddress.prepend("http://");
        QDesktopServices::openUrl(QUrl(newAddress));
    }
}

bool MessagesDialog::fileSave()
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

bool MessagesDialog::fileSaveAs()
{
    QString fn = QFileDialog::getSaveFileName(this, tr("Save as..."),
                                              QString(), tr("HTML-Files (*.htm *.html);;All Files (*)"));
    if (fn.isEmpty())
        return false;
    setCurrentFileName(fn);
    return fileSave();
}

void MessagesDialog::setCurrentFileName(const QString &fileName)
{
    this->fileName = fileName;
    ui.msgText->document()->setModified(false);

    setWindowModified(false);
}

void MessagesDialog::buttonsicononly()
{
    ui.newmessageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    ui.removemessageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    ui.replymessageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    ui.replyallmessageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    ui.forwardmessageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    ui.printbutton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    ui.viewtoolButton->setToolButtonStyle(Qt::ToolButtonIconOnly);

    Settings->beginGroup(QString("MessageDialog"));

    Settings->setValue("ToolButon_Stlye1",ui.newmessageButton->toolButtonStyle());
    Settings->setValue("ToolButon_Stlye2",ui.removemessageButton->toolButtonStyle());
    Settings->setValue("ToolButon_Stlye3",ui.replymessageButton->toolButtonStyle());
    Settings->setValue("ToolButon_Stlye4",ui.replyallmessageButton->toolButtonStyle());
    Settings->setValue("ToolButon_Stlye5",ui.forwardmessageButton->toolButtonStyle());
    Settings->setValue("ToolButon_Stlye6",ui.printbutton->toolButtonStyle());
    Settings->setValue("ToolButon_Stlye7",ui.viewtoolButton->toolButtonStyle());

    Settings->endGroup();
}

void MessagesDialog::buttonstextbesideicon()
{
    ui.newmessageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui.removemessageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui.replymessageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui.replyallmessageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui.forwardmessageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui.printbutton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    ui.viewtoolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    Settings->beginGroup(QString("MessageDialog"));

    Settings->setValue("ToolButon_Stlye1",ui.newmessageButton->toolButtonStyle());
    Settings->setValue("ToolButon_Stlye2",ui.removemessageButton->toolButtonStyle());
    Settings->setValue("ToolButon_Stlye3",ui.replymessageButton->toolButtonStyle());
    Settings->setValue("ToolButon_Stlye4",ui.replyallmessageButton->toolButtonStyle());
    Settings->setValue("ToolButon_Stlye5",ui.forwardmessageButton->toolButtonStyle());
    Settings->setValue("ToolButon_Stlye6",ui.printbutton->toolButtonStyle());
    Settings->setValue("ToolButon_Stlye7",ui.viewtoolButton->toolButtonStyle());

    Settings->endGroup();
}

void MessagesDialog::buttonstextundericon()
{
    ui.newmessageButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui.removemessageButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui.replymessageButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui.replyallmessageButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui.forwardmessageButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui.printbutton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    ui.viewtoolButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    Settings->beginGroup("MessageDialog");

    Settings->setValue("ToolButon_Stlye1",ui.newmessageButton->toolButtonStyle());
    Settings->setValue("ToolButon_Stlye2",ui.removemessageButton->toolButtonStyle());
    Settings->setValue("ToolButon_Stlye3",ui.replymessageButton->toolButtonStyle());
    Settings->setValue("ToolButon_Stlye4",ui.replyallmessageButton->toolButtonStyle());
    Settings->setValue("ToolButon_Stlye5",ui.forwardmessageButton->toolButtonStyle());
    Settings->setValue("ToolButon_Stlye6",ui.printbutton->toolButtonStyle());
    Settings->setValue("ToolButon_Stlye7",ui.viewtoolButton->toolButtonStyle());

    Settings->endGroup();
}

void MessagesDialog::loadToolButtonsettings()
{
    Settings->beginGroup("MessageDialog");

    if(Settings->value("ToolButon_Stlye1","0").toInt() == 0)
    {
        qDebug() << "ToolButon IconOnly";
        ui.newmessageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
        ui.removemessageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
        ui.replymessageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
        ui.replyallmessageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
        ui.forwardmessageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
        ui.printbutton->setToolButtonStyle(Qt::ToolButtonIconOnly);
        ui.viewtoolButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }

    else if (Settings->value("ToolButon_Stlye1","2").toInt() ==2)
    {
        qDebug() << "ToolButon TextBesideIcon";
        ui.newmessageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        ui.removemessageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        ui.replymessageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        ui.replyallmessageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        ui.forwardmessageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        ui.printbutton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        ui.viewtoolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    }

    else if(Settings->value("ToolButon_Stlye1","3").toInt() ==3)
    {
        qDebug() << "ToolButton TextUnderIcon";
        ui.newmessageButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui.removemessageButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui.replymessageButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui.replyallmessageButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui.forwardmessageButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui.printbutton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui.viewtoolButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    }

    Settings->endGroup();
}

void MessagesDialog::filterRegExpChanged()
{
    QRegExp regExp(ui.filterPatternLineEdit->text(),  Qt::CaseInsensitive , QRegExp::FixedString);
    proxyModel->setFilterRegExp(regExp);

    QString text = ui.filterPatternLineEdit->text();

    if (text.isEmpty())
    {
        ui.clearButton->hide();
    }
    else
    {
        ui.clearButton->show();
    }
}

void MessagesDialog::filterColumnChanged()
{
    if (m_bProcessSettings) {
        return;
    }

    int nFilterColumn = FilterColumnFromComboBox(ui.filterColumnComboBox->currentIndex());
    if (nFilterColumn == COLUMN_CONTENT) {
        // need content ... refill
        insertMessages();
    }
    proxyModel->setFilterKeyColumn(nFilterColumn);

    // save index
    Settings->setValueToGroup("MessageDialog", "filterColumn", nFilterColumn);
}

void MessagesDialog::updateMessageSummaryList()
{
    unsigned int newInboxCount = 0;
    unsigned int newOutboxCount = 0;
    unsigned int newDraftCount = 0;
    unsigned int newSentboxCount = 0;
    unsigned int inboxCount = 0;

    /* calculating the new messages */
//    rsMsgs->getMessageCount (&inboxCount, &newInboxCount, &newOutboxCount, &newDraftCount, &newSentboxCount);

    std::list<MsgInfoSummary> msgList;
    std::list<MsgInfoSummary>::const_iterator it;

    rsMsgs->getMessageSummaries(msgList);

    /*calculating the new messages*/
    for (it = msgList.begin(); it != msgList.end(); it++) {
        switch (it->msgflags & RS_MSG_BOXMASK) {
        case RS_MSG_INBOX:
                inboxCount++;
                if ((it->msgflags & RS_MSG_NEW) == RS_MSG_NEW) {
                    newInboxCount++;
                } else {
                    // check locale config
#ifdef STATIC_MSGID
                    m_pConfig->beginGroup(CONFIG_SECTION_UNREAD);
                    if (m_pConfig->value(QString::fromStdString(it->msgId), false).toBool()) {
                        newInboxCount++;
                    }
                    m_pConfig->endGroup();
#endif
                }
                break;
        case RS_MSG_OUTBOX:
                newOutboxCount++;
                break;
        case RS_MSG_DRAFTBOX:
                newDraftCount++;
                break;
        case RS_MSG_SENTBOX:
                newSentboxCount++;
                break;
        }
    }

    QString textItem;
    /*updating the labels in leftcolumn*/

    //QList<QListWidgetItem *> QListWidget::findItems ( const QString & text, Qt::MatchFlags flags ) const
    QListWidgetItem* item = ui.listWidget->item(0);
    if (newInboxCount != 0)
    {
        textItem = tr("Inbox") + " " + "(" + QString::number(newInboxCount)+")";
        item->setText(textItem);
        QFont qf = item->font();
        qf.setBold(true);
        item->setFont(qf);
        item->setIcon(QIcon(":/images/folder-inbox-new.png"));
        item->setForeground(QBrush(QColor(49, 106, 197)));
    }
    else
    {
        textItem = tr("Inbox");
        item->setText(textItem);
        QFont qf = item->font();
        qf.setBold(false);
        item->setFont(qf);
        item->setIcon(QIcon(":/images/folder-inbox.png"));
        item->setForeground(QBrush(QColor(0, 0, 0)));
    }

    //QList<QListWidgetItem *> QListWidget::findItems ( const QString & text, Qt::MatchFlags flags ) const
    item = ui.listWidget->item(1);
    if (newOutboxCount != 0)
    {
        textItem = tr("Outbox") + " " + "(" + QString::number(newOutboxCount)+")";
        item->setText(textItem);
        QFont qf = item->font();
        qf.setBold(true);
        item->setFont(qf);
    }
    else
    {
        textItem = tr("Outbox");
        item->setText(textItem);
        QFont qf = item->font();
        qf.setBold(false);
        item->setFont(qf);

    }

    //QList<QListWidgetItem *> QListWidget::findItems ( const QString & text, Qt::MatchFlags flags ) const
    item = ui.listWidget->item(2);
    if (newDraftCount != 0)
    {
        textItem = tr("Draft") + "(" + QString::number(newDraftCount)+")";
        item->setText(textItem);
        QFont qf = item->font();
        qf.setBold(true);
        item->setFont(qf);
    }
    else
    {
        textItem = tr("Draft");
        item->setText(textItem);
        QFont qf = item->font();
        qf.setBold(false);
        item->setFont(qf);

    }

    /* Total Inbox */
    item = ui.listWidget->item(5);
    textItem = tr("Total Inbox:") + " "  + QString::number(inboxCount);
    item->setText(textItem);

    /* Total Sent */
    item = ui.listWidget->item(6);
    textItem = tr("Total Sent:") + " "  + QString::number(newSentboxCount);
    item->setText(textItem);
}

/** clear Filter **/
void MessagesDialog::clearFilter()
{
    ui.filterPatternLineEdit->clear();
    ui.filterPatternLineEdit->setFocus();
}
