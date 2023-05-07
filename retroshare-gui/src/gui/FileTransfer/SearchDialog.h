/*******************************************************************************
 * retroshare-gui/src/gui/FileTransfer/SearchDialog.h                          *
 *                                                                             *
 * Copyright (c) 2006, Crypton       <retroshare.project@gmail.com>            *
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

#ifndef _SEARCHDIALOG_H
#define _SEARCHDIALOG_H

#include "retroshare/rstypes.h"
#include "retroshare/rsevents.h"
#include "ui_SearchDialog.h"
#include "retroshare-gui/mainpage.h"

class AdvancedSearchDialog;
class RSTreeWidgetItemCompareRole;

namespace RsRegularExpression { class Expression; }

#define FRIEND_SEARCH 1
#define ANONYMOUS_SEARCH 2
class SearchDialog : public MainPage
{
    Q_OBJECT

    Q_PROPERTY(QColor textColorLocal READ textColorLocal WRITE setTextColorLocal)
	Q_PROPERTY(QColor textColorDownloading READ textColorDownloading WRITE setTextColorDownloading)
    Q_PROPERTY(QColor textColorNoSources READ textColorNoSources WRITE setTextColorNoSources)
    Q_PROPERTY(QColor textColorLowSources READ textColorLowSources WRITE setTextColorLowSources)
    Q_PROPERTY(QColor textColorHighSources READ textColorHighSources WRITE setTextColorHighSources)

public:
/** Default Constructor */
    SearchDialog(QWidget *parent = 0);
/** Default Destructor */
    ~SearchDialog();

    void searchKeywords(const QString& keywords);

    QColor textColorLocal() const { return mTextColorLocal; }
	QColor textColorDownloading() const { return mTextColorDownloading; }
    QColor textColorNoSources() const { return mTextColorNoSources; }
    QColor textColorLowSources() const { return mTextColorLowSources; }
    QColor textColorHighSources() const { return mTextColorHighSources; }

    void setTextColorLocal(QColor color) { mTextColorLocal = color; }
	void setTextColorDownloading(QColor color) { mTextColorDownloading = color; }
    void setTextColorNoSources(QColor color) { mTextColorNoSources = color; }
    void setTextColorLowSources(QColor color) { mTextColorLowSources = color; }
    void setTextColorHighSources(QColor color) { mTextColorHighSources = color; }

public slots:
		void updateFiles(qulonglong request_id,FileDetail file) ;

private slots:

/** Create the context popup menu and it's submenus */
    void searchResultWidgetCustomPopupMenu( QPoint point );

	void processResultQueue();
    void searchSummaryWidgetCustomPopupMenu( QPoint point );

    void download();
    void ban();

    void collCreate();
    void collModif();
    void collView();
    void collOpen();

    void broadcastonchannel();

    void recommendtofriends();
    void checkText(const QString&);

    void openBannedFiles();
    void copyResultLink();
    void copySearchLink();
    void openFolderSearch();

    void searchAgain();
    void searchRemove();
    void searchRemoveAll();
    void searchKeywords();

/** management of the adv search dialog object when switching search modes */
    void toggleAdvancedSearchDialog(bool);
    void hideEvent(QHideEvent * event);

/** raises (and if necessary instantiates) the advanced search dialog */
    void showAdvSearchDialog(bool=true);

/** perform the advanced search */
    void advancedSearch(RsRegularExpression::Expression*);

    void selectSearchResults(int index = -1);
    void hideOrShowSearchResult(QTreeWidgetItem* resultItem, QString currentSearchId = QString(), int fileTypeIndex = -1);

    void sendLinkTo();
    
    void selectFileType(int index);

	void filterItems();

private:
/** render the results to the tree widget display */
    void initSearchResult(const QString& txt,qulonglong searchId, int fileType, bool advanced) ;
    void resultsToTree(const QString& txt,qulonglong searchId, const std::list<DirDetails>&);
    void insertFile(qulonglong searchId,const FileDetail &file, int searchType = ANONYMOUS_SEARCH) ;
    void insertDirectory(const QString &txt, qulonglong searchId, const DirDetails &dir, QTreeWidgetItem *item);
    void insertDirectory(const QString &txt, qulonglong searchId, const DirDetails &dir);
    void setIconAndType(QTreeWidgetItem *item, const QString& filename);
    void downloadDirectory(const QTreeWidgetItem *item, const QString &base);
    void getSourceFriendsForHash(const RsFileHash &hash,std::list<RsPeerId> &srcIds);
    void handleEvent_main_thread(std::shared_ptr<const RsEvent> event);

/** the advanced search dialog instance */
    AdvancedSearchDialog * advSearchDialog;

/** Contains the mapping of filetype combobox to filetype extensions */
    static const int FILETYPE_IDX_ANY;
    static const int FILETYPE_IDX_ARCHIVE;
    static const int FILETYPE_IDX_AUDIO;
    static const int FILETYPE_IDX_CDIMAGE;
    static const int FILETYPE_IDX_DOCUMENT;
    static const int FILETYPE_IDX_PICTURE;
    static const int FILETYPE_IDX_PROGRAM;
    static const int FILETYPE_IDX_VIDEO;
    static const int FILETYPE_IDX_DIRECTORY;


    static QMap<int, QString> * FileTypeExtensionMap;
    static bool initialised;
    void initialiseFileTypeMappings();

	void processSettings(bool bLoad);

	bool filterItem(QTreeWidgetItem *item, const QString &text, int filterColumn);

    bool m_bProcessSettings;

    int nextSearchId;

    RSTreeWidgetItemCompareRole *compareSummaryRole;
    RSTreeWidgetItemCompareRole *compareResultRole;

    QAbstractItemDelegate *mAgeColumnDelegate;
    QAbstractItemDelegate *mSizeColumnDelegate;

	/* Color definitions (for standard see default.qss) */
	QColor mTextColorLocal;
	QColor mTextColorDownloading;
	QColor mTextColorNoSources;
	QColor mTextColorLowSources;
	QColor mTextColorHighSources;

	QAction *collCreateAct;
	QAction *collModifAct;
	QAction *collViewAct;
	QAction *collOpenAct;

/** Qt Designer generated object */
    Ui::SearchDialog ui;

	 bool _queueIsAlreadyTakenCareOf ;
	 std::vector<std::pair<qulonglong,FileDetail> > searchResultsQueue ;

     RsEventsHandlerId_t mEventHandlerId ;
};

#endif

