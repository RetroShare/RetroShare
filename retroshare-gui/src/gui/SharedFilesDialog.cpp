/****************************************************************
 *  RShare is distributed under the following license:
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

#include <QString>
#include <QTreeView>
#include <QClipboard>
#include <QMenu>
#include <QProcess>
#include <QSortFilterProxyModel>

#include "SharedFilesDialog.h"
#include "settings/AddFileAssociationDialog.h"
#include "util/RsAction.h"
#include "msgs/MessageComposer.h"
#include "settings/rsharesettings.h"
#ifdef RS_USE_LINKS
#include "AddLinksDialog.h"
#endif
#include "RetroShareLink.h"
#include "ShareManager.h"
#include "RemoteDirModel.h"
#include "ShareDialog.h"
#include "common/PeerDefs.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsfiles.h>


/* Images for context menu icons */
#define IMAGE_DOWNLOAD       ":/images/download16.png"
#define IMAGE_PLAY           ":/images/start.png"
#define IMAGE_HASH_BUSY      ":/images/settings.png"
#define IMAGE_HASH_DONE      ":/images/accepted16.png"
#define IMAGE_MSG            ":/images/message-mail.png"
#define IMAGE_ATTACHMENT     ":/images/attachment.png"
#define IMAGE_FRIEND         ":/images/peers_16x16.png"
#define IMAGE_PROGRESS       ":/images/browse-looking.gif"
#define IMAGE_COPYLINK       ":/images/copyrslink.png"
#define IMAGE_OPENFOLDER     ":/images/folderopen.png"
#define IMAGE_OPENFILE		 ":/images/fileopen.png"
#define IMAGE_COLLECTION     ":/images/mimetypes/rscollection-16.png"
#define IMAGE_EDITSHARE      ":/images/edit_16.png"
#define IMAGE_MYFILES        ":images/my_documents_22.png"

// Define to avoid using the search in treeview, because it is really slow for now.
//
#define DONT_USE_SEARCH_IN_TREE_VIEW 1

const QString Image_AddNewAssotiationForFile = ":/images/kcmsystem24.png";

class SFDSortFilterProxyModel : public QSortFilterProxyModel
{
public:
    SFDSortFilterProxyModel(RetroshareDirModel *dirModel, QObject *parent) : QSortFilterProxyModel(parent)
    {
        m_dirModel = dirModel;
    };

protected:
    virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const
    {
        bool dirLeft = (m_dirModel->getType(left) == DIR_TYPE_DIR);
        bool dirRight = (m_dirModel->getType(right) == DIR_TYPE_DIR);

        if (dirLeft ^ dirRight) {
            return dirLeft;
        }

        return QSortFilterProxyModel::lessThan(left, right);
    }

private:
    RetroshareDirModel *m_dirModel;
};

/** Constructor */
SharedFilesDialog::SharedFilesDialog(RetroshareDirModel *_tree_model,RetroshareDirModel *_flat_model,QWidget *parent)
: RsAutoUpdatePage(1000,parent),model(NULL)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

