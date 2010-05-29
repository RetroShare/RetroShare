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

#ifndef _SEARCHDIALOG_H
#define _SEARCHDIALOG_H

#include <QFileDialog>

#include <QMessageBox>
#include <QModelIndex>

#include <rsiface/rsfiles.h>
#include "mainpage.h"
#include "ui_SearchDialog.h"
#include "advsearch/advancedsearchdialog.h"

#define FRIEND_SEARCH 1
#define ANONYMOUS_SEARCH 2
class SearchDialog : public MainPage
{
    Q_OBJECT

        public:
/** Default Constructor */
    SearchDialog(QWidget *parent = 0);
/** Default Destructor */


public slots:
		void updateFiles(qulonglong request_id,FileDetail file) ;

private slots:

/** Create the context popup menu and it's submenus */
    void searchtableWidgetCostumPopupMenu( QPoint point );

    void searchtableWidget2CostumPopupMenu( QPoint point );

    void download();

    void broadcastonchannel();

    void recommendtofriends();
	 void checkText(const QString&) ;
    
    void copysearchLink();


    void searchRemove();

    void searchRemoveAll();

    void searchKeywords();

/** management of the adv search dialog object when switching search modes */
    void toggleAdvancedSearchDialog(bool);
    void hideEvent(QHideEvent * event);

/** raises (and if necessary instantiates) the advanced search dialog */
    void showAdvSearchDialog(bool=true);

/** perform the advanced search */
    void advancedSearch(Expression*);

    void selectSearchResults(int index = -1);

    void clearKeyword();
    
    void sendLinkTo();
    
    void togglereset();

    void onComboIndexChanged(int index);



private:
/** render the results to the tree widget display */
	 void initSearchResult(const std::string& txt,qulonglong searchId) ;
   void resultsToTree(std::string,qulonglong searchId, const std::list<DirDetails>&);
	 void insertFile(const std::string& txt,qulonglong searchId, const FileDetail& file, int searchType = ANONYMOUS_SEARCH) ;
	 void insertDirectory(const std::string &txt, qulonglong searchId, const DirDetails &dir, QTreeWidgetItem *item);
         void insertDirectory(const std::string &txt, qulonglong searchId, const DirDetails &dir);
	 void setIconAndType(QTreeWidgetItem *item, QString &ext);
	 void downloadDirectory(const QTreeWidgetItem *item, const QString &base);
	 void getSourceFriendsForHash(const std::string& hash,std::list<std::string>& srcIds);



/** the advanced search dialog instance */
    AdvancedSearchDialog * advSearchDialog;

/** Defines the actions for the context menu */
    QAction* downloadAct;
    QAction* copysearchlinkAct;
    QAction* sendrslinkAct;
    QAction* broadcastonchannelAct;
    QAction* recommendtofriendsAct;

    QAction* searchRemoveAct;
    QAction* searchRemoveAllAct;

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

/****
QTreeWidget *searchtableWidget;
QTreeWidget *searchtablewidget2;
****/

    int nextSearchId;

/** Qt Designer generated object */
    Ui::SearchDialog ui;
};

#endif

