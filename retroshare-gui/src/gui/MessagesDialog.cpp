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

/* Images for context menu icons */
#define IMAGE_MESSAGE		   ":/images/folder-draft.png"
#define IMAGE_MESSAGEREPLY	   ":/images/mail_reply.png"
#define IMAGE_MESSAGEREPLYALL      ":/images/mail_replyall.png"
#define IMAGE_MESSAGEFORWARD	   ":/images/mail_forward.png"
#define IMAGE_MESSAGEREMOVE 	   ":/images/message-mail-imapdelete.png"
#define IMAGE_DOWNLOAD    	   ":/images/start.png"
#define IMAGE_DOWNLOADALL          ":/images/startall.png"

#define COLUMN_COUNT         7
#define COLUMN_ATTACHEMENTS  0
#define COLUMN_SUBJECT       1
#define COLUMN_READ          2
#define COLUMN_FROM          3
#define COLUMN_DATE          4
#define COLUMN_CONTENT       5
#define COLUMN_TAGS          6

#define COLUMN_DATA          0 // column for storing the userdata like msgid and srcid

#define ROLE_SORT  Qt::UserRole
#define ROLE_MSGID Qt::UserRole + 1
#define ROLE_SRCID Qt::UserRole + 2

#define ROW_INBOX         0
#define ROW_OUTBOX        1
#define ROW_DRAFTBOX      2
#define ROW_SENTBOX       3
#define ROW_TRASHBOX      4

#define ACTION_TAGSINDEX_SIZE  3
#define ACTION_TAGSINDEX_TYPE  "Type"
#define ACTION_TAGSINDEX_ID    "ID"
#define ACTION_TAGSINDEX_COLOR "Color"

#define ACTION_TAGS_REMOVEALL 0
#define ACTION_TAGS_TAG       1
#define ACTION_TAGS_NEWTAG    2

#define CONFIG_FILE (RsInit::RsProfileConfigDirectory() + "/msg_locale.cfg")

#define CONFIG_SECTION_UNREAD   "Unread"

#define CONFIG_SECTION_TAGS     "Tags"
#define CONFIG_KEY_TEXT         "Text"
#define CONFIG_KEY_COLOR        "Color"

#define CONFIG_SECTION_TAG      "Tag"
#define CONFIG_KEY_TAG          "Tag"

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

class MyMenu : public QMenu
{
public:
    MyMenu(const QString &title, QWidget *parent) : QMenu (title, parent)
    {
    }

protected:
    virtual void paintEvent(QPaintEvent *e)
    {
        QMenu::paintEvent(e);

        QPainter p(this);
        QRegion emptyArea = QRegion(rect());

        //draw the items with color
        foreach (QAction *pAction, actions()) {
            QRect adjustedActionRect = actionGeometry(pAction);
            if (!e->rect().intersects(adjustedActionRect))
               continue;

            const QMap<QString, QVariant> &Values = pAction->data().toMap();
            if (Values.size () != ACTION_TAGSINDEX_SIZE) {
                continue;
            }
            if (Values [ACTION_TAGSINDEX_TYPE] != ACTION_TAGS_TAG) {
                continue;
            }

            //set the clip region to be extra safe (and adjust for the scrollers)
            QRegion adjustedActionReg(adjustedActionRect);
            emptyArea -= adjustedActionReg;
            p.setClipRegion(adjustedActionReg);

            QStyleOptionMenuItem opt;
            initStyleOption(&opt, pAction);

            opt.palette.setColor(QPalette::ButtonText, QColor(Values [ACTION_TAGSINDEX_COLOR].toInt()));
            // needed for Cleanlooks
            opt.palette.setColor(QPalette::Text, QColor(Values [ACTION_TAGSINDEX_COLOR].toInt()));

            opt.rect = adjustedActionRect;
            style()->drawControl(QStyle::CE_MenuItem, &opt, &p, this);
        }
    }
};

MessagesDialog::LockUpdate::LockUpdate (MessagesDialog *pDialog, bool bUpdate)
{
    m_pDialog = pDialog;
    m_bUpdate = bUpdate;

    m_pDialog->m_nLockUpdate++;
}

