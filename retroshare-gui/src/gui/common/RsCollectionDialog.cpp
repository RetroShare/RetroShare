/*******************************************************************************
 * gui/common/RsCollectionDialog.cpp                                           *
 *                                                                             *
 * Copyright (C) 2011, Retroshare Team <retroshare.project@gmail.com>          *
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

#include "RsCollectionDialog.h"

#include "RsCollection.h"
#include "util/misc.h"

#include <QCheckBox>
#include <QDateTime>
#include <QDir>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMenu>
#include <QTextEdit>
#include <QTreeView>

#define COLUMN_FILE     0
#define COLUMN_FILEPATH 1
#define COLUMN_SIZE     2
#define COLUMN_HASH     3
#define COLUMN_FILEC    4
#define COLUMN_COUNT    5
// In COLUMN_HASH (COLUMN_FILE reserved for CheckState)
#define ROLE_NAME Qt::UserRole + 1
#define ROLE_PATH Qt::UserRole + 2
#define ROLE_TYPE Qt::UserRole + 3
// In COLUMN_SIZE (don't start from 1 to read)
#define ROLE_SIZE Qt::UserRole + 4
#define ROLE_SELSIZE Qt::UserRole + 5
// In COLUMN_FILEC (don't start from 1 to read)
#define ROLE_FILEC Qt::UserRole + 6
#define ROLE_SELFILEC Qt::UserRole + 7

#define MAX_FILE_ADDED_BEFORE_ASK  500 //Number of file added in Recursive mode before asking to continue

#define IMAGE_SEARCH               ":/icons/svg/magnifying-glass.svg"

/**
 * @brief The FSMSortFilterProxyModel class sort directory before file.
 */
class FSMSortFilterProxyModel : public QSortFilterProxyModel
{
public:
	FSMSortFilterProxyModel( QObject *parent) : QSortFilterProxyModel(parent)
	{}

protected:
	virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const
	{
		QFileSystemModel *fsm = qobject_cast<QFileSystemModel*>(sourceModel());
		bool asc = (sortOrder() == Qt::AscendingOrder ? true : false) ;

		QFileInfo leftFileInfo  = fsm->fileInfo(left);
		QFileInfo rightFileInfo = fsm->fileInfo(right);

		// If Dot move in the beginning
		if (sourceModel()->data(left).toString() == ".")
			return asc;
		if (sourceModel()->data(right).toString() == ".")
			return !asc;

		// If DotAndDot move in the beginning
		if (sourceModel()->data(left).toString() == "..")
			return asc;
		if (sourceModel()->data(right).toString() == "..")
			return !asc;

		// Move dirs upper
		if (!leftFileInfo.isDir() && rightFileInfo.isDir())
			return !asc;
		if (leftFileInfo.isDir() && !rightFileInfo.isDir())
			return asc;


		/*If sorting by Size (Take real size, not Display one 10<2)*/
		if ((sortColumn()==1) && (!leftFileInfo.isDir() && !rightFileInfo.isDir())) {
			if (leftFileInfo.size() < rightFileInfo.size())
				return true;
			if (leftFileInfo.size() > rightFileInfo.size())
				return false;
		}
		/*If sorting by Date Modified (Take real date, not Display one 01-10-2014<02-01-1980)*/
		if (sortColumn()==3) {
			if (leftFileInfo.lastModified() < rightFileInfo.lastModified())
				return true;
			if (leftFileInfo.lastModified() > rightFileInfo.lastModified())
				return false;
		}
		//Columns found here:https://qt.gitorious.org/qt/qt/source/9e8abb63ba4609887d988ee15ba6daee0b01380e:src/gui/dialogs/qfilesystemmodel.cpp

		return QSortFilterProxyModel::lessThan(left, right);
	}

};

/**
 * @brief RsCollectionDialog::RsCollectionDialog
 * @param collectionFileName: Filename of RSCollection saved
 * @param colFileInfos: Vector of ColFileInfo to be add in intialization
 * @param creation: Open dialog as RsColl Creation or RsColl DownLoad
 * @param readOnly: Open dialog for RsColl as ReadOnly
 */
RsCollectionDialog::RsCollectionDialog(const QString& collectionFileName
                                       , const std::vector<ColFileInfo>& colFileInfos
                                       , const bool& creation /* = false*/
                                       , const bool& readOnly)
  : _fileName(collectionFileName), _creationMode(creation) ,_readOnly(readOnly)
{
	ui.setupUi(this) ;

	uint32_t size = colFileInfos.size();
	for(uint32_t i=0;i<size;++i)
	{
		const ColFileInfo &colFileInfo = colFileInfos[i];
		_newColFileInfos.push_back(colFileInfo);
	}

	setWindowFlags(Qt::Window); // for maximize button
	setWindowFlags(windowFlags() & ~Qt::WindowMinimizeButtonHint);

	setWindowTitle(QString("%1 - %2").arg(windowTitle()).arg(QFileInfo(_fileName).completeBaseName()));
	
	
	ui.headerFrame->setHeaderImage(QPixmap(":/icons/collections.png"));

	if(creation)
	{
		ui.headerFrame->setHeaderText(tr("Collection Editor"));
		ui.downloadFolder_LE->hide();
		ui.downloadFolder_LB->hide();
		ui.destinationDir_TB->hide();
	}
	else
	{
		ui.headerFrame->setHeaderText(tr("Download files"));
		ui.downloadFolder_LE->show();
		ui.downloadFolder_LB->show();
		ui.label_filename->hide();
		ui._filename_TL->hide();

		ui.downloadFolder_LE->setText(QString::fromUtf8(rsFiles->getDownloadDirectory().c_str())) ;

		QObject::connect(ui.downloadFolder_LE,SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(openDestinationDirectoryMenu()));
		QObject::connect(ui.destinationDir_TB,SIGNAL(pressed()), this, SLOT(openDestinationDirectoryMenu()));
	}

	// 1 - add all elements to the list.

	ui._fileEntriesTW->setColumnCount(COLUMN_COUNT) ;

	QTreeWidgetItem *headerItem = ui._fileEntriesTW->headerItem();
	headerItem->setText(COLUMN_FILE, tr("File"));
	headerItem->setText(COLUMN_FILEPATH, tr("File Path"));
	headerItem->setText(COLUMN_SIZE, tr("Size"));
	headerItem->setText(COLUMN_HASH, tr("Hash"));
	headerItem->setText(COLUMN_FILEC, tr("File Count"));

	bool wrong_chars = !updateList();

	// 2 - connect necessary signals/slots

	connect(ui._changeFile, SIGNAL(clicked()), this, SLOT(changeFileName()));
	connect(ui._add_PB, SIGNAL(clicked()), this, SLOT(add()));
	connect(ui._addRecur_PB, SIGNAL(clicked()), this, SLOT(addRecursive()));
	connect(ui._remove_PB, SIGNAL(clicked()), this, SLOT(remove()));
	connect(ui._makeDir_PB, SIGNAL(clicked()), this, SLOT(makeDir()));
	connect(ui._removeDuplicate_CB, SIGNAL(clicked(bool)), this, SLOT(updateRemoveDuplicate(bool)));
	connect(ui._cancel_PB, SIGNAL(clicked()), this, SLOT(cancel()));
	connect(ui._save_PB, SIGNAL(clicked()), this, SLOT(save()));
	connect(ui._download_PB, SIGNAL(clicked()), this, SLOT(download()));
	connect(ui._hashBox, SIGNAL(fileHashingFinished(QList<HashedFile>)), this, SLOT(fileHashingFinished(QList<HashedFile>)));
	connect(ui._fileEntriesTW, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(itemChanged(QTreeWidgetItem*,int)));

	// 3 Initialize List
	_dirModel = new QFileSystemModel(this);
	_dirModel->setRootPath("/");
	_dirModel->setFilter(QDir::AllEntries | QDir::NoSymLinks | QDir::NoDotAndDotDot);
	_dirLoaded = false;
	connect(_dirModel, SIGNAL(directoryLoaded(QString)), this, SLOT(directoryLoaded(QString)));

	_tree_proxyModel = new FSMSortFilterProxyModel(this);
	_tree_proxyModel->setDynamicSortFilter(true);
	_tree_proxyModel->setSourceModel(_dirModel);
	_tree_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
	_tree_proxyModel->setSortRole(Qt::DisplayRole);

	ui._systemFileTW->setModel(_tree_proxyModel);
	//Selection Setup
	_selectionProxy = ui._systemFileTW->selectionModel();

	// 4 Restore Configuration
	// load settings
	processSettings(true);

	// 5 Activate button follow creationMode
	ui._changeFile->setVisible(_creationMode && !_readOnly);
	ui._makeDir_PB->setVisible(_creationMode && !_readOnly);
	ui._removeDuplicate_CB->setVisible(_creationMode && !_readOnly);
	ui._save_PB->setVisible(_creationMode && !_readOnly);
	ui._treeViewFrame->setVisible(_creationMode && !_readOnly);
	ui._download_PB->setVisible(!_creationMode && !_readOnly);

	ui._fileEntriesTW->installEventFilter(this);
	ui._systemFileTW->installEventFilter(this);

	// 6 Add HashBox
	setAcceptDrops(true);
	ui._hashBox->setDropWidget(this);
	ui._hashBox->setAutoHide(true);
	ui._hashBox->setDefaultTransferRequestFlags(RS_FILE_REQ_ANONYMOUS_ROUTING) ;

	if(wrong_chars)
		QMessageBox::warning(NULL,tr("Bad filenames have been cleaned"),tr("Some filenames or directory names contained forbidden characters.\nCharacters <b>\",|,/,\\,&lt;,&gt;,*,?</b> will be replaced by '_'.\n Concerned files are listed in red.")) ;
}