//==	connect(ui.localButton, SIGNAL(toggled(bool)), this, SLOT(showFrame(bool)));
//==	connect(ui.remoteButton, SIGNAL(toggled(bool)), this, SLOT(showFrameRemote(bool)));
//==	connect(ui.splittedButton, SIGNAL(toggled(bool)), this, SLOT(showFrameSplitted(bool)));
	connect(ui.viewType_CB, SIGNAL(currentIndexChanged(int)), this, SLOT(changeCurrentViewModel(int)));

	connect( ui.dirTreeView, SIGNAL( customContextMenuRequested( QPoint ) ), this,  SLOT( spawnCustomPopupMenu( QPoint ) ) );

	connect(ui.indicatorCBox, SIGNAL(currentIndexChanged(int)), this, SLOT(indicatorChanged(int)));

	tree_model = _tree_model ;
	flat_model = _flat_model ;

	tree_proxyModel = new SFDSortFilterProxyModel(tree_model, this);
	tree_proxyModel->setDynamicSortFilter(true);
	tree_proxyModel->setSourceModel(tree_model);
	tree_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
	tree_proxyModel->setSortRole(RetroshareDirModel::SortRole);
	tree_proxyModel->sort(0);

	flat_proxyModel = new SFDSortFilterProxyModel(flat_model, this);
	flat_proxyModel->setDynamicSortFilter(true);
	flat_proxyModel->setSourceModel(flat_model);
	flat_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
	flat_proxyModel->setSortRole(RetroshareDirModel::SortRole);
	flat_proxyModel->sort(0);

	connect(ui.filterClearButton, SIGNAL(clicked()), this, SLOT(clearFilter()));
	connect(ui.filterStartButton, SIGNAL(clicked()), this, SLOT(startFilter()));
	connect(ui.filterPatternLineEdit, SIGNAL(returnPressed()), this, SLOT(startFilter()));
	connect(ui.filterPatternLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(filterRegExpChanged()));

	/* Set header resize modes and initial section sizes  */
	QHeaderView * header = ui.dirTreeView->header () ;

	header->resizeSection ( 0, 490 );
	header->resizeSection ( 1, 70 );
	header->resizeSection ( 2, 100 );
	header->resizeSection ( 3, 100 );
	header->resizeSection ( 4, 100 );

	header->setStretchLastSection(false);

	/* Set Multi Selection */
	ui.dirTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);

  /* Hide platform specific features */
  copylinkAct = new QAction(QIcon(IMAGE_COPYLINK), tr( "Copy retroshare Links to Clipboard" ), this );
  connect( copylinkAct , SIGNAL( triggered() ), this, SLOT( copyLink() ) );
  copylinkhtmlAct = new QAction(QIcon(IMAGE_COPYLINK), tr( "Copy retroshare Links to Clipboard (HTML)" ), this );
  connect( copylinkhtmlAct , SIGNAL( triggered() ), this, SLOT( copyLinkhtml() ) );
  sendlinkAct = new QAction(QIcon(IMAGE_COPYLINK), tr( "Send retroshare Links" ), this );
  connect( sendlinkAct , SIGNAL( triggered() ), this, SLOT( sendLinkTo( ) ) );
#ifdef RS_USE_LINKS
  sendlinkCloudAct = new QAction(QIcon(IMAGE_COPYLINK), tr( "Send retroshare Links to Cloud" ), this );
  connect( sendlinkCloudAct , SIGNAL( triggered() ), this, SLOT( sendLinkToCloud(  ) ) );
  addlinkCloudAct = new QAction(QIcon(IMAGE_COPYLINK), tr( "Add Links to Cloud" ), this );
  connect( addlinkCloudAct , SIGNAL( triggered() ), this, SLOT( addLinkToCloud(  ) ) );
#endif
}

LocalSharedFilesDialog::LocalSharedFilesDialog(QWidget *parent)
	: SharedFilesDialog(new TreeStyle_RDM(false),new FlatStyle_RDM(false),parent)
{
	// Hide columns after loading the settings
	ui.dirTreeView->setColumnHidden(4,false) ;
	ui.downloadButton->hide() ;

	// load settings
	processSettings(true);
	// Setup the current view model.
	//
	changeCurrentViewModel(ui.viewType_CB->currentIndex()) ;

  connect(ui.addShares_PB, SIGNAL(clicked()), this, SLOT(addShares()));
  connect(ui.checkButton, SIGNAL(clicked()), this, SLOT(forceCheck()));

  createcollectionfileAct = new QAction(QIcon(IMAGE_COLLECTION), tr("Create collection file"), this);
  openfileAct = new QAction(QIcon(IMAGE_OPENFILE), tr("Open File"), this);
  openfolderAct = new QAction(QIcon(IMAGE_OPENFOLDER), tr("Open Folder"), this);
  editshareAct = new QAction(QIcon(IMAGE_EDITSHARE), tr("Edit Share Permissions"), this);

  connect(createcollectionfileAct, SIGNAL(triggered()), this, SLOT(createCollectionFile()));
  connect(openfileAct, SIGNAL(triggered()), this, SLOT(openfile()));
  connect(openfolderAct, SIGNAL(triggered()), this, SLOT(openfolder()));
  connect(editshareAct, SIGNAL(triggered()), this, SLOT(editSharePermissions()));

  ui.titleBarPixmap->setPixmap(QPixmap(IMAGE_MYFILES));
}