MessagesDialog::LockUpdate::~LockUpdate ()
{
    m_pDialog->m_nLockUpdate = qMax (--m_pDialog->m_nLockUpdate, 0);

    if (m_bUpdate && m_pDialog->m_nLockUpdate == 0) {
        m_pDialog->insertMessages();
    }
}

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
    case 5:
        return COLUMN_TAGS;
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
    case COLUMN_TAGS:
        return 5;
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
    m_bInChange = false;
    m_nLockUpdate = 0;
    m_pConfig = new RSettings (CONFIG_FILE);

    connect( ui.messagestreeView, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( messageslistWidgetCostumPopupMenu( QPoint ) ) );
    connect( ui.msgList, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( msgfilelistWidgetCostumPopupMenu( QPoint ) ) );
    connect( ui.messagestreeView, SIGNAL(clicked ( const QModelIndex &) ) , this, SLOT( clicked( const QModelIndex & ) ) );
    connect( ui.messagestreeView, SIGNAL(doubleClicked ( const QModelIndex& ) ) , this, SLOT( doubleClicked( const QModelIndex & ) ) );
    connect( ui.listWidget, SIGNAL( currentRowChanged ( int) ), this, SLOT( changeBox ( int) ) );
    connect( ui.tagWidget, SIGNAL( currentRowChanged ( int) ), this, SLOT( changeTag ( int) ) );

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

    m_eListMode = LIST_NOTHING;

    mCurrCertId = "";
    mCurrMsgId  = "";

    // Set the QStandardItemModel
    MessagesModel = new QStandardItemModel(0, COLUMN_COUNT);
    MessagesModel->setHeaderData(COLUMN_ATTACHEMENTS,  Qt::Horizontal, QIcon(":/images/attachment.png"), Qt::DecorationRole);
    MessagesModel->setHeaderData(COLUMN_SUBJECT,       Qt::Horizontal, tr("Subject"));
    MessagesModel->setHeaderData(COLUMN_READ,          Qt::Horizontal, QIcon(":/images/message-mail-state-header.png"), Qt::DecorationRole);
    MessagesModel->setHeaderData(COLUMN_FROM,          Qt::Horizontal, tr("From"));
    MessagesModel->setHeaderData(COLUMN_DATE,          Qt::Horizontal, tr("Date"));
    MessagesModel->setHeaderData(COLUMN_TAGS,          Qt::Horizontal, tr("Tags"));
    MessagesModel->setHeaderData(COLUMN_CONTENT,       Qt::Horizontal, tr("Content"));

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSourceModel(MessagesModel);
    proxyModel->setSortRole(ROLE_SORT);
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
    Shortcut = new QShortcut(QKeySequence (Qt::SHIFT | Qt::Key_Delete), ui.messagestreeView, 0, 0, Qt::WidgetShortcut);
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
    ui.listWidget->setCurrentRow(ROW_INBOX);

    // create tag menu
    fillTags();

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

        // state of tag list
        bValue = Settings->value("tagList", true).toBool();
        ui.Tags_Button->setChecked(bValue);

        // state of splitter
        ui.msgSplitter->restoreState(Settings->value("Splitter").toByteArray());
        ui.msgSplitter_2->restoreState(Settings->value("Splitter2").toByteArray());
        ui.listSplitter->restoreState(Settings->value("Splitter3").toByteArray());
    } else {
        // save settings

        // state of message tree
        Settings->setValue("MessageTree", msgwheader->saveState());

        // state of tag list
        Settings->setValue("tagList", ui.Tags_Button->isChecked());

        // state of splitter
        Settings->setValue("Splitter", ui.msgSplitter->saveState());
        Settings->setValue("Splitter2", ui.msgSplitter_2->saveState());
        Settings->setValue("Splitter3", ui.listSplitter->saveState());
    }

    Settings->endGroup();

    m_bProcessSettings = false;
}

static void getMessageTags (RSettings *pConfig, QString msgId, QList<int> &tagIds)
{
    pConfig->beginGroup(CONFIG_SECTION_TAG);

    int nSize = pConfig->beginReadArray(msgId);

    for (int i = 0; i < nSize; i++) {
        pConfig->setArrayIndex(i);
        int nTagId = pConfig->value(CONFIG_KEY_TAG).toInt();
        tagIds.push_back(nTagId);
    }
    pConfig->endArray();

    pConfig->endGroup();
}

static void setMessageTags (RSettings *pConfig, QString &msgId, QList<int> &tagIds)
{
    pConfig->beginGroup(CONFIG_SECTION_TAG);

    pConfig->remove(msgId);

    if (tagIds.size()) {
        pConfig->beginWriteArray(msgId, tagIds.size());

        int i = 0;
        for (QList<int>::iterator tagId = tagIds.begin(); tagId != tagIds.end(); tagId++) {
            pConfig->setArrayIndex(i++);
            pConfig->setValue(CONFIG_KEY_TAG, *tagId);
        }
        pConfig->endArray();
    }

    pConfig->endGroup();
}

/*static*/ void MessagesDialog::initStandardTagItems(std::map<int, TagItem> &Items)
{
    // create standard enties ... id = sort, maybe later own member
    Items [-5].text = tr("Important");
    Items [-5].color = QColor(255, 0, 0).rgb();

    Items [-4].text = tr("Work");
    Items [-4].color = QColor(255, 153, 0).rgb();

    Items [-3].text = tr("Personal");
    Items [-3].color = QColor(0, 153, 0).rgb();

    Items [-2].text = tr("Todo");
    Items [-2].color = QColor(51, 51, 255).rgb();

    Items [-1].text = tr("Later");
    Items [-1].color = QColor(153, 51, 153).rgb();
}

void MessagesDialog::getTagItems(std::map<int, TagItem> &Items)
{
    initStandardTagItems (Items);

    // create standard enties
    initStandardTagItems(Items);

    // load user tags and colors
    m_pConfig->beginGroup(CONFIG_SECTION_TAGS);

    QStringList ids = m_pConfig->childGroups();
    for (QStringList::iterator id = ids.begin(); id != ids.end(); id++) {
        int nId = 0;
        std::istringstream instream((*id).toStdString());
        instream >> nId;

        m_pConfig->beginGroup(*id);

        TagItem Item;
        if (nId < 0) {
            // standard tag
            Item = Items[nId];
        } else {
            Item.text = m_pConfig->value(CONFIG_KEY_TEXT).toString();
        }
        Item.color = m_pConfig->value(CONFIG_KEY_COLOR, Item.color).toUInt();

        m_pConfig->endGroup();

        Items [nId] = Item;
    }

    m_pConfig->endGroup();
}