void RsCollectionDialog::openDestinationDirectoryMenu()
{
	QMenu contextMnu( this );

	// pop a menu with existing entries and also a custom entry
	// Now get the list of existing  directories.

	std::list< SharedDirInfo> dirs ;
	rsFiles->getSharedDirectories( dirs) ;

	for (std::list<SharedDirInfo>::const_iterator it(dirs.begin());it!=dirs.end();++it){
		// Check for existence of directory name
		QFile directory( QString::fromUtf8((*it).filename.c_str())) ;

		if (!directory.exists()) continue ;
		if (!(directory.permissions() & QFile::WriteOwner)) continue ;

		contextMnu.addAction(QString::fromUtf8((*it).filename.c_str()), this, SLOT(setDestinationDirectory()))->setData(QString::fromUtf8( (*it).filename.c_str() ) ) ;
	}

	contextMnu.addAction( QIcon(IMAGE_SEARCH),tr("Specify..."),this,SLOT(chooseDestinationDirectory()));

	contextMnu.exec(QCursor::pos()) ;
}

void RsCollectionDialog::setDestinationDirectory()
{
	QString dest_dir(qobject_cast<QAction*>(sender())->data().toString()) ;
	ui.downloadFolder_LE->setText(dest_dir) ;
}

void RsCollectionDialog::chooseDestinationDirectory()
{
	QString dest_dir = QFileDialog::getExistingDirectory(this,tr("Choose directory")) ;

	if(dest_dir.isNull())
		return ;

	ui.downloadFolder_LE->setText(dest_dir) ;
}
/**
 * @brief RsCollectionDialog::~RsCollectionDialog
 */
RsCollectionDialog::~RsCollectionDialog()
{
	// save settings
	processSettings(false);
}

/**
 * @brief RsCollectionDialog::eventFilter: Proccess event in object
 * @param obj: object where event occured
 * @param event: event occured
 * @return If we don't have to process event in parent.
 */
bool RsCollectionDialog::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == ui._fileEntriesTW) {
		if (event->type() == QEvent::KeyPress) {
			QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
			if (keyEvent && (keyEvent->key() == Qt::Key_Space)) {
				// Space pressed

				// get state of current item
				QTreeWidgetItem *item = ui._fileEntriesTW->currentItem();
				if (item) {
					Qt::CheckState checkState = (item->checkState(COLUMN_FILE) == Qt::Checked) ? Qt::Unchecked : Qt::Checked;

					// set state of all selected items
					QList<QTreeWidgetItem*> selectedItems = ui._fileEntriesTW->selectedItems();
					QList<QTreeWidgetItem*>::iterator it;
					for (it = selectedItems.begin(); it != selectedItems.end(); ++it) {
						if ((*it)->checkState(COLUMN_FILE) != checkState)
							(*it)->setCheckState(COLUMN_FILE, checkState);
					}
				}

				return true; // eat event
			}

			if (keyEvent && (keyEvent->key() == Qt::Key_Delete)) {
				// Delete pressed
				remove();
				return true; // eat event
			}

			if (keyEvent && (keyEvent->key() == Qt::Key_Plus)) {
				// Plus pressed
				makeDir();
				return true; // eat event
			}

		}
	}

	if (obj == ui._systemFileTW) {
		if (event->type() == QEvent::KeyPress) {
			QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
			if (keyEvent && ((keyEvent->key() == Qt::Key_Enter)
			                 || keyEvent->key() == Qt::Key_Return)) {
				// Enter pressed
				if (keyEvent->modifiers() == Qt::ShiftModifier)
					addRecursive();
				else if(keyEvent->modifiers() == Qt::NoModifier) {
					add();
				}

				return true; // eat event
			}
		}
	}

	// pass the event on to the parent class
	return QDialog::eventFilter(obj, event);
}

/**
 * @brief RsCollectionDialog::processSettings
 * @param bLoad: Load or Save dialog's settings
 */
