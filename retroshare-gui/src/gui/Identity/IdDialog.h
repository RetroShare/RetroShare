/*
 * Retroshare Identity.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#ifndef IDENTITYDIALOG_H
#define IDENTITYDIALOG_H

#include "gui/gxs/RsGxsUpdateBroadcastPage.h"

#include <retroshare/rsidentity.h>

#include "util/TokenQueue.h"

#define IMAGE_IDDIALOG          ":/icons/friends_128.png"

namespace Ui {
class IdDialog;
}

class UIStateHelper;
class QTreeWidgetItem;

class IdDialog : public RsGxsUpdateBroadcastPage, public TokenResponse
{
	Q_OBJECT

public:
	IdDialog(QWidget *parent = 0);
	~IdDialog();

	virtual QIcon iconPixmap() const { return QIcon(IMAGE_IDDIALOG) ; } //MainPage
	virtual QString pageName() const { return tr("People") ; } //MainPage
	virtual QString helpText() const { return ""; } //MainPage

	void loadRequest(const TokenQueue *queue, const TokenRequest &req);

protected:
	virtual void updateDisplay(bool complete);

private slots:
	void filterComboBoxChanged();
	void filterChanged(const QString &text);

	void addIdentity();
	void removeIdentity();
	void editIdentity();
	void chatIdentity();
	void sendMsg();


	void updateSelection();

	void modifyReputation();

	/** Create the context popup menu and it's submenus */
	void IdListCustomPopupMenu( QPoint point );

	void  addtoContacts();
  void  removefromContacts();

	void banPerson();
	void unbanPerson();
	
	static QString inviteMessage();
	void sendInvite();

private:
	void processSettings(bool load);

	void requestIdDetails();
	void insertIdDetails(uint32_t token);

	void requestIdList();
	void requestIdData(std::list<RsGxsGroupId> &ids);
	bool fillIdListItem(const RsGxsIdGroup& data, QTreeWidgetItem *&item, const RsPgpId &ownPgpId, int accept);
	void insertIdList(uint32_t token);
	void filterIds();

	void requestRepList();
	void insertRepList(uint32_t token);

	void requestIdEdit(std::string &id);
	void showIdEdit(uint32_t token);

private:
	TokenQueue *mIdQueue;
	UIStateHelper *mStateHelper;

	QTreeWidgetItem *contactsItem;
	QTreeWidgetItem *allItem;

	RsGxsGroupId mId;

	/* UI -  Designer */
	Ui::IdDialog *ui;
};

#endif