void MessagesDialog::setTagItems(std::map<int, TagItem> &Items)
{
    // process deleted tags
    QList<int> tagIdsToDelete;
    std::map<int, TagItem>::iterator Item;
    for (Item = Items.begin(); Item != Items.end(); Item++) {
        if (Item->second._delete) {
            tagIdsToDelete.push_back(Item->first);
        }
    }

    if (tagIdsToDelete.size()) {
        // iterate all saved tags on messages and remove the id's

        // get all msgIds with tags
        m_pConfig->beginGroup(CONFIG_SECTION_TAG);
        QStringList msgIds = m_pConfig->childGroups();
        m_pConfig->endGroup();

        for (QStringList::iterator msgId = msgIds.begin(); msgId != msgIds.end(); msgId++) {
            QList<int> tagIds;
            getMessageTags (m_pConfig, *msgId, tagIds);

            bool bSave = false;

            for (QList<int>::iterator tagIdToDelete = tagIdsToDelete.begin(); tagIdToDelete != tagIdsToDelete.end(); tagIdToDelete++) {
                QList<int>::iterator tagId = qFind(tagIds.begin(), tagIds.end(), *tagIdToDelete);
                if (tagId != tagIds.end()) {
                    tagIds.erase(tagId);
                    bSave = true;
                }
            }

            if (bSave) {
                setMessageTags (m_pConfig, *msgId, tagIds);
            }
        }
    }

    // save tags
    m_pConfig->remove(CONFIG_SECTION_TAGS);

    m_pConfig->beginGroup(CONFIG_SECTION_TAGS);

    for (Item = Items.begin(); Item != Items.end(); Item++) {
        if (Item->second._delete) {
            continue;
        }

        QString sId;
        sId.sprintf("%d", Item->first);
        m_pConfig->beginGroup(sId);

        if (Item->first > 0) {
            m_pConfig->setValue(CONFIG_KEY_TEXT, Item->second.text);
        }
        m_pConfig->setValue(CONFIG_KEY_COLOR, Item->second.color);

        m_pConfig->endGroup();
    }

    m_pConfig->endGroup();

    fillTags();
    insertMessages();
}