void RsCollectionDialog::processSettings(bool bLoad)
{
	Settings->beginGroup("RsCollectionDialog");

	if (bLoad) {
		// load settings

		if(_creationMode && !_readOnly){
			// Load windows geometrie
			restoreGeometry(Settings->value("WindowGeometrie_CM").toByteArray());
			// Load splitters state
			ui._mainSplitter->restoreState(Settings->value("MainSplitterState_CM").toByteArray());
			ui._listSplitter->restoreState(Settings->value("ListSplitterState_CM").toByteArray());
			// Load system file header configuration
			ui._systemFileTW->header()->restoreState(Settings->value("SystemFileHeader_CM").toByteArray());
			// Load file entries header configuration
			ui._fileEntriesTW->header()->restoreState(Settings->value("FileEntriesHeader_CM").toByteArray());
		} else {
			// Load windows geometrie
			restoreGeometry(Settings->value("WindowGeometrie").toByteArray());
			// Load splitters state
			ui._mainSplitter->restoreState(Settings->value("MainSplitterState").toByteArray());
			ui._listSplitter->restoreState(Settings->value("ListSplitterState").toByteArray());
			// Load system file header configuration
			ui._systemFileTW->header()->restoreState(Settings->value("SystemFileHeader").toByteArray());
			// Load file entries header configuration
			ui._fileEntriesTW->header()->restoreState(Settings->value("FileEntriesHeader").toByteArray());
		}
	} else {
		if(_creationMode && !_readOnly){
			// Save windows geometrie
			Settings->setValue("WindowGeometrie_CM",saveGeometry());
			// Save splitters state
			Settings->setValue("MainSplitterState_CM", ui._mainSplitter->saveState());
			Settings->setValue("ListSplitterState_CM", ui._listSplitter->saveState());
			// Save system file header configuration
			Settings->setValue("SystemFileHeader_CM", ui._systemFileTW->header()->saveState());
			// Save file entries header configuration
			Settings->setValue("FileEntriesHeader_CM", ui._fileEntriesTW->header()->saveState());
		} else {
			// Save windows geometrie
			Settings->setValue("WindowGeometrie",saveGeometry());
			// Save splitter state
			Settings->setValue("MainSplitterState", ui._mainSplitter->saveState());
			Settings->setValue("ListSplitterState", ui._listSplitter->saveState());
			// Save system file header configuration
			Settings->setValue("SystemFileHeader", ui._systemFileTW->header()->saveState());
			// Save file entries header configuration
			Settings->setValue("FileEntriesHeader", ui._fileEntriesTW->header()->saveState());
		}
	}

	Settings->endGroup();
}

/**
 * @brief RsCollectionDialog::getRootItem: Create the root Item if not existing
 * @return: the root item
 */
QTreeWidgetItem* RsCollectionDialog::getRootItem()
{
	return ui._fileEntriesTW->invisibleRootItem();

// (csoler) I removed this code because it does the job of the invisibleRootItem() method.
//
//	QTreeWidgetItem* root= ui._fileEntriesTW->topLevelItem(0);
//	if (!root) {
//		root= new QTreeWidgetItem;
//		root->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsTristate);
//		root->setText(COLUMN_FILE, "/");
//		root->setToolTip(COLUMN_FILE,tr("This is the root directory."));
//		root->setText(COLUMN_FILEPATH, "/");
//		root->setText(COLUMN_HASH, "");
//		root->setData(COLUMN_HASH, ROLE_NAME, "");
//		root->setData(COLUMN_HASH, ROLE_PATH, "");
//		root->setData(COLUMN_HASH, ROLE_TYPE, DIR_TYPE_DIR);
//		root->setText(COLUMN_SIZE, misc::friendlyUnit(0));
//		root->setToolTip(COLUMN_SIZE, tr("Real Size: Waiting child..."));
//		root->setData(COLUMN_SIZE, ROLE_SIZE, 0);
//		root->setData(COLUMN_SIZE, ROLE_SELSIZE, 0);
//		root->setText(COLUMN_FILEC, "0");
//		root->setToolTip(COLUMN_FILEC, tr("Real File Count: Waiting child..."));
//		root->setData(COLUMN_FILEC, ROLE_FILEC, 0);
//		root->setData(COLUMN_FILEC, ROLE_SELFILEC, 0);
//		ui._fileEntriesTW->addTopLevelItem(root);
//	}
//	root->setExpanded(true);
//
//	return root;
}

/**
 * @brief RsCollectionDialog::updateList: Update list of item in RsCollection
 * @return If at least one item have a Wrong Char
 */
bool RsCollectionDialog::updateList()
{
	bool wrong_chars = false ;
	wrong_chars = addChild(getRootItem(), _newColFileInfos);

	_newColFileInfos.clear();

	ui._filename_TL->setText(_fileName) ;
	for (int column = 0; column < ui._fileEntriesTW->columnCount(); ++column) {
		ui._fileEntriesTW->resizeColumnToContents(column);
	}

	updateSizes() ;

	return !wrong_chars;
}

/**
 * @brief RsCollectionDialog::addChild: Add Child Item in list
 * @param parent: Parent Item
 * @param child: Child ColFileInfo item to add
 * @param total_size: Saved total size of children to save in parent
 * @param total_files: Saved total file of children to save in parent
 * @return If at least one item have a Wrong Char
 */
