/*
 * Retroshare Gxs Support
 *
 * Copyright 2012-2013 by Robert Fernie.
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

#ifndef _GXS_ID_TREEWIDGETITEM_H
#define _GXS_ID_TREEWIDGETITEM_H

#include <retroshare/rsidentity.h>

#include "gui/common/RSTreeWidgetItem.h"
#include "gui/gxs/GxsIdDetails.h"

/*****
 * NOTE: When the tree item is created within a thread you have to move the object
 * with "moveThreadTo()" QObject into the main thread.
 * The tick signal to fill the id cannot be processed when the thread is finished.
 ***/

class GxsIdRSTreeWidgetItem : public QObject, public RSTreeWidgetItem
{
	Q_OBJECT

public:
    GxsIdRSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, uint32_t icon_mask,QTreeWidget *parent = NULL);

	void setId(const RsGxsId &id, int column, bool retryWhenFailed);
	bool getId(RsGxsId &id);

	int idColumn() const { return mColumn; }
    void processResult(bool success);
    uint32_t iconTypeMask() const { return mIconTypeMask ;}

	void setAvatar(const RsGxsImage &avatar);
	virtual QVariant data(int column, int role) const;
	void forceUpdate();
    
    	void setBannedState(bool b) { mBannedState = b; }	// does not actually change the state, but used instead by callbacks to leave a trace
    	void updateBannedState() ;				// checks reputation, and update is needed

private slots:
	void startProcess();

private:
	void init();

	RsGxsId mId;
	int mColumn;
	bool mIdFound;
    	bool mBannedState ;
	bool mRetryWhenFailed;
    RsReputations::ReputationLevel mReputationLevel ;
	uint32_t mIconTypeMask;
	RsGxsImage mAvatar;
};

#endif