void MessagesDialog::fillTags()
{
    std::map<int, TagItem> TagItems;
    std::map<int, TagItem>::iterator Item;
    getTagItems(TagItems);

    // create tag menu
    QMenu *pMenu = new MyMenu (tr("Tag"), this);
    connect(pMenu, SIGNAL(triggered (QAction*)), this, SLOT(tagTriggered(QAction*)));
    connect(pMenu, SIGNAL(aboutToShow()), this, SLOT(tagAboutToShow()));

    bool bUser = false;

    QAction *pAction;
    QMap<QString, QVariant> Values;

    if (TagItems.size()) {
        pAction = new QAction(tr("Remove All Tags"), pMenu);
        Values [ACTION_TAGSINDEX_TYPE] = ACTION_TAGS_REMOVEALL;
        Values [ACTION_TAGSINDEX_ID] = 0;
        Values [ACTION_TAGSINDEX_COLOR] = 0;
        pAction->setData (Values);
        pMenu->addAction(pAction);

        pMenu->addSeparator();

        for (Item = TagItems.begin(); Item != TagItems.end(); Item++) {
            pAction = new QAction(Item->second.text, pMenu);
            Values [ACTION_TAGSINDEX_TYPE] = ACTION_TAGS_TAG;
            Values [ACTION_TAGSINDEX_ID] = Item->first;
            Values [ACTION_TAGSINDEX_COLOR] = Item->second.color;
            pAction->setData (Values);
            pAction->setCheckable(true);

            if (Item->first > 0 && bUser == false) {
                bUser = true;
                pMenu->addSeparator();
            }

            pMenu->addAction(pAction);
        }

        pMenu->addSeparator();
    }

    pAction = new QAction(tr("New tag ..."), pMenu);
    Values [ACTION_TAGSINDEX_TYPE] = ACTION_TAGS_NEWTAG;
    Values [ACTION_TAGSINDEX_ID] = 0;
    Values [ACTION_TAGSINDEX_COLOR] = 0;
    pAction->setData (Values);
    pMenu->addAction(pAction);

    ui.tagButton->setMenu(pMenu);

    // fill tags
    m_bInChange = true;

    // save current selection
    QListWidgetItem *pItem = ui.tagWidget->currentItem();
    int nSelectecTagId = 0;
    if (pItem) {
        nSelectecTagId = pItem->data(Qt::UserRole).toInt();
    }

    QListWidgetItem *pItemToSelect = NULL;

    ui.tagWidget->clear();
    for (Item = TagItems.begin(); Item != TagItems.end(); Item++) {
        pItem = new QListWidgetItem (Item->second.text, ui.tagWidget);
        pItem->setForeground(QBrush(QColor(Item->second.color)));
        pItem->setIcon(QIcon(":/images/foldermail.png"));
        pItem->setData(Qt::UserRole, Item->first);
        pItem->setData(Qt::UserRole + 1, Item->second.text); // for updateMessageSummaryList

        if (Item->first == nSelectecTagId) {
            pItemToSelect = pItem;
        }
    }

    if (pItemToSelect) {
        ui.tagWidget->setCurrentItem(pItemToSelect);
    }

    m_bInChange = false;

    updateMessageSummaryList();
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

int MessagesDialog::getSelectedMsgCount (QList<int> *pRows, QList<int> *pRowsRead, QList<int> *pRowsUnread)
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

            if (pRows || pRowsRead || pRowsUnread) {
                int mappedRow = proxyModel->mapToSource(*it).row();

                if (pRows) pRows->append(mappedRow);

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

    QAction *replytomsgAct = new QAction(QIcon(IMAGE_MESSAGEREPLY), tr( "Reply to Message" ), this );
    connect( replytomsgAct , SIGNAL( triggered() ), this, SLOT( replytomessage() ) );
    contextMnu.addAction( replytomsgAct);

    QAction *replyallmsgAct = new QAction(QIcon(IMAGE_MESSAGEREPLYALL), tr( "Reply to All" ), this );
    connect( replyallmsgAct , SIGNAL( triggered() ), this, SLOT( replyallmessage() ) );
    contextMnu.addAction( replyallmsgAct);

    QAction *forwardmsgAct = new QAction(QIcon(IMAGE_MESSAGEFORWARD), tr( "Forward Message" ), this );
    connect( forwardmsgAct , SIGNAL( triggered() ), this, SLOT( forwardmessage() ) );
    contextMnu.addAction( forwardmsgAct);

    QList<int> RowsRead;
    QList<int> RowsUnread;
    int nCount = getSelectedMsgCount (NULL, &RowsRead, &RowsUnread);

    QAction *editAct = new QAction(tr( "Edit..." ), this );
    connect(editAct, SIGNAL(triggered()), this, SLOT(editmessage()));
    contextMnu.addAction(editAct);

    if (nCount == 1) {
        editAct->setEnabled(true);
    } else {
        editAct->setDisabled(true);
    }

    contextMnu.addSeparator();

    QAction *markAsRead = new QAction(QIcon(":/images/message-mail-read.png"), tr( "Mark as read" ), this);
    connect(markAsRead , SIGNAL(triggered()), this, SLOT(markAsRead()));
    contextMnu.addAction(markAsRead);
    if (RowsUnread.size() == 0) {
        markAsRead->setDisabled(true);
    }

    QAction *markAsUnread = new QAction(QIcon(":/images/message-mail.png"), tr( "Mark as unread" ), this);
    connect(markAsUnread , SIGNAL(triggered()), this, SLOT(markAsUnread()));
    contextMnu.addAction(markAsUnread);
    if (RowsRead.size() == 0) {
        markAsUnread->setDisabled(true);
    }

    contextMnu.addSeparator();

    // add tags
    contextMnu.addMenu(ui.tagButton->menu());
    contextMnu.addSeparator();

    QAction *removemsgAct;
    if (nCount > 1) {
        removemsgAct = new QAction(QIcon(IMAGE_MESSAGEREMOVE), tr( "Remove Messages" ), this );
    } else {
        removemsgAct = new QAction(QIcon(IMAGE_MESSAGEREMOVE), tr( "Remove Message" ), this );
    }

    connect( removemsgAct , SIGNAL( triggered() ), this, SLOT( removemessage() ) );
    contextMnu.addAction( removemsgAct);

    int listrow = ui.listWidget -> currentRow();
    if (listrow == ROW_TRASHBOX) {
        QAction *undeleteAct = new QAction(tr( "Undelete" ), this );
        connect(undeleteAct, SIGNAL(triggered()), this, SLOT(undeletemessage()));
        contextMnu.addAction(undeleteAct);

        if (nCount) {
            undeleteAct->setEnabled(true);
        } else {
            undeleteAct->setDisabled(true);
        }
    }

    contextMnu.addAction( ui.actionSave_as);
    contextMnu.addAction( ui.actionPrintPreview);
    contextMnu.addAction( ui.actionPrint);
    contextMnu.addSeparator();

    QAction *newmsgAct = new QAction(QIcon(IMAGE_MESSAGE), tr( "New Message" ), this );
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

void MessagesDialog::editmessage()
{
    std::string cid;
    std::string mid;

    if(!getCurrentMsg(cid, mid))
        return ;

    MessageInfo msgInfo;
    if (!rsMsgs->getMessage(mid, msgInfo)) {
        std::cerr << "MessagesDialog::editmessage() Couldn't find Msg" << std::endl;
        return;
    }

    MessageComposer *pMsgDialog = new MessageComposer();
    /* fill it in */
    pMsgDialog->newMsg(msgInfo.msgId);

    pMsgDialog->show();
    pMsgDialog->activateWindow();

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
    if (m_bInChange) {
        // already in change method
        return;
    }

    m_bInChange = true;

    MessagesModel->removeRows (0, MessagesModel->rowCount());

    ui.tagWidget->setCurrentItem(NULL);
    m_eListMode = LIST_BOX;

    insertMessages();
    insertMsgTxtAndFiles();

    m_bInChange = false;
}

void MessagesDialog::changeTag(int)
{
    if (m_bInChange) {
        // already in change method
        return;
    }

    m_bInChange = true;

    MessagesModel->removeRows (0, MessagesModel->rowCount());

    ui.listWidget->setCurrentItem(NULL);
    m_eListMode = LIST_TAG;

    insertMessages();
    insertMsgTxtAndFiles();

    m_bInChange = false;
}

static void InitIconAndFont(RSettings *pConfig, QStandardItem *pItem [COLUMN_COUNT], int nFlag)
{
    QString sText = pItem [COLUMN_SUBJECT]->text();
    QString mid = pItem [COLUMN_DATA]->data(ROLE_MSGID).toString();

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

    // set font
    for (int i = 0; i < COLUMN_COUNT; i++) {
        QFont qf = pItem[i]->font();
        qf.setBold(bNew);
        pItem[i]->setFont(qf);
    }
}

void MessagesDialog::insertMessages()
{
    if (m_nLockUpdate) {
        return;
    }

    std::cerr <<"MessagesDialog::insertMessages called";
    fflush(0);

    std::list<MsgInfoSummary> msgList;
    std::list<MsgInfoSummary>::const_iterator it;
    MessageInfo msgInfo;
    bool bGotInfo;
    QString text;

    rsMsgs -> getMessageSummaries(msgList);

    std::cerr << "MessagesDialog::insertMessages()" << std::endl;
    fflush(0);

    int nFilterColumn = FilterColumnFromComboBox(ui.filterColumnComboBox->currentIndex());

    /* check the mode we are in */
    unsigned int msgbox = 0;
    bool bTrash = false;
    bool bFill = true;
    int nTagId = 0;

    switch (m_eListMode) {
    case LIST_NOTHING:
        bFill = false;
        break;

    case LIST_BOX:
        {
            int listrow = ui.listWidget->currentRow();

            switch (listrow) {
            case ROW_INBOX:
                msgbox = RS_MSG_INBOX;
                ui.tabWidget->setTabIcon ( 0, QIcon(":/images/folder-inbox.png") );
                ui.tabWidget->setTabText ( 0, tr ("Inbox") );
                break;
            case ROW_OUTBOX:
                msgbox = RS_MSG_OUTBOX;
                ui.tabWidget->setTabIcon ( 0, QIcon(":/images/folder-outbox.png") );
                ui.tabWidget->setTabText ( 0, tr ("Outbox") );
                break;
            case ROW_DRAFTBOX:
                msgbox = RS_MSG_DRAFTBOX;
                ui.tabWidget->setTabIcon ( 0, QIcon(":/images/folder-draft.png") );
                ui.tabWidget->setTabText ( 0, tr ("Drafts") );
                break;
            case ROW_SENTBOX:
                msgbox = RS_MSG_SENTBOX;
                ui.tabWidget->setTabIcon ( 0, QIcon(":/images/folder-sent.png") );
                ui.tabWidget->setTabText ( 0, tr ("Sent") );
                break;
            case ROW_TRASHBOX:
                bTrash = true;
                ui.tabWidget->setTabIcon ( 0, QIcon(":/images/folder-trash.png") );
                ui.tabWidget->setTabText ( 0, tr ("Trash") );
                break;
            default:
                bFill = false;
            }
        }
        break;

    case LIST_TAG:
        {
            QListWidgetItem *pItem = ui.tagWidget->currentItem();
            if (pItem) {
                nTagId = pItem->data (Qt::UserRole).toInt();
            } else {
                bFill = false;
            }
        }
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
        std::map<int, TagItem> TagItems;
        getTagItems(TagItems);

        /* search messages */
        std::list<MsgInfoSummary> msgToShow;
        for(it = msgList.begin(); it != msgList.end(); it++) {
            if (m_eListMode == LIST_BOX) {
                if (bTrash) {
                    if ((it->msgflags & RS_MSG_TRASH) == 0) {
                        continue;
                    }
                } else {
                    if (it->msgflags & RS_MSG_TRASH) {
                        continue;
                    }
                    if ((it->msgflags & RS_MSG_BOXMASK) != msgbox) {
                        continue;
                    }
                }
            } else if (m_eListMode == LIST_TAG) {
                QList<int> tagIds;
                getMessageTags (m_pConfig, QString::fromStdString(it->msgId), tagIds);
                if (qFind(tagIds.begin(), tagIds.end(), nTagId) == tagIds.end()) {
                    continue;
                }
            } else {
                continue;
            }

            msgToShow.push_back(*it);
        }

        /* remove old items */
        int nRowCount = MessagesModel->rowCount();
        int nRow = 0;
        for (nRow = 0; nRow < nRowCount; ) {
            std::string msgIdFromRow = MessagesModel->item(nRow, COLUMN_DATA)->data(ROLE_MSGID).toString().toStdString();
            for(it = msgToShow.begin(); it != msgToShow.end(); it++) {
                if (it->msgId == msgIdFromRow) {
                    break;
                }
            }

            if (it == msgToShow.end ()) {
                MessagesModel->removeRow (nRow);
                nRowCount = MessagesModel->rowCount();
            } else {
                nRow++;
            }
        }

        for(it = msgToShow.begin(); it != msgToShow.end(); it++)
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

            bGotInfo = false;
            msgInfo = MessageInfo(); // clear

            // search exisisting items
            nRowCount = MessagesModel->rowCount();
            for (nRow = 0; nRow < nRowCount; nRow++) {
                if (it->msgId == MessagesModel->item(nRow, COLUMN_DATA)->data(ROLE_MSGID).toString().toStdString()) {
                    break;
                }
            }

            /* make a widget per friend */

            QStandardItem *item [COLUMN_COUNT];

            bool bInsert = false;

            if (nRow < nRowCount) {
                for (int i = 0; i < COLUMN_COUNT; i++) {
                    item[i] = MessagesModel->item(nRow, i);
                }
            } else {
                for (int i = 0; i < COLUMN_COUNT; i++) {
                    item[i] = new QStandardItem();
                }
                bInsert = true;
            }

            //set this false if you want to expand on double click
            for (int i = 0; i < COLUMN_COUNT; i++) {
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
                item[COLUMN_DATE]->setData(qdatetime, ROLE_SORT);
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
                item[COLUMN_FROM]->setData(text + dateString, ROLE_SORT);
            }

            // Subject
            text = QString::fromStdWString(it->title);
            item[COLUMN_SUBJECT]->setText(text);
            item[COLUMN_SUBJECT]->setData(text + dateString, ROLE_SORT);

            // internal data
            QString msgId = QString::fromStdString(it->msgId);
            item[COLUMN_DATA]->setData(QString::fromStdString(it->srcId), ROLE_SRCID);
            item[COLUMN_DATA]->setData(msgId, ROLE_MSGID);

            // Init icon and font
            InitIconAndFont(m_pConfig, item, it->msgflags);

            // Tags
            QList<int> tagIds;
            getMessageTags (m_pConfig, msgId, tagIds);
            qSort(tagIds.begin(), tagIds.end());

            text.clear();

            for (QList<int>::iterator tagId = tagIds.begin(); tagId != tagIds.end(); tagId++) {
                if (text.isEmpty() == false) {
                    text += ",";
                }
                text += TagItems[*tagId].text;
            }
            item[COLUMN_TAGS]->setText(text);

            // set color
            QBrush Brush; // standard
            if (tagIds.size()) {
                Brush = QBrush(TagItems [tagIds [0]].color);
            }
            for (int i = 0; i < COLUMN_COUNT; i++) {
                item[i]->setForeground(Brush);
            }

            // No of Files.
            {
                std::ostringstream out;
                out << it -> count;
                item[COLUMN_ATTACHEMENTS] -> setText(QString::fromStdString(out.str()));
                item[COLUMN_ATTACHEMENTS] -> setData(item[COLUMN_ATTACHEMENTS]->text() + dateString, ROLE_SORT);
                item[COLUMN_ATTACHEMENTS] -> setTextAlignment(Qt::AlignHCenter);
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
                for (int i = 0; i < COLUMN_COUNT; i++) {
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
    ui.messagestreeView->showColumn(COLUMN_READ);
    ui.messagestreeView->showColumn(COLUMN_FROM);
    ui.messagestreeView->showColumn(COLUMN_DATE);
    ui.messagestreeView->showColumn(COLUMN_TAGS);
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
    /* activate row */
    clicked (index);

    /* edit message */
    editmessage();
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

        QString mid = item[COLUMN_DATA]->data(ROLE_MSGID).toString();

        m_pConfig->beginGroup(CONFIG_SECTION_UNREAD);
        if (bRead) {
            // set as read in config
            m_pConfig->setValue(mid, false);
            // set message to read
            rsMsgs->MessageRead(mid.toStdString());
        } else {
            // set as unread in config
            m_pConfig->setValue(mid, true);
        }
        m_pConfig->endGroup();

        InitIconAndFont(m_pConfig, item, 0);
    }
}

void MessagesDialog::markAsRead()
{
    QList<int> RowsUnread;
    getSelectedMsgCount (NULL, NULL, &RowsUnread);

    setMsgAsReadUnread (RowsUnread, true);
    updateMessageSummaryList();
}

void MessagesDialog::markAsUnread()
{
    QList<int> RowsRead;
    getSelectedMsgCount (NULL, &RowsRead, NULL);

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
        QStandardItem *item;
        item = MessagesModel->item(currentIndex.row(),COLUMN_DATA);
        if (item == NULL) {
            return;
        }
        cid = item->data(ROLE_SRCID).toString().toStdString();
        mid = item->data(ROLE_MSGID).toString().toStdString();
    }

    int nCount = getSelectedMsgCount (NULL, NULL, NULL);
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
    tree->setColumnCount(4);

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

    QStandardItem *item = MessagesModel->item(rowSelected,COLUMN_DATA);
    if (item == NULL) {
        return false;
    }
    cid = item->data(ROLE_SRCID).toString().toStdString();
    mid = item->data(ROLE_MSGID).toString().toStdString();
    return true;
}

void MessagesDialog::removemessage()
{
    LockUpdate Lock (this, true);

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

    bool bDelete = false;
    int listrow = ui.listWidget->currentRow();
    if (listrow == ROW_TRASHBOX) {
        bDelete = true;
    } else {
        if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
            bDelete = true;
        }
    }

    for(QList<int>::const_iterator it1 = rowList.begin(); it1 != rowList.end(); it1++) {
        QStandardItem *pItem = MessagesModel->item((*it1), COLUMN_DATA);
        if (pItem) {
            QString mid = pItem->data(ROLE_MSGID).toString();
            if (bDelete) {
                rsMsgs->MessageDelete(mid.toStdString());

                // clean locale config
                m_pConfig->beginGroup(CONFIG_SECTION_UNREAD);
                m_pConfig->remove (mid);
                m_pConfig->endGroup();

                // remove tag
                m_pConfig->beginGroup(CONFIG_SECTION_TAG);
                m_pConfig->remove (mid);
                m_pConfig->endGroup();
            } else {
                rsMsgs->MessageToTrash(mid.toStdString(), true);
            }
        }
    }

    // LockUpdate -> insertMessages();
}

void MessagesDialog::undeletemessage()
{
    LockUpdate (this, true);

    QList<int> Rows;
    getSelectedMsgCount (&Rows, NULL, NULL);
    for (int nRow = 0; nRow < Rows.size(); nRow++) {
        QString mid = MessagesModel->item (Rows [nRow], COLUMN_DATA)->data(ROLE_MSGID).toString();
        rsMsgs->MessageToTrash(mid.toStdString(), false);
    }

    // LockUpdate -> insertMessages();
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

void MessagesDialog::setToolbarButtonStyle(Qt::ToolButtonStyle style)
{
    ui.newmessageButton->setToolButtonStyle(style);
    ui.removemessageButton->setToolButtonStyle(style);
    ui.replymessageButton->setToolButtonStyle(style);
    ui.replyallmessageButton->setToolButtonStyle(style);
    ui.forwardmessageButton->setToolButtonStyle(style);
    ui.tagButton->setToolButtonStyle(style);
    ui.printbutton->setToolButtonStyle(style);
    ui.viewtoolButton->setToolButtonStyle(style);
}

void MessagesDialog::buttonsicononly()
{
    setToolbarButtonStyle(Qt::ToolButtonIconOnly);

    Settings->setValueToGroup("MessageDialog", "ToolButon_Stlye", Qt::ToolButtonIconOnly);
}

void MessagesDialog::buttonstextbesideicon()
{
    setToolbarButtonStyle(Qt::ToolButtonTextBesideIcon);

    Settings->setValueToGroup("MessageDialog", "ToolButon_Stlye", Qt::ToolButtonTextBesideIcon);
}

void MessagesDialog::buttonstextundericon()
{
    setToolbarButtonStyle(Qt::ToolButtonTextUnderIcon);

    Settings->setValueToGroup("MessageDialog", "ToolButon_Stlye", Qt::ToolButtonTextUnderIcon);
}

void MessagesDialog::loadToolButtonsettings()
{
    Qt::ToolButtonStyle style = (Qt::ToolButtonStyle) Settings->valueFromGroup("MessageDialog", "ToolButon_Stlye", Qt::ToolButtonIconOnly).toInt();
    setToolbarButtonStyle(style);
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
    unsigned int trashboxCount = 0;

    /* calculating the new messages */
//    rsMsgs->getMessageCount (&inboxCount, &newInboxCount, &newOutboxCount, &newDraftCount, &newSentboxCount);

    std::list<MsgInfoSummary> msgList;
    std::list<MsgInfoSummary>::const_iterator it;

    rsMsgs->getMessageSummaries(msgList);

    QMap<int, int> tagCount;

    /*calculating the new messages*/
    for (it = msgList.begin(); it != msgList.end(); it++) {
        /* calcluate tag count */
        QList<int> tagIds;
        getMessageTags (m_pConfig, QString::fromStdString(it->msgId), tagIds);
        for (QList<int>::iterator tagId = tagIds.begin(); tagId != tagIds.end(); tagId++) {
            int nCount = tagCount [*tagId];
            nCount++;
            tagCount [*tagId] = nCount;
        }

        /* calculate box */
        if (it->msgflags & RS_MSG_TRASH) {
            trashboxCount++;
            continue;
        }

        switch (it->msgflags & RS_MSG_BOXMASK) {
        case RS_MSG_INBOX:
                inboxCount++;
                if ((it->msgflags & RS_MSG_NEW) == RS_MSG_NEW) {
                    newInboxCount++;
                } else {
                    // check locale config
                    m_pConfig->beginGroup(CONFIG_SECTION_UNREAD);
                    if (m_pConfig->value(QString::fromStdString(it->msgId), false).toBool()) {
                        newInboxCount++;
                    }
                    m_pConfig->endGroup();
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
    

    int listrow = ui.listWidget->currentRow();
    QString textTotal;

    switch (listrow) 
    {
        case ROW_INBOX:
            textTotal = tr("Total:") + " "  + QString::number(inboxCount);
            ui.total_label->setText(textTotal);
            break;
        case ROW_OUTBOX:
            textTotal = tr("Total:") + " "  + QString::number(newOutboxCount);
            ui.total_label->setText(textTotal);
            break;
        case ROW_DRAFTBOX:
            textTotal = tr("Total:") + " "  + QString::number(newDraftCount);
            ui.total_label->setText(textTotal);
            break;
        case ROW_SENTBOX:
            textTotal = tr("Total:") + " "  + QString::number(newSentboxCount);
            ui.total_label->setText(textTotal);
            break;
        case ROW_TRASHBOX:
            textTotal = tr("Total:") + " "  + QString::number(trashboxCount);
            ui.total_label->setText(textTotal);
            break;
    }


    QString textItem;
    /*updating the labels in leftcolumn*/

    //QList<QListWidgetItem *> QListWidget::findItems ( const QString & text, Qt::MatchFlags flags ) const
    QListWidgetItem* item = ui.listWidget->item(ROW_INBOX);
    if (newInboxCount != 0)
    {
        textItem = tr("Inbox") + " (" + QString::number(newInboxCount)+")";
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
    item = ui.listWidget->item(ROW_OUTBOX);
    if (newOutboxCount != 0)
    {
        textItem = tr("Outbox") + " (" + QString::number(newOutboxCount)+")";
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
    item = ui.listWidget->item(ROW_DRAFTBOX);
    if (newDraftCount != 0)
    {
        textItem = tr("Drafts") + " (" + QString::number(newDraftCount)+")";
        item->setText(textItem);
        QFont qf = item->font();
        qf.setBold(true);
        item->setFont(qf);
    }
    else
    {
        textItem = tr("Drafts");
        item->setText(textItem);
        QFont qf = item->font();
        qf.setBold(false);
        item->setFont(qf);

    }

    item = ui.listWidget->item(ROW_TRASHBOX);
    if (trashboxCount != 0)
    {
        textItem = tr("Trash") + " (" + QString::number(trashboxCount)+")";
        item->setText(textItem);
    }
    else
    {
        textItem = tr("Trash");
        item->setText(textItem);
    }


    /* set tag counts */
    int nRowCount = ui.tagWidget->count();
    for (int nRow = 0; nRow < nRowCount; nRow++) {
        QListWidgetItem *pItem = ui.tagWidget->item(nRow);
        int nCount = tagCount[pItem->data(Qt::UserRole).toInt()];

        QString sText = pItem->data(Qt::UserRole + 1).toString();
        if (nCount) {
            sText += " (" + QString::number(nCount) + ")";
        }

        pItem->setText(sText);
    }
}

/** clear Filter **/
void MessagesDialog::clearFilter()
{
    ui.filterPatternLineEdit->clear();
    ui.filterPatternLineEdit->setFocus();
}

void MessagesDialog::tagAboutToShow()
{
    // activate actions from the first selected row
    QList<int> tagIds;

    QList<int> Rows;
    getSelectedMsgCount (&Rows, NULL, NULL);

    if (Rows.size()) {
        QStandardItem* pItem = MessagesModel->item(Rows [0], COLUMN_DATA);
        QString msgId = pItem->data(ROLE_MSGID).toString();

        getMessageTags(m_pConfig, msgId, tagIds);
    }

    QMenu *pMenu = ui.tagButton->menu();

    foreach(QObject *pObject, pMenu->children()) {
        QAction *pAction = qobject_cast<QAction*> (pObject);
        if (pAction == NULL) {
            continue;
        }

        const QMap<QString, QVariant> &Values = pAction->data().toMap();
        if (Values.size () != ACTION_TAGSINDEX_SIZE) {
            continue;
        }
        if (Values [ACTION_TAGSINDEX_TYPE] != ACTION_TAGS_TAG) {
            continue;
        }

        QList<int>::iterator tagId = qFind(tagIds.begin(), tagIds.end(), Values [ACTION_TAGSINDEX_ID]);
        pAction->setChecked(tagId != tagIds.end());
    }
}

void MessagesDialog::tagTriggered(QAction *pAction)
{
    if (pAction == NULL) {
        return;
    }

    const QMap<QString, QVariant> &Values = pAction->data().toMap();
    if (Values.size () != ACTION_TAGSINDEX_SIZE) {
        return;
    }

    bool bRemoveAll = false;
    int nId = 0;
    bool bSet = false;

    if (Values [ACTION_TAGSINDEX_TYPE] == ACTION_TAGS_REMOVEALL) {
        // remove all tags
        bRemoveAll = true;
    } else if (Values [ACTION_TAGSINDEX_TYPE] == ACTION_TAGS_NEWTAG) {
        // new tag
        std::map<int, TagItem> TagItems;
        getTagItems(TagItems);

        NewTag Tag(TagItems);
        if (Tag.exec() == QDialog::Accepted && Tag.m_nId) {
            // Tag.m_nId
            setTagItems (TagItems);
            nId = Tag.m_nId;
            bSet = true;
        } else {
            return;
        }
    }  else if (Values [ACTION_TAGSINDEX_TYPE].toInt() == ACTION_TAGS_TAG) {
        nId = Values [ACTION_TAGSINDEX_ID].toInt();
        if (nId == 0) {
            return;
        }
        bSet = pAction->isChecked();
    } else {
        return;
    }

    QList<int> Rows;
    getSelectedMsgCount (&Rows, NULL, NULL);
    for (int nRow = 0; nRow < Rows.size(); nRow++) {
        QStandardItem* pItem = MessagesModel->item(Rows [nRow], COLUMN_DATA);
        QString msgId = pItem->data(ROLE_MSGID).toString();

        if (bRemoveAll) {
            // remove all
            m_pConfig->beginGroup(CONFIG_SECTION_TAG);
            m_pConfig->remove (msgId);
            m_pConfig->endGroup();

            insertMessages();
        } else {
            // set or unset tag
            QList<int> tagIds;
            getMessageTags(m_pConfig, msgId, tagIds);

            QList<int>::iterator tagId = qFind(tagIds.begin(), tagIds.end(), nId);

            bool bSaveAndRefresh = false;
            if (bSet) {
                if (tagId == tagIds.end()) {
                    // not found
                    tagIds.push_back(nId);
                    bSaveAndRefresh = true;
                }
            } else {
                if (tagId != tagIds.end()) {
                    // found
                    tagIds.erase(tagId);
                    bSaveAndRefresh = true;
                }
            }

            if (bSaveAndRefresh) {
                setMessageTags(m_pConfig, msgId, tagIds);
                insertMessages();
            }
        }
    }
}