bool RsCollectionDialog::addChild(QTreeWidgetItem* parent, const std::vector<ColFileInfo>& child)
{
	bool wrong_chars = false ;

	uint32_t childCount = child.size();
	for(uint32_t i=0; i<childCount; ++i)
	{
		const ColFileInfo &colFileInfo = child[i];

		QList<QTreeWidgetItem*> founds;
		QList<QTreeWidgetItem*> parentsFounds;
		parentsFounds = ui._fileEntriesTW->findItems(colFileInfo.path , Qt::MatchExactly | Qt::MatchRecursive, COLUMN_FILEPATH);
		if (colFileInfo.type == DIR_TYPE_DIR){
			founds = ui._fileEntriesTW->findItems(colFileInfo.path + "/" +colFileInfo.name, Qt::MatchExactly | Qt::MatchRecursive, COLUMN_FILEPATH);
		} else {
			founds = ui._fileEntriesTW->findItems(colFileInfo.path + "/" +colFileInfo.name, Qt::MatchExactly | Qt::MatchRecursive, COLUMN_FILEPATH);
			if (ui._removeDuplicate_CB->isChecked()) {
				founds << ui._fileEntriesTW->findItems(colFileInfo.hash, Qt::MatchExactly | Qt::MatchRecursive, COLUMN_HASH);
			}
		}
		if (founds.empty()) {
			QTreeWidgetItem *item = new QTreeWidgetItem;

			//item->setFlags(Qt::ItemIsUserCheckable | item->flags());
			item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsTristate);
			item->setCheckState(COLUMN_FILE, Qt::Checked);
			item->setText(COLUMN_FILE, colFileInfo.name);
			item->setText(COLUMN_FILEPATH, colFileInfo.path + "/" + colFileInfo.name);
			item->setText(COLUMN_HASH, colFileInfo.hash);
			item->setData(COLUMN_HASH, ROLE_NAME, colFileInfo.name);
			item->setData(COLUMN_HASH, ROLE_PATH, colFileInfo.path);
			item->setData(COLUMN_HASH, ROLE_TYPE, colFileInfo.type);
			QFont font = item->font(COLUMN_FILE);
			if (colFileInfo.type==DIR_TYPE_DIR) {
				item->setToolTip(COLUMN_FILE,tr("This is a directory. Double-click to expand it."));
				font.setBold(true);
				//Size calculated after for added child after init
				item->setText(COLUMN_SIZE, misc::friendlyUnit(0));
				item->setToolTip(COLUMN_SIZE,tr("Real Size: Waiting child..."));
				item->setData(COLUMN_SIZE, ROLE_SIZE, 0);
				item->setData(COLUMN_SIZE, ROLE_SELSIZE, 0);

				item->setText(COLUMN_FILEC, "");
				item->setToolTip(COLUMN_FILEC, tr("Real File Count: Waiting child..."));
				item->setData(COLUMN_FILEC, ROLE_FILEC, 0);
				item->setData(COLUMN_FILEC, ROLE_SELFILEC, 0);
			} else {
				font.setBold(false);
				item->setText(COLUMN_SIZE, misc::friendlyUnit(colFileInfo.size));
				item->setToolTip(COLUMN_SIZE, tr("Real Size=%1").arg(misc::friendlyUnit(colFileInfo.size)));
				item->setData(COLUMN_SIZE, ROLE_SIZE, colFileInfo.size);
				item->setData(COLUMN_SIZE, ROLE_SELSIZE, colFileInfo.size);

				item->setText(COLUMN_FILEC, "1");
				item->setToolTip(COLUMN_FILEC, tr("Real File Count=%1").arg(1));
				item->setData(COLUMN_FILEC, ROLE_FILEC, 1);
				item->setData(COLUMN_FILEC, ROLE_SELFILEC, 1);
			}
			item->setFont(COLUMN_FILE, font);
			item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);

			if (colFileInfo.filename_has_wrong_characters)
			{
				wrong_chars = true ;
				item->setTextColor(COLUMN_FILE, QColor(255,80,120)) ;
			}

			if (parentsFounds.empty()) {
				parent->addChild(item);
			} else {
				parentsFounds.at(0)->addChild(item);
			}

			if (colFileInfo.type == DIR_TYPE_FILE) {
				//update parents size only for file children
				QTreeWidgetItem *itemParent = item->parent();
				if (itemParent) {
					while (itemParent) {
						qulonglong parentSize = itemParent->data(COLUMN_SIZE, ROLE_SIZE).toULongLong() + colFileInfo.size;
						itemParent->setData(COLUMN_SIZE, ROLE_SIZE, parentSize);
						itemParent->setToolTip(COLUMN_SIZE, tr("Real Size=%1").arg(misc::friendlyUnit(parentSize)));
						qulonglong parentSelSize = itemParent->data(COLUMN_SIZE, ROLE_SELSIZE).toULongLong() + colFileInfo.size;
						itemParent->setData(COLUMN_SIZE, ROLE_SELSIZE, parentSelSize);
						itemParent->setText(COLUMN_SIZE, misc::friendlyUnit(parentSelSize));

						qulonglong parentFileCount = itemParent->data(COLUMN_FILEC, ROLE_FILEC).toULongLong() + 1;
						itemParent->setData(COLUMN_FILEC, ROLE_FILEC, parentFileCount);
						itemParent->setToolTip(COLUMN_FILEC, tr("Real File Count=%1").arg(parentFileCount));
						qulonglong parentSelFileCount = itemParent->data(COLUMN_FILEC, ROLE_SELFILEC).toULongLong() + 1;
						itemParent->setData(COLUMN_FILEC, ROLE_SELFILEC, parentSelFileCount);
						itemParent->setText(COLUMN_FILEC, QString("%1").arg(parentSelFileCount));

						itemParent = itemParent->parent();
					}
				}
			}

			founds.push_back(item);
		}

		if (!founds.empty()) {

			if (colFileInfo.type == DIR_TYPE_DIR) {
				wrong_chars |= addChild(founds.at(0), colFileInfo.children);
			}
		}
	}
	return wrong_chars;
}

/**
 * @brief RsCollectionDialog::directoryLoaded: called when ui._treeView have load an directory as QFileSystemModel don't load all in one time
 * @param dirLoaded: Name of directory just loaded
 */
void RsCollectionDialog::directoryLoaded(QString dirLoaded)
{
    if(!_dirLoaded)
    {

        QFileInfo lastDir = Settings->getLastDir(RshareSettings::LASTDIR_EXTRAFILE);
        if (lastDir.absoluteFilePath() == "") return;
        if (lastDir.absoluteFilePath() == dirLoaded) _dirLoaded = true;

        // Check all parent directory to see if it is loaded
        //as QFileSystemModel don't load all
        do {
            if(lastDir.absolutePath()==dirLoaded) {
                //Only expand loaded directory
                QModelIndex lastDirIdx = _dirModel->index(lastDir.absoluteFilePath());
                //Select LASTDIR_EXTRAFILE or last parent found when loaded
                if (!lastDirIdx.isValid()){
                    _dirLoaded = true;
                    break;
                }

                //Ask dirModel to load next parent directory
                while (_dirModel->canFetchMore(lastDirIdx)) _dirModel->fetchMore(lastDirIdx);

                //Ask to Expand last loaded parent
                lastDirIdx = _tree_proxyModel->mapFromSource(lastDirIdx);
                if (lastDirIdx.isValid()){
                    ui._systemFileTW->expand(lastDirIdx);
                    ui._systemFileTW->setCurrentIndex(lastDirIdx);
                }
                break;
            }
            //std::cerr << "lastDir = " << lastDir.absoluteFilePath().toStdString() << std::endl;

            QDir c = lastDir.dir() ;
            if(!c.cdUp())
                break ;

            lastDir = QFileInfo(c.path());
        } while (true); //(lastDir.absoluteFilePath() != lastDir.absolutePath());
    }
}

/**
 * @brief RsCollectionDialog::updateSizes: Update Column size of ui._fileEntriesTW)
 */
void RsCollectionDialog::updateSizes()
{
	uint64_t total_size = 0 ;
	uint32_t total_count = 0 ;

	for(int i=0;i<ui._fileEntriesTW->topLevelItemCount();++i)
	{
		total_size  += ui._fileEntriesTW->topLevelItem(i)->data(COLUMN_SIZE ,ROLE_SELSIZE ).toULongLong();
		total_count += ui._fileEntriesTW->topLevelItem(i)->data(COLUMN_FILEC,ROLE_SELFILEC).toULongLong();
	}

	ui._selectedFiles_TL->setText(QString::number(total_count));
	ui._totalSize_TL->setText(misc::friendlyUnit(total_size));
}

/**
 * @brief purifyFileName: Check if name of file is good or not.
 * @param input
 * @param bad
 * @return
 */
static QString purifyFileName(const QString& input, bool& bad)
{
	static const QString bad_chars = "/\\\"*:?<>|" ;
	bad = false ;
	QString output = input ;

	for(int i=0;i<output.length();++i)
		for(int j=0;j<bad_chars.length();++j)
			if(output[i] == bad_chars[j])
			{
				output[i] = '_' ;
				bad = true ;
			}

	return output ;
}

/**
 * @brief RsCollectionDialog::changeFileName: Change the name of saved file
 */
