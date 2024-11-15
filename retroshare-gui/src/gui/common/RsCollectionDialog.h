/*******************************************************************************
 * gui/common/RsCollectionDialog.h                                             *
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

#include <set>
#include "ui_RsCollectionDialog.h"
#include "RsCollection.h"
#include "RsCollectionModel.h"
#include <QFileSystemModel>
#include <QSortFilterProxyModel>

class QCheckBox ;

class RsCollectionDialog: public QDialog
{
	Q_OBJECT

public:
	virtual ~RsCollectionDialog();

    // Open new collection
    static bool openNewCollection(const RsFileTree &tree = RsFileTree());

    // Edit existing collection
    static bool editExistingCollection(const QString& fileName, bool showError = true);

    // Open existing collection for download
    static bool openExistingCollection(const QString& fileName, bool showError = true);

    // Open existing collection for download
    static bool downloadFiles(const RsCollection& collection);
protected:
    static QString errorString(RsCollection::RsCollectionErrorCode code);

    void init(const QString& collectionFileName);

    enum RsCollectionDialogMode {
        UNKNOWN       = 0x00,
        EDIT          = 0x01,
        DOWNLOAD      = 0x02,
    };

    RsCollectionDialog(const QString& filename, RsCollectionDialogMode mode) ;
    RsCollectionDialog(const RsCollection& coll, RsCollectionDialogMode mode) ;

private slots:
	void directoryLoaded(QString dirLoaded);
	void updateSizes() ;
	void changeFileName() ;
    void addSelection() ;
    void addSelectionRecursive() ;
	void remove() ;
	void chooseDestinationDirectory();
	void setDestinationDirectory();
	void openDestinationDirectoryMenu();
	void makeDir() ;
	void fileHashingFinished(QList<HashedFile> hashedFiles) ;
	void cancel() ;
	void download() ;
	void save() ;

signals:
	void saveColl(std::vector<ColFileInfo>, QString);

private:
	void processSettings(bool bLoad) ;
    void addSelection(bool recursive) ;

	Ui::RsCollectionDialog ui;

    RsCollectionDialogMode _mode;

	QFileSystemModel *_dirModel;
	QSortFilterProxyModel *_tree_proxyModel;
	QItemSelectionModel *_selectionProxy;
	bool _dirLoaded;
	QHash<QString,QString> _listOfFilesAddedInDir;

    RsCollectionModel *mCollectionModel;
    RsCollection *mCollection;

    std::map<QString,RsFileHash> mFilesBeingHashed; // map of file path vs. temporary ID used for the file while hashing
};