RemoteSharedFilesDialog::RemoteSharedFilesDialog(QWidget *parent)
	: SharedFilesDialog(new TreeStyle_RDM(true),new FlatStyle_RDM(true),parent)
{
	ui.dirTreeView->setColumnHidden(3,false) ;
	ui.dirTreeView->setColumnHidden(4,true) ;
	ui.checkButton->hide() ;

	connect(ui.downloadButton, SIGNAL(clicked()), this, SLOT(downloadRemoteSelected()));

	// load settings
	processSettings(true);
	// Setup the current view model.
	//
	changeCurrentViewModel(ui.viewType_CB->currentIndex()) ;

	ui.addShares_PB->hide() ;
}

void LocalSharedFilesDialog::addShares()
{
	ShareManager::showYourself();
}

void SharedFilesDialog::hideEvent(QHideEvent *)
{
	if(model!=NULL)
		model->setVisible(false) ;
}
void SharedFilesDialog::showEvent(QShowEvent *)
{
	if(model!=NULL)
	{
		model->setVisible(true) ;
		model->update() ;
	}
}
RemoteSharedFilesDialog::~RemoteSharedFilesDialog()
{
    // save settings
    processSettings(false);
}

LocalSharedFilesDialog::~LocalSharedFilesDialog()
{
    // save settings
    processSettings(false);
}

void LocalSharedFilesDialog::processSettings(bool bLoad)
{
	Settings->beginGroup("LocalSharedFilesDialog");

	if (bLoad) {
		// load settings

		// state of the trees
		ui.dirTreeView->header()->restoreState(Settings->value("LocalDirTreeView").toByteArray());

		// state of splitter
		ui.splitter->restoreState(Settings->value("LocalSplitter").toByteArray());

		// view type
		ui.viewType_CB->setCurrentIndex(Settings->value("LocalViewType").toInt());
	} else {
		// save settings

		// state of trees
		Settings->setValue("LocalDirTreeView", ui.dirTreeView->header()->saveState());

		// state of splitter
		Settings->setValue("LocalSplitter", ui.splitter->saveState());

		// view type
		Settings->setValue("LocalViewType", ui.viewType_CB->currentIndex());
	}

	Settings->endGroup();
}
void RemoteSharedFilesDialog::processSettings(bool bLoad)
{
	Settings->beginGroup("SharedFilesDialog");

	if (bLoad) {
		// load settings

		// state of the trees
		ui.dirTreeView->header()->restoreState(Settings->value("RemoteDirTreeView").toByteArray());

		// state of splitter
		ui.splitter->restoreState(Settings->value("RemoteSplitter").toByteArray());

		// view type
		ui.viewType_CB->setCurrentIndex(Settings->value("RemoteViewType").toInt());
	} else {
		// save settings

		// state of trees
		Settings->setValue("RemoteDirTreeView", ui.dirTreeView->header()->saveState());

		// state of splitter
		Settings->setValue("RemoteSplitter", ui.splitter->saveState());

		// view type
		Settings->setValue("RemoteViewType", ui.viewType_CB->currentIndex());
	}

	Settings->endGroup();
}

void SharedFilesDialog::changeCurrentViewModel(int c)
{
	disconnect( ui.dirTreeView, SIGNAL( collapsed(const QModelIndex & ) ), 0, 0 );
	disconnect( ui.dirTreeView, SIGNAL(  expanded(const QModelIndex & ) ), 0, 0 );

	if(model!=NULL)
		model->setVisible(false) ;

	if(c==0)
	{
		model = tree_model ;
		proxyModel = tree_proxyModel ;
	}
	else
	{
		model = flat_model ;
		proxyModel = flat_proxyModel ;
	}

	showProperColumns() ;

	if(isVisible())
	{
		model->setVisible(true) ;
		model->update() ;
	}

	connect( ui.dirTreeView, SIGNAL( collapsed(const QModelIndex & ) ), model, SLOT(  collapsed(const QModelIndex & ) ) );
	connect( ui.dirTreeView, SIGNAL(  expanded(const QModelIndex & ) ), model, SLOT(   expanded(const QModelIndex & ) ) );

	ui.dirTreeView->setModel(proxyModel);
	ui.dirTreeView->update();

	QHeaderView * header = ui.dirTreeView->header () ;
	header->setResizeMode (0, QHeaderView::Interactive);

	ui.dirTreeView->header()->headerDataChanged(Qt::Horizontal,0,4) ;

#ifdef DONT_USE_SEARCH_IN_TREE_VIEW
	if(c == 1)
#endif
		FilterItems();
}