void RsCollectionDialog::changeFileName()
{
	QString fileName;
	if(!misc::getSaveFileName(this, RshareSettings::LASTDIR_EXTRAFILE
														, QApplication::translate("RsCollectionFile", "Create collection file")
														, QApplication::translate("RsCollectionFile", "Collection files") + " (*." + RsCollection::ExtensionString + ")"
														, fileName,0, QFileDialog::DontConfirmOverwrite))
		return;

	if (!fileName.endsWith("." + RsCollection::ExtensionString))
		fileName += "." + RsCollection::ExtensionString ;

	std::cerr << "Got file name: " << fileName.toStdString() << std::endl;

	QFile file(fileName) ;

	if(file.exists())
	{
		RsCollection collFile;
		if (!collFile.checkFile(fileName,true)) return;

		QMessageBox mb;
		mb.setText(tr("Save Collection File."));
		mb.setInformativeText(tr("File already exists.")+"\n"+tr("What do you want to do?"));
		QAbstractButton *btnOwerWrite = mb.addButton(tr("Overwrite"), QMessageBox::YesRole);
		QAbstractButton *btnMerge = mb.addButton(tr("Merge"), QMessageBox::NoRole);
		QAbstractButton *btnCancel = mb.addButton(tr("Cancel"), QMessageBox::ResetRole);
		mb.setIcon(QMessageBox::Question);
		mb.exec();

		if (mb.clickedButton()==btnOwerWrite) {
			//Nothing to do
		} else if (mb.clickedButton()==btnMerge) {
			//Open old file to merge it with RsCollection
			QDomDocument qddOldFile("RsCollection");
			if (qddOldFile.setContent(&file)) {
				QDomElement docOldElem = qddOldFile.elementsByTagName("RsCollection").at(0).toElement();
				collFile.recursCollectColFileInfos(docOldElem,_newColFileInfos,QString(),false);
			}

		} else if (mb.clickedButton()==btnCancel) {
			return;
		} else {
			return;
		}

	} else {//if(file.exists())
		//create a new empty file to check if name if good.
		if (!file.open(QFile::WriteOnly)) return;
		file.remove();
	}

	_fileName = fileName;

	updateList();
}

/**
 * @brief RsCollectionDialog::add:  */
void RsCollectionDialog::add()
{
	addRecursive(false);
}

/**
 * @brief RsCollectionDialog::addRecursive:
 */
void RsCollectionDialog::addRecursive()
{
	addRecursive(true);
}

/**
 * @brief RsCollectionDialog::addRecursive: Add Selected item to RSCollection
 *   -Add File seperatly if parent folder not selected
 *   -Add File in folder if selected
 *   -Get root folder the selected one
 * @param recursive: If true, add all selected directory childrens
 */
void RsCollectionDialog::addRecursive(bool recursive)
{
	QStringList fileToHash;
	QMap<QString, QString > dirToAdd;
    int count=0;//to not scan all items on list .count()

	QModelIndexList milSelectionList =	ui._systemFileTW->selectionModel()->selectedIndexes();
	foreach (QModelIndex index, milSelectionList)
	{

		if (index.column()==0){//Get only FileName
			QString filePath = _dirModel->filePath(_tree_proxyModel->mapToSource(index));
			QFileInfo fileInfo = filePath;
			if (fileInfo.isDir()) {
				dirToAdd.insert(fileInfo.absoluteFilePath(),fileInfo.absolutePath());
				++count;
				if (recursive) {
					if (!addAllChild(fileInfo, dirToAdd, fileToHash, count)) return;
				} else {
					continue;
				}
			}
			if (fileInfo.isFile()){
				fileToHash.append(fileInfo.absoluteFilePath());
				++count;
				if (dirToAdd.contains(fileInfo.absolutePath()))
					_listOfFilesAddedInDir.insert(fileInfo.absoluteFilePath(),fileInfo.absolutePath());
				else
					_listOfFilesAddedInDir.insert(fileInfo.absoluteFilePath(),"");
			}
		}
	}

	// Process Dirs
	QTreeWidgetItem *item = NULL;
	if (!ui._fileEntriesTW->selectedItems().empty())
		item= ui._fileEntriesTW->selectedItems().at(0);
	if (item) {
		while (item->data(COLUMN_HASH, ROLE_TYPE).toUInt() != DIR_TYPE_DIR) {
			item = item->parent();//Only Dir as Parent
		}
	}

	int index = 0;
	while (index < dirToAdd.count())
	{
		ColFileInfo root;
		if (item && (item != getRootItem())) {
			root.name = "";
			root.path = item->text(COLUMN_FILEPATH);
		} else {
			root.name = "";
			root.path = "";
		}
		//QMap is ordered, so we get parent before child
		//Iterator is moved inside this function
		processItem(dirToAdd, index, root);
	}

	//Update liste before attach files to be sure when file is hashed, parent directory exists.
	updateList();

	for (QHash<QString,QString>::Iterator it = _listOfFilesAddedInDir.begin(); it != _listOfFilesAddedInDir.end() ; ++it)
	{
		QString path = it.value();
		//it.value() = "";//Don't reset value, could be an older attachment not terminated.
		if (dirToAdd.contains(path)){
			it.value() = dirToAdd.value(path);
		} else if(item) {
			if (item->data(COLUMN_HASH, ROLE_NAME) != "") {
				it.value() = item->text(COLUMN_FILEPATH);
			}
		}
	}

	// Process Files once all done
	ui._hashBox->addAttachments(fileToHash,RS_FILE_REQ_ANONYMOUS_ROUTING /*, 0*/);
}

/**
 * @brief RsCollectionDialog::addAllChild: Add children to RsCollection
 * @param fileInfoParent: Parent's QFileInfo to scan
 * @param dirToAdd: QMap where directories are added
 * @param fileToHash: QStringList where files are added
 * @return false if too many items is selected and aborted
 */
bool RsCollectionDialog::addAllChild(QFileInfo &fileInfoParent
                                     , QMap<QString, QString > &dirToAdd
                                     , QStringList &fileToHash
                                     , int &count)
{
	//Save count only first time to not scan all items on list .count()
	if (count == 0) count = (dirToAdd.count() + fileToHash.count());
	QDir dirParent = fileInfoParent.absoluteFilePath();
	dirParent.setFilter(QDir::AllEntries | QDir::NoSymLinks | QDir::NoDotAndDotDot);
	QFileInfoList childrenList = dirParent.entryInfoList();
	foreach (QFileInfo fileInfo, childrenList)
	{
		if (count == MAX_FILE_ADDED_BEFORE_ASK)
		{
			QMessageBox msgBox;
			msgBox.setText(tr("Warning, selection contains more than %1 items.").arg(MAX_FILE_ADDED_BEFORE_ASK));
			msgBox.setInformativeText("Do you want to continue?");
			msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
			msgBox.setDefaultButton(QMessageBox::No);
			int ret = msgBox.exec();
			switch (ret) {
				case QMessageBox::Yes:
				break;
				case QMessageBox::No:
					return false;
				break;
				break;
				default:
					// should never be reached
				break;
			}
		}
		if (fileInfo.isDir()) {
			dirToAdd.insert(fileInfo.absoluteFilePath(),fileInfo.absolutePath());
			++count;
			if (!addAllChild(fileInfo, dirToAdd, fileToHash, count)) return false;
		}
		if (fileInfo.isFile()){
			fileToHash.append(fileInfo.absoluteFilePath());
			++count;
			if (dirToAdd.contains(fileInfo.absolutePath()))
				_listOfFilesAddedInDir.insert(fileInfo.absoluteFilePath(),fileInfo.absolutePath());
			else
				_listOfFilesAddedInDir.insert(fileInfo.absoluteFilePath(),"");
		}
	}
	return true;
}

