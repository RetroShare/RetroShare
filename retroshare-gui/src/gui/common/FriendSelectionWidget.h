/*******************************************************************************
 * gui/common/FriendSelectionWidget.h                                          *
 *                                                                             *
 * Copyright (C) 2012, Retroshare Team <retroshare.project@gmail.com>          *
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

#ifndef FRIENDSELECTIONWIDGET_H
#define FRIENDSELECTIONWIDGET_H

#include <QWidget>
#include <QDialog>

#include "retroshare/rsevents.h"
#include "retroshare/rsstatus.h"
#include <gui/gxs/RsGxsUpdateBroadcastPage.h>
#include "util/FontSizeHandler.h"

namespace Ui {
class FriendSelectionWidget;
}

class QTreeWidgetItem;
class RSTreeWidgetItemCompareRole;

class FriendSelectionWidget : public QWidget
{
	Q_OBJECT

	Q_PROPERTY(QColor textColorOnline READ textColorOnline WRITE setTextColorOnline)

public:
	enum IdType
	{
		IDTYPE_NONE,
		IDTYPE_GROUP,
		IDTYPE_SSL,
		IDTYPE_GPG,
		IDTYPE_GXS
	};

	enum Modus
	{
		MODUS_SINGLE,
		MODUS_MULTI,
		MODUS_CHECK
	};

    enum ShowType {
        SHOW_NONE             = 0,
        SHOW_GROUP            = 1,
        SHOW_GPG              = 2,
        SHOW_SSL              = 4,
        SHOW_NON_FRIEND_GPG   = 8,
        SHOW_GXS              =16,
        SHOW_CONTACTS         =32
    };

    Q_DECLARE_FLAGS(ShowTypes, ShowType)

public:
	explicit FriendSelectionWidget(QWidget *parent = 0);
	~FriendSelectionWidget();

	void setHeaderText(const QString &text);
	void setModus(Modus modus);
	void setShowType(ShowTypes types);
	int addColumn(const QString &title);
	void start();

	bool isSortByState();
	bool isFilterConnected();

	void loadIdentities();
	int selectedItemCount();
    std::string selectedId(IdType &idType);

    void setSelectedIdsFromString(IdType type,const std::set<std::string>& ids,bool add);

    template<class ID_CLASS,FriendSelectionWidget::IdType TYPE> void selectedIds(std::set<ID_CLASS>& ids, bool onlyDirectSelected)
    {
        std::set<std::string> tmpids ;
        selectedIds_internal(TYPE, tmpids, onlyDirectSelected);
        ids.clear() ;
        for(std::set<std::string>::const_iterator it(tmpids.begin());it!=tmpids.end();++it)
            ids.insert(ID_CLASS(*it)) ;
    }
    template<class ID_CLASS,FriendSelectionWidget::IdType TYPE> void setSelectedIds(const std::set<ID_CLASS>& ids, bool add)
    {
        std::set<std::string> tmpids ;
        for(typename std::set<ID_CLASS>::const_iterator it(ids.begin());it!=ids.end();++it)
            tmpids.insert((*it).toStdString()) ;
        setSelectedIds_internal(TYPE, tmpids, add);
    }

	void itemsFromId(IdType idType, const std::string &id, QList<QTreeWidgetItem*> &items);
	void items(QList<QTreeWidgetItem*> &items, IdType = IDTYPE_NONE);

	IdType idTypeFromItem(QTreeWidgetItem *item);
	std::string idFromItem(QTreeWidgetItem *item);

	QColor textColorOnline() const { return mTextColorOnline; }

	void setTextColorOnline(QColor color) { mTextColorOnline = color; }

	// Add QAction to context menu (action won't be deleted)
	void addContextMenuAction(QAction *action);

protected:
	void showEvent(QShowEvent *e) override;
	void changeEvent(QEvent *e);

	virtual void updateDisplay(bool complete);

signals:
	void itemAdded(int idType, const QString &id, QTreeWidgetItem *item);
	void contentChanged();
	void doubleClicked(int idType, const QString &id);
	void itemChanged(int idType, const QString &id, QTreeWidgetItem *item, int column);
	void itemSelectionChanged();

public slots:
	void sortByState(bool sort);
    void sortByChecked(bool sort);
    void filterConnected(bool filter);

private slots:
    void peerStatusChanged(const RsPeerId &peerid, RsStatusValue status);
	void filterItems(const QString &text);
	void contextMenuRequested(const QPoint &pos);
	void itemDoubleClicked(QTreeWidgetItem *item, int column);
	void itemChanged(QTreeWidgetItem *item, int column);
	void selectAll() ;
	void deselectAll() ;

private:
    void groupsChanged();
    void fillList();
	void secured_fillList();

    void selectedIds_internal(IdType idType, std::set<std::string> &ids, bool onlyDirectSelected);
    void setSelectedIds_internal(IdType idType, const std::set<std::string> &ids, bool add);

private:
    void handleEvent_main_thread(std::shared_ptr<const RsEvent> event);

	bool mStarted;
	RSTreeWidgetItemCompareRole *mCompareRole;
	Modus mListModus;
	ShowTypes mShowTypes;
	bool mInGroupItemChanged;
	bool mInGpgItemChanged;
	bool mInSslItemChanged;
	bool mInFillList;
	QAction *mActionSortByState;
	QAction *mActionFilterConnected;

	/* Color definitions (for standard see default.qss) */
	QColor mTextColorOnline;

	Ui::FriendSelectionWidget *ui;

	friend class FriendSelectionDialog ;

	std::vector<RsGxsGroupId> gxsIds ;
	QList<QAction*> mContextMenuActions;

    std::set<std::string> mPreSelectedIds; // because loading of GxsIds is asynchroneous we keep selected Ids from the client in a list here and use it to initialize after loading them.

    FontSizeHandler mFontSizeHandler;

    RsEventsHandlerId_t mEventHandlerId_identities;
    RsEventsHandlerId_t mEventHandlerId_peers;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(FriendSelectionWidget::ShowTypes)

#endif // FRIENDSELECTIONWIDGET_H