void LocalSharedFilesDialog::showProperColumns()
{
	if(model == tree_model)
	{
		ui.dirTreeView->setColumnHidden(3,false) ;
		ui.dirTreeView->setColumnHidden(4,false) ;
#ifdef DONT_USE_SEARCH_IN_TREE_VIEW
		ui.filterLabel->hide();
		ui.filterPatternLineEdit->hide();
		ui.filterStartButton->hide();
		ui.filterClearButton->hide();
#endif
	}
	else
	{
		ui.dirTreeView->setColumnHidden(3,true) ;
		ui.dirTreeView->setColumnHidden(4,false) ;
#ifdef DONT_USE_SEARCH_IN_TREE_VIEW
		ui.filterLabel->show();
		ui.filterPatternLineEdit->show();
#endif
	}
}
void RemoteSharedFilesDialog::showProperColumns()
{
	if(model == tree_model)
	{
		ui.dirTreeView->setColumnHidden(3,true) ;
		ui.dirTreeView->setColumnHidden(4,true) ;
#ifdef DONT_USE_SEARCH_IN_TREE_VIEW
		ui.filterLabel->hide();
		ui.filterPatternLineEdit->hide();
		ui.filterStartButton->hide();
		ui.filterClearButton->hide();
#endif
	}
	else
	{
		ui.dirTreeView->setColumnHidden(3,false) ;
		ui.dirTreeView->setColumnHidden(4,false) ;
#ifdef DONT_USE_SEARCH_IN_TREE_VIEW
		ui.filterLabel->show();
		ui.filterPatternLineEdit->show();
#endif
	}
}

void LocalSharedFilesDialog::checkUpdate()
{
	/* update */
	if (rsFiles->InDirectoryCheck())
	{
		ui.checkButton->setText(tr("Checking..."));
	}
	else
	{
		ui.checkButton->setText(tr("Check files"));
		ui.hashLabel->setPixmap(QPixmap(IMAGE_HASH_DONE));
		ui.hashLabel->setToolTip("") ;
	}

	return;
}

void LocalSharedFilesDialog::forceCheck()
{
	rsFiles->ForceDirectoryCheck();
	return;
}

void RemoteSharedFilesDialog::spawnCustomPopupMenu( QPoint point )
{
    QModelIndex idx = ui.dirTreeView->indexAt(point);
    if (!idx.isValid())
        return;
    QModelIndex midx = proxyModel->mapToSource(idx);
    if (!midx.isValid())
        return;

    int type = model->getType(midx);
    if (type != DIR_TYPE_DIR && type != DIR_TYPE_FILE) {
        return;
    }

    QMenu contextMnu( this );

    QAction *downloadAct = new QAction(QIcon(IMAGE_DOWNLOAD), tr( "Download" ), &contextMnu );
    connect( downloadAct , SIGNAL( triggered() ), this, SLOT( downloadRemoteSelected() ) );
    contextMnu.addAction( downloadAct);

    if (type == DIR_TYPE_FILE) {
        //QAction *copyremotelinkAct = new QAction(QIcon(IMAGE_COPYLINK), tr( "Copy retroshare Link" ), &contextMnu );
        //connect( copyremotelinkAct , SIGNAL( triggered() ), this, SLOT( copyLink() ) );

        //QAction *sendremotelinkAct = new QAction(QIcon(IMAGE_COPYLINK), tr( "Send retroshare Link" ), &contextMnu );
        //connect( sendremotelinkAct , SIGNAL( triggered() ), this, SLOT( sendremoteLinkTo(  ) ) );

        contextMnu.addSeparator();
        contextMnu.addAction( copylinkAct);
        contextMnu.addAction( sendlinkAct);
        contextMnu.addSeparator();
        contextMnu.addAction(QIcon(IMAGE_MSG), tr("Recommend in a message to"), this, SLOT(recommendFilesToMsg()));
    }

    contextMnu.exec(QCursor::pos());
}

QModelIndexList SharedFilesDialog::getSelected()
{
    QModelIndexList list = ui.dirTreeView->selectionModel()->selectedIndexes();
    QModelIndexList proxyList;
    for (QModelIndexList::iterator index = list.begin(); index != list.end(); index++) {
        proxyList.append(proxyModel->mapToSource(*index));
    }

    return proxyList;
}