/**
 * @brief RsCollectionDialog::remove: Remove selected Items in RSCollection
 */
void RsCollectionDialog::remove()
{
	bool removeOnlyFile=false;
	QString listDir;
	// First, check if selection contains directories
	for (int curs = 0; curs < ui._fileEntriesTW->selectedItems().count(); ++curs)
	{// Have to call ui._fileEntriesTW->selectedItems().count() each time as selected could change
		QTreeWidgetItem *item = NULL;
		item= ui._fileEntriesTW->selectedItems().at(curs);

		//Uncheck child directory item if parent is checked
		if (item != getRootItem()){
			if (item->data(COLUMN_HASH, ROLE_TYPE).toUInt() == DIR_TYPE_DIR) {
				QString path = item->data(COLUMN_HASH, ROLE_PATH).toString();
				if (listDir.contains(path) && !path.isEmpty()) {
					item->setSelected(false);
				} else {
					listDir += item->data(COLUMN_HASH, ROLE_NAME).toString() +"<br>";
				}
			}
		}
	}

	//If directories, ask to remove them or not
	if (!listDir.isEmpty()){
		QMessageBox* msgBox = new QMessageBox(QMessageBox::Information, "", "");
		msgBox->setText("Warning, selection contains directories.");
		//msgBox->setInformativeText(); If text too long, no scroll, so I add an text edit
		QGridLayout* layout = qobject_cast<QGridLayout*>(msgBox->layout());
		if (layout) {
			int newRow = 1;
			for (int row = layout->count()-1; row >= 0; --row) {
				for (int col = layout->columnCount()-1; col >= 0; --col) {
					QLayoutItem *item = layout->itemAtPosition(row, col);
					if (item) {
						int index = layout->indexOf(item->widget());
						int r=0, c=0, rSpan=0, cSpan=0;
						layout->getItemPosition(index, &r, &c, &rSpan, &cSpan);
						if (r>0) {
							layout->removeItem(item);
							layout->addItem(item, r+3, c, rSpan, cSpan);
						} else if (rSpan>1) {
							newRow = rSpan + 1;
						}
					}
				}
			}
			QLabel *label = new QLabel(tr("Do you want to remove them and all their children, too?"));
			layout->addWidget(label,newRow, 0, 1, layout->columnCount(), Qt::AlignHCenter );
			QTextEdit *edit = new QTextEdit(listDir);
			edit->setReadOnly(true);
			edit->setWordWrapMode(QTextOption::NoWrap);
			layout->addWidget(edit,newRow+1, 0, 1, layout->columnCount(), Qt::AlignHCenter );
		}

		msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
		msgBox->setDefaultButton(QMessageBox::Yes);
		int ret = msgBox->exec();
		switch (ret) {
			case QMessageBox::Yes:
			break;
			case QMessageBox::No:
				removeOnlyFile = true;
			break;
			case QMessageBox::Cancel: {
				delete msgBox;
				return;
			}
			break;
			default:
				// should never be reached
			break;
		}
		delete msgBox;
	}

	//Remove wanted items
	int leftItem = 0;
	// Have to call ui._fileEntriesTW->selectedItems().count() each time as selected change
	while (ui._fileEntriesTW->selectedItems().count() > leftItem) {
		QTreeWidgetItem *item = ui._fileEntriesTW->selectedItems().at(leftItem);

		if (item != getRootItem()){
			if (!removeItem(item, removeOnlyFile)) {
				++leftItem;
			}
		} else {
			//Get Root change index
			++leftItem;
		}
	}

	updateSizes() ;

}

bool RsCollectionDialog::removeItem(QTreeWidgetItem *item, bool &removeOnlyFile)
{
	if (item){
		if ((item->data(COLUMN_HASH, ROLE_TYPE).toUInt() != DIR_TYPE_DIR) || !removeOnlyFile) {
			int leftItem = 0;
			while (item->childCount() > leftItem) {
				if (!removeItem(item->child(0), removeOnlyFile)) {
					++leftItem;
				}
			}
			if (leftItem == 0) {
				//First uncheck item to update parent informations
				item->setCheckState(COLUMN_FILE,Qt::Unchecked);
				QTreeWidgetItem *parent = item->parent();
				if (parent) {
					parent->removeChild(item);
				} else {
					getRootItem()->removeChild(item);
				}
				return true;
			} else {
				if (!removeOnlyFile) {
					std::cerr << "(EE) RsCollectionDialog::removeItem This could never happen." << std::endl;
				}
			}
		}
	}
	return false;
}

/** Process each item to make a new RsCollection item */
void RsCollectionDialog::processItem(QMap<QString, QString> &dirToAdd
                                     , int &index
                                     , ColFileInfo &parent
                                     )
{
	ColFileInfo newChild;
	int count = dirToAdd.count();
	if (index < count) {
		QString key=dirToAdd.keys().at(index);
		bool bad_chars_detected = false;
		QFileInfo fileInfo=key;
		QString cleanDirName = purifyFileName(fileInfo.fileName(),bad_chars_detected);
		newChild.name = cleanDirName;
		newChild.filename_has_wrong_characters = bad_chars_detected;
		newChild.size = fileInfo.isDir()? 0: fileInfo.size();
		newChild.type = fileInfo.isDir()? DIR_TYPE_DIR: DIR_TYPE_FILE ;
		if (parent.name != "") {
			newChild.path = parent.path + "/" + parent.name;
		} else {
			newChild.path = parent.path;
		}
		dirToAdd[key] = newChild.path + "/" + newChild.name;
		//Move to next item
		++index;
		if (index < count){
			QString newKey = dirToAdd.keys().at(index);
			while ((dirToAdd.value(newKey) == key)
			       && (index < count)) {
				processItem(dirToAdd, index, newChild);
				if (index < count)newKey = dirToAdd.keys().at(index);
			}
		}

		//Save parent when child are processed
		if (parent.name != "") {
			parent.children.push_back(newChild);
			parent.size += newChild.size;
		} else {
			_newColFileInfos.push_back(newChild);
		}
	}
}

/**
 * @brief RsCollectionDialog::addDir: Add new empty dir to list
 */
void RsCollectionDialog::makeDir()
{
	QString childName="";
	bool ok, badChar, nameOK = false;
	// Ask for name
	while (!nameOK)
	{
		childName = QInputDialog::getText(this, tr("New Directory")
		                                  , tr("Enter the new directory's name")
		                                  , QLineEdit::Normal, childName, &ok);
		if (ok && !childName.isEmpty())
		{
			childName = purifyFileName(childName, badChar);
			nameOK = !badChar;
			if (badChar)
			{
				QMessageBox msgBox;
				msgBox.setText("The name contains bad characters.");
				msgBox.setInformativeText("Do you want to use the corrected one?\n" + childName);
				msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Retry | QMessageBox::Cancel);
				msgBox.setDefaultButton(QMessageBox::Ok);
				int ret = msgBox.exec();
				switch (ret) {
					case QMessageBox::Ok:
						nameOK = true;
					break;
					case QMessageBox::Retry:
					break;
					case QMessageBox::Cancel:
						return;
					break;
					default:
						// should never be reached
					break;
				}
			}
		} else {//if (ok && !childName.isEmpty())
			return;
		}

	}


	QList<QTreeWidgetItem*> selected = ui._fileEntriesTW->selectedItems();

	if(selected.empty())
	{
		QTreeWidgetItem *item = getRootItem();

		ColFileInfo newChild;
		newChild.name = childName;
		newChild.filename_has_wrong_characters = false;
		newChild.size = 0;
		newChild.type = DIR_TYPE_DIR;
		newChild.path = item->data(COLUMN_HASH, ROLE_PATH).toString()
		        + "/" + item->data(COLUMN_HASH, ROLE_NAME).toString();
		if (item == getRootItem()) newChild.path = "";

		_newColFileInfos.push_back(newChild);
	}
	else
		for(auto it(selected.begin());it!=selected.end();++it)
		{
			QTreeWidgetItem *item = *it;

			while(item->data(COLUMN_HASH, ROLE_TYPE).toUInt() != DIR_TYPE_DIR)
				item = item->parent();//Only Dir as Parent

			ColFileInfo newChild;
			newChild.name = childName;
			newChild.filename_has_wrong_characters = false;
			newChild.size = 0;
			newChild.type = DIR_TYPE_DIR;

			if (item == getRootItem())
				newChild.path = "";
			else
				newChild.path = item->data(COLUMN_HASH, ROLE_PATH).toString() + "/" + item->data(COLUMN_HASH, ROLE_NAME).toString();

			_newColFileInfos.push_back(newChild);
		}


//	// Process all selected items
//	int count = ui._fileEntriesTW->selectedItems().count();
//	int curs = 0;
//	if (count == 0) curs = -1;
//
//	for (; curs < count; ++curs)
//	{
//		QTreeWidgetItem *item = NULL;
//		if (curs >= 0) {
//			item= ui._fileEntriesTW->selectedItems().at(curs);
//		} else {
//			item = getRootItem();
//		}
//		if (item) {
//			while (item->parent() != NULL && item->data(COLUMN_HASH, ROLE_TYPE).toUInt() != DIR_TYPE_DIR) {
//				item = item->parent();//Only Dir as Parent
//			}
//			ColFileInfo newChild;
//			newChild.name = childName;
//			newChild.filename_has_wrong_characters = false;
//			newChild.size = 0;
//			newChild.type = DIR_TYPE_DIR;
//			newChild.path = item->data(COLUMN_HASH, ROLE_PATH).toString()
//			                + "/" + item->data(COLUMN_HASH, ROLE_NAME).toString();
//			if (item == getRootItem()) newChild.path = "";
//
//			_newColFileInfos.push_back(newChild);
//		}
//	}


	updateList();
}

/**
 * @brief RsCollectionDialog::fileHashingFinished: Connected to ui._hashBox.fileHashingFinished
 *  Add finished File to collection in respective directory
 * @param hashedFiles: List of the file finished
 */
void RsCollectionDialog::fileHashingFinished(QList<HashedFile> hashedFiles)
{
	std::cerr << "RsCollectionDialog::fileHashingFinished() started." << std::endl;

	QString message;

	QList<HashedFile>::iterator it;
	for (it = hashedFiles.begin(); it != hashedFiles.end(); ++it) {
		HashedFile& hashedFile = *it;

		ColFileInfo colFileInfo;
		colFileInfo.name=hashedFile.filename;
		colFileInfo.path="";
		colFileInfo.size=hashedFile.size;
		colFileInfo.hash=QString::fromStdString(hashedFile.hash.toStdString());
		colFileInfo.filename_has_wrong_characters=false;
		colFileInfo.type=DIR_TYPE_FILE;

		if(_listOfFilesAddedInDir.value(hashedFile.filepath,"")!="") {
			//File Added in directory, find its parent
			colFileInfo.path = _listOfFilesAddedInDir.value(hashedFile.filepath,"");
			_listOfFilesAddedInDir.remove(hashedFile.filepath);
		}

		_newColFileInfos.push_back(colFileInfo);

	}

	std::cerr << "RsCollectionDialog::fileHashingFinished message : " << message.toStdString() << std::endl;

	updateList();
}

void RsCollectionDialog::itemChanged(QTreeWidgetItem *item, int col)
{
	if (col != COLUMN_FILE) return;

	if (item->data(COLUMN_HASH, ROLE_TYPE).toUInt() != DIR_TYPE_FILE) return;

	//In COLUMN_FILE, normaly, only checkState could change...
	qulonglong size = item->data(COLUMN_SIZE, ROLE_SIZE).toULongLong();
	bool unchecked = (item->checkState(COLUMN_FILE) == Qt::Unchecked);
	item->setData(COLUMN_SIZE, ROLE_SELSIZE, unchecked?0:size);
	item->setText(COLUMN_SIZE, misc::friendlyUnit(unchecked?0:size));
	item->setData(COLUMN_FILEC, ROLE_SELFILEC, unchecked?0:1);
	item->setText(COLUMN_FILEC, QString("%1").arg(unchecked?0:1));

	//update parents size
	QTreeWidgetItem *itemParent = item->parent();
	while (itemParent) {
		//When unchecked only remove selected size
		qulonglong parentSize = itemParent->data(COLUMN_SIZE, ROLE_SELSIZE).toULongLong() + (unchecked?0-size:size);
		itemParent->setData(COLUMN_SIZE, ROLE_SELSIZE, parentSize);
		itemParent->setText(COLUMN_SIZE, misc::friendlyUnit(parentSize));

		qulonglong parentFileCount = itemParent->data(COLUMN_FILEC, ROLE_SELFILEC).toULongLong() + (unchecked?0-1:1);
		itemParent->setData(COLUMN_FILEC, ROLE_SELFILEC, parentFileCount);
		itemParent->setText(COLUMN_FILEC, QString("%1").arg(parentFileCount));

		itemParent = itemParent->parent();
	}

	updateSizes() ;

}

/**
 * @brief RsCollectionDialog::updateRemoveDuplicate Remove all duplicate file when checked.
 * @param checked
 */