void LocalSharedFilesDialog::createCollectionFile()
{
	/* call back to the model (which does all the interfacing? */

	std::cerr << "Creating a collection file!" << std::endl;
	QModelIndexList lst = getSelected();
	model->createCollectionFile(this, lst);
}

void RemoteSharedFilesDialog::downloadRemoteSelected()
{
  /* call back to the model (which does all the interfacing? */

  std::cerr << "Downloading Files";
  std::cerr << std::endl;

  QModelIndexList lst = getSelected();
  model -> downloadSelected(lst);
}

void LocalSharedFilesDialog::editSharePermissions()
{
	std::list<SharedDirInfo> dirs;
	rsFiles->getSharedDirectories(dirs);

	std::list<SharedDirInfo>::const_iterator it;
	for (it = dirs.begin(); it != dirs.end(); it++) {
		if (currentFile == currentFile) {
			/* file name found, show dialog */
			ShareDialog sharedlg (it->filename, this);
			sharedlg.setWindowTitle(tr("Edit Shared Folder"));
			sharedlg.exec();
			break;
		}
	}

}

void SharedFilesDialog::copyLink (const QModelIndexList& lst, bool remote)
{
    std::vector<DirDetails> dirVec;

	 model->getDirDetailsFromSelect(lst, dirVec);

    QList<RetroShareLink> urls ;

    for (int i = 0, n = dirVec.size(); i < n; ++i)
    {
        const DirDetails& details = dirVec[i];

        if (details.type == DIR_TYPE_DIR)
        {
            for (std::list<DirStub>::const_iterator cit = details.children.begin();cit != details.children.end(); ++cit)
            {
                const DirStub& dirStub = *cit;

                DirDetails details;
                FileSearchFlags flags = remote?RS_FILE_HINTS_REMOTE:RS_FILE_HINTS_LOCAL ;

                // do not recursive copy sub dirs.
                if (!rsFiles->RequestDirDetails(dirStub.ref, details, flags) || details.type != DIR_TYPE_FILE)
                    continue;

                RetroShareLink link;
                if (link.createFile(QString::fromUtf8(details.name.c_str()), details.count, details.hash.c_str())) {
                    urls.push_back(link) ;
                }
            }
        }
        else
        {
            RetroShareLink link;
            if (link.createFile(QString::fromUtf8(details.name.c_str()), details.count, details.hash.c_str())) {
                urls.push_back(link) ;
            }
        }
    }
    RSLinkClipboard::copyLinks(urls) ;
}

void SharedFilesDialog::copyLink()
{
    copyLink ( getSelected() , isRemote());
}

void SharedFilesDialog::copyLinkhtml( )
{
    copyLink();

    QString link = QApplication::clipboard()->text();

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText("<a href='" +  link + "'> " + link + "</a>");
}

void SharedFilesDialog::sendLinkTo()
{
    copyLink();

    /* create a message */
    MessageComposer *nMsgDialog = MessageComposer::newMsg();
    if (nMsgDialog == NULL) {
        return;
    }

    /* fill it in
    * files are receommended already
    * just need to set peers
    */
    std::cerr << "SharedFilesDialog::sendLinkTo()" << std::endl;
    nMsgDialog->setTitleText(tr("RetroShare Link"));
    nMsgDialog->setMsgText(RSLinkClipboard::toHtml(), true);

    nMsgDialog->show();

    /* window will destroy itself! */
}

#ifdef RS_USE_LINKS
void SharedFilesDialog::sendLinkToCloud()
{
	copyLink();

	AddLinksDialog *nAddLinksDialog = new AddLinksDialog(QApplication::clipboard()->text());

	nAddLinksDialog->addLinkComment();
	nAddLinksDialog->close();

	/* window will destroy itself! */
}

void SharedFilesDialog::addLinkToCloud()
{
	copyLink();

	AddLinksDialog *nAddLinksDialog = new AddLinksDialog(QApplication::clipboard()->text());

	nAddLinksDialog->show();

	/* window will destroy itself! */
}
#endif

void LocalSharedFilesDialog::playselectedfiles()
{
  /* call back to the model (which does all the interfacing? */

  std::cerr << "SharedFilesDialog::playselectedfiles()";
  std::cerr << std::endl;

  std::list<std::string> paths;
  model -> getFilePaths(getSelected(), paths);

  std::list<std::string>::iterator it;
  QStringList fullpaths;
  for(it = paths.begin(); it != paths.end(); it++)
  {
	  std::string fullpath;
	  rsFiles->ConvertSharedFilePath(*it, fullpath);
	  fullpaths.push_back(QString::fromStdString(fullpath));

	  std::cerr << "Playing: " << fullpath;
	  std::cerr << std::endl;
  }

  playFiles(fullpaths);

  std::cerr << "SharedFilesDialog::playselectedfiles() Completed";
  std::cerr << std::endl;
}

void SharedFilesDialog::recommendFilesToMsg()
{
    std::list<DirDetails> files_info ;

    model->getFileInfoFromIndexList(getSelected(),files_info);

    if(files_info.empty())
        return ;

    /* create a message */

    MessageComposer *nMsgDialog = MessageComposer::newMsg();
    if (nMsgDialog == NULL) {
        return;
    }

    nMsgDialog->setFileList(files_info) ;
    nMsgDialog->setTitleText(tr("Recommendation(s)"));
    nMsgDialog->setMsgText(tr("Recommendation(s)"));
    nMsgDialog->show();

    /* window will destroy itself! */
}

void LocalSharedFilesDialog::openfile()
{
	/* call back to the model (which does all the interfacing? */

	std::cerr << "SharedFilesDialog::openfile" << std::endl;

	QModelIndexList qmil = getSelected();
	model->openSelected(qmil);
}


void LocalSharedFilesDialog::openfolder()
{
	std::cerr << "SharedFilesDialog::openfolder" << std::endl;

	QModelIndexList qmil = getSelected();
	model->openSelected(qmil);
}

void  SharedFilesDialog::preModDirectories()
{
	model->preMods();
}

void  SharedFilesDialog::postModDirectories()
{
	model->postMods();
	ui.dirTreeView->update() ;

	if (ui.filterPatternLineEdit->text().isEmpty() == false) 
		FilterItems();

	QCoreApplication::flush();
}


void LocalSharedFilesDialog::spawnCustomPopupMenu( QPoint point )
{
    if (!rsPeers) 
	 {
        /* not ready yet! */
        return;
    }

    QModelIndex idx = ui.dirTreeView->indexAt(point);
    if (!idx.isValid())
        return;
    QModelIndex midx = proxyModel->mapToSource(idx);
    if (!midx.isValid())
        return;

    currentFile = model->data(midx, RetroshareDirModel::FileNameRole).toString();

    int type = model->getType(midx);

    QMenu contextMnu(this);

    switch (type) 
	 {
		 case DIR_TYPE_DIR:
			 contextMnu.addAction(openfolderAct);
			 contextMnu.addSeparator() ;
			 contextMnu.addAction(createcollectionfileAct) ;
			 break;
		 case DIR_TYPE_FILE:
			 contextMnu.addAction(openfileAct);
			 contextMnu.addSeparator();
			 contextMnu.addAction(copylinkAct);
			 contextMnu.addAction(sendlinkAct);
			 contextMnu.addSeparator();
			 contextMnu.addAction(createcollectionfileAct) ;
			 contextMnu.addSeparator();
#ifdef RS_USE_LINKS
			 contextMnu.addAction(sendlinkCloudAct);
			 contextMnu.addAction(addlinkCloudAct);
			 contextMnu.addSeparator();
#endif
			 contextMnu.addSeparator();
			 contextMnu.addAction(QIcon(IMAGE_MSG), tr("Recommend in a message to"), this, SLOT(recommendFilesToMsg()));
			 break;
		 default:
			 return;
	 }

    contextMnu.exec(QCursor::pos());
}

//============================================================================

QAction*
LocalSharedFilesDialog::fileAssotiationAction(const QString /*fileName*/)
{
    QAction* result = 0;

    Settings->beginGroup("FileAssotiations");

    QString key = AddFileAssociationDialog::cleanFileType(currentFile) ;
    if ( Settings->contains(key) )
    {
        result = new QAction(QIcon(IMAGE_PLAY), tr( "Open File" ), this );
        connect( result , SIGNAL( triggered() ),
                 this, SLOT( runCommandForFile() ) );

        currentCommand = (Settings->value( key )).toString();
    }
    else
    {
        result = new QAction(QIcon(Image_AddNewAssotiationForFile),
                             tr( "Set command for opening this file"), this );
        connect( result , SIGNAL( triggered() ),
                 this,    SLOT(   tryToAddNewAssotiation() ) );
    }

    Settings->endGroup();

    return result;
}