void RsCollectionDialog::updateRemoveDuplicate(bool checked)
{
	if (checked) {
		bool bRemoveAll = false;
		QTreeWidgetItemIterator it(ui._fileEntriesTW);
		QTreeWidgetItem *item;
		while ((item = *it) != NULL) {
			++it;
			if (item->data(COLUMN_HASH, ROLE_TYPE).toUInt() != DIR_TYPE_DIR) {
				QList<QTreeWidgetItem*> founds;
				founds << ui._fileEntriesTW->findItems(item->text(COLUMN_HASH), Qt::MatchExactly | Qt::MatchRecursive, COLUMN_HASH);
				if (founds.count() > 1) {
					bool bRemove = false;
					if (!bRemoveAll) {
						QMessageBox* msgBox = new QMessageBox(QMessageBox::Information, "", "");
						msgBox->setText("Warning, duplicate file found.");
						//msgBox->setInformativeText(); If text too long, no scroll, so I add an text edit
						msgBox->setStandardButtons(QMessageBox::YesToAll | QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
						msgBox->setDefaultButton(QMessageBox::Yes);

						QGridLayout* layout = qobject_cast<QGridLayout*>(msgBox->layout());
						if (layout) {
							int newRow = 1;
							for (int row = layout->count()-1; row >= 0; --row) {
								for (int col = layout->columnCount()-1; col >= 0; --col) {
									QLayoutItem *item = layout->itemAtPosition(row, col);
									if (item) {
										int index = layout->indexOf(item->widget());
										int r=0, c=0, rSpan=0, cSpan=0;
										layout->getItemPosition(index, &r, &c, &rSpan, &cSpan);
										if (r>0) {
											layout->removeItem(item);
											layout->addItem(item, r+3, c, rSpan, cSpan);
										} else if (rSpan>1) {
											newRow = rSpan + 1;
										}
									}
								}
							}
							QLabel *label = new QLabel(tr("Do you want to remove this file from the list?"));
							layout->addWidget(label,newRow, 0, 1, layout->columnCount(), Qt::AlignHCenter );
							QTextEdit *edit = new QTextEdit(item->text(COLUMN_FILEPATH));
							edit->setReadOnly(true);
							edit->setWordWrapMode(QTextOption::NoWrap);
							layout->addWidget(edit,newRow+1, 0, 1, layout->columnCount(), Qt::AlignHCenter );
						}

						int ret = msgBox->exec();
						switch (ret) {
							case QMessageBox::YesToAll: {
								bRemoveAll = true;
							}
							break;
							case QMessageBox::Yes: {
								bRemove = true;
							}
							break;
							case QMessageBox::No:
							break;
							case QMessageBox::Cancel: {
								delete msgBox;
								ui._removeDuplicate_CB->setChecked(false);
								return;
							}
							break;
							default:
								// should never be reached
							break;
						}
						delete msgBox;
					}

					if (bRemove || bRemoveAll) {
							//First uncheck item to update parent informations
							item->setCheckState(COLUMN_FILE,Qt::Unchecked);
							item->parent()->removeChild(item);
					}

				}
			}
		}
	}
}

/**
 * @brief RsCollectionDialog::cancel: Cancel RScollection editing or donwloading
 */
void RsCollectionDialog::cancel()
{
	std::cerr << "Canceling!" << std::endl;
	close() ;
}

/**
 * @brief RsCollectionDialog::download: Start downloading checked items
 */
void RsCollectionDialog::download()
{
	std::cerr << "Downloading!" << std::endl;

	QString dldir = ui.downloadFolder_LE->text();

	std::cerr << "downloading all these files:" << std::endl;

	QTreeWidgetItemIterator itemIterator(ui._fileEntriesTW);
	QTreeWidgetItem *item;
	while ((item = *itemIterator) != NULL) {
		++itemIterator;

		if (item->checkState(COLUMN_FILE) == Qt::Checked) {
			std::cerr << item->data(COLUMN_HASH,ROLE_NAME).toString().toStdString()
			          << " " << item->text(COLUMN_HASH).toStdString()
			          << " " << item->text(COLUMN_SIZE).toStdString()
			          << " " << item->data(COLUMN_HASH,ROLE_PATH).toString().toStdString() << std::endl;
			ColFileInfo colFileInfo;
			colFileInfo.hash = item->text(COLUMN_HASH);
			colFileInfo.name = item->data(COLUMN_HASH,ROLE_NAME).toString();
			colFileInfo.path = item->data(COLUMN_HASH,ROLE_PATH).toString();
			colFileInfo.type = item->data(COLUMN_HASH,ROLE_TYPE).toUInt();
			colFileInfo.size = item->data(COLUMN_SIZE,ROLE_SELSIZE).toULongLong();

			QString cleanPath = dldir + colFileInfo.path ;
			std::cerr << "making directory " << cleanPath.toStdString() << std::endl;

			if(!QDir(QApplication::applicationDirPath()).mkpath(cleanPath))
				QMessageBox::warning(NULL,QObject::tr("Unable to make path"),QObject::tr("Unable to make path:")+"<br>  "+cleanPath) ;

			if (colFileInfo.type==DIR_TYPE_FILE)
				rsFiles->FileRequest(colFileInfo.name.toUtf8().constData(),
				                     RsFileHash(colFileInfo.hash.toStdString()),
				                     colFileInfo.size,
				                     cleanPath.toUtf8().constData(),
				                     RS_FILE_REQ_ANONYMOUS_ROUTING,
				                     std::list<RsPeerId>());
		} else {//if (item->checkState(COLUMN_FILE) == Qt::Checked)
			std::cerr<<"Skipping file : " << item->data(COLUMN_HASH,ROLE_NAME).toString().toStdString() << std::endl;
		}
	}

	close();
}

/**
 * @brief RsCollectionDialog::save: Update collection to save it in caller.
 */
void RsCollectionDialog::save()
{
	std::cerr << "Saving!" << std::endl;
	_newColFileInfos.clear();
	QTreeWidgetItem* root = getRootItem();
	if (root) {
		saveChild(root);

		emit saveColl(_newColFileInfos, _fileName);
	}
	close();
}

/**
 * @brief RsCollectionDialog::saveChild: Save each child in _newColFileInfos
 * @param parent
 */
void RsCollectionDialog::saveChild(QTreeWidgetItem *parentItem, ColFileInfo *parentInfo)
{
	ColFileInfo parent;
	if (!parentInfo) parentInfo = &parent;

	parentInfo->checked = (parentItem->checkState(COLUMN_FILE)==Qt::Checked);
	parentInfo->hash = parentItem->text(COLUMN_HASH);
	parentInfo->name = parentItem->data(COLUMN_HASH,ROLE_NAME).toString();
	parentInfo->path = parentItem->data(COLUMN_HASH,ROLE_PATH).toString();
	parentInfo->type = parentItem->data(COLUMN_HASH,ROLE_TYPE).toUInt();
	parentInfo->size = parentItem->data(COLUMN_SIZE,ROLE_SELSIZE).toULongLong();

	for (int i=0; i<parentItem->childCount(); ++i) {
		ColFileInfo child;
		saveChild(parentItem->child(i), &child);
		if (parentInfo->name != "") {
			parentInfo->children.push_back(child);
		} else {
			_newColFileInfos.push_back(child);
		}
	}
}