//============================================================================

void
LocalSharedFilesDialog::runCommandForFile()
{
    QStringList tsl;
    tsl.append( currentFile );
    QProcess::execute( currentCommand, tsl);
}

//============================================================================

void
LocalSharedFilesDialog::tryToAddNewAssotiation()
{
    AddFileAssociationDialog afad(true, this);//'add file assotiations' dialog

    afad.setFileType(AddFileAssociationDialog::cleanFileType(currentFile));

    int ti = afad.exec();

    if (ti==QDialog::Accepted)
    {
        QString currType = afad.resultFileType() ;
        QString currCmd = afad.resultCommand() ;

        Settings->setValueToGroup("FileAssotiations", currType, currCmd);
    }
}

void SharedFilesDialog::indicatorChanged(int index)
{
	static uint32_t correct_indicator[4] = { IND_ALWAYS,IND_LAST_DAY,IND_LAST_WEEK,IND_LAST_MONTH } ;

	model->changeAgeIndicator(correct_indicator[index]);

	ui.dirTreeView->update(ui.dirTreeView->rootIndex());
  
	if (correct_indicator[index] != IND_ALWAYS)
		ui.dirTreeView->sortByColumn(2, Qt::AscendingOrder);
	else
		ui.dirTreeView->sortByColumn(0, Qt::AscendingOrder);

	updateDisplay() ;
}

void SharedFilesDialog::filterRegExpChanged()
{
    QString text = ui.filterPatternLineEdit->text();

    if (text.isEmpty()) {
        ui.filterClearButton->hide();
    } else {
        ui.filterClearButton->show();
    }

    if (text == lastFilterString) {
        ui.filterStartButton->hide();
    } else {
        ui.filterStartButton->show();
    }
}

/* clear Filter */
void SharedFilesDialog::clearFilter()
{
    ui.filterPatternLineEdit->clear();
    ui.filterPatternLineEdit->setFocus();

    startFilter();
}

/* clear Filter */
void SharedFilesDialog::startFilter()
{
    ui.filterStartButton->hide();
    lastFilterString = ui.filterPatternLineEdit->text();

    FilterItems();
}

void SharedFilesDialog::FilterItems()
{
    QString text = ui.filterPatternLineEdit->text();

    setCursor(Qt::WaitCursor);
	 QCoreApplication::processEvents() ;

    int rowCount = ui.dirTreeView->model()->rowCount();
    for (int row = 0; row < rowCount; row++) 
		 if(proxyModel == tree_proxyModel)
			 tree_FilterItem(ui.dirTreeView->model()->index(row, 0), text, 0);
		 else
			 flat_FilterItem(ui.dirTreeView->model()->index(row, 0), text, 0);

    setCursor(Qt::ArrowCursor);
}

bool SharedFilesDialog::flat_FilterItem(const QModelIndex &index, const QString &text, int /*level*/)
{
	if(index.data(RetroshareDirModel::FileNameRole).toString().contains(text, Qt::CaseInsensitive)) 
	{
		ui.dirTreeView->setRowHidden(index.row(), index.parent(), false);
		return false ;
	}
	else 
	{
		ui.dirTreeView->setRowHidden(index.row(), index.parent(), true);
		return true ;
	}
}

bool SharedFilesDialog::tree_FilterItem(const QModelIndex &index, const QString &text, int level)
{
    bool visible = true;

    if (text.isEmpty() == false) {
        // better use RetroshareDirModel::getType, but its slow enough
        if (/*index.parent().isValid()*/ level >= 1) {
            if (index.data(RetroshareDirModel::FileNameRole).toString().contains(text, Qt::CaseInsensitive) == false) {
                visible = false;
            }
        } else {
            visible = false;
        }
    }

    int visibleChildCount = 0;
    int rowCount = ui.dirTreeView->model()->rowCount(index);
    for (int row = 0; row < rowCount; row++) {
        if (tree_FilterItem(ui.dirTreeView->model()->index(row, index.column(), index), text, level + 1)) {
            visibleChildCount++;
        }
    }

    if (visible || visibleChildCount) {
        ui.dirTreeView->setRowHidden(index.row(), index.parent(), false);
    } else {
        ui.dirTreeView->setRowHidden(index.row(), index.parent(), true);
    }

    return (visible || visibleChildCount);
}

