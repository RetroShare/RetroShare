/*******************************************************************************
 * retroshare-gui/src/gui/gxs/RsGxsUpdateBroadcastBase.h                       *
 *                                                                             *
 * Copyright 2014 Retroshare Team           <retroshare.project@gmail.com>     *
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

#pragma once

#include <QObject>
#include <retroshare/rsgxsifacetypes.h>

class QShowEvent;
struct RsGxsIfaceHelper;
class RsGxsUpdateBroadcast;

typedef uint32_t TurtleRequestId ;

class RsGxsUpdateBroadcastBase : public QObject
{
	friend class RsGxsUpdateBroadcastPage;
	friend class RsGxsUpdateBroadcastWidget;
	friend class GxsIdChooser;

	Q_OBJECT

public:
	RsGxsUpdateBroadcastBase(RsGxsIfaceHelper* ifaceImpl, QWidget *parent = NULL);
	virtual ~RsGxsUpdateBroadcastBase();

	const std::set<RsGxsGroupId> &getGrpIds() { return mGrpIds; }
	const std::set<RsGxsGroupId> &getGrpIdsMeta() { return mGrpIdsMeta; }
	void getAllGrpIds(std::set<RsGxsGroupId> &grpIds);
	const std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &getMsgIds() { return mMsgIds; }
	const std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &getMsgIdsMeta() { return mMsgIdsMeta; }
	void getAllMsgIds(std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &msgIds);
    const std::set<TurtleRequestId>& getSearchResults() { return mTurtleResults ; }

protected:
	void fillComplete();
	void setUpdateWhenInvisible(bool update) { mUpdateWhenInvisible = update; }

	void showEvent(QShowEvent *e);

signals:
	void fillDisplay(bool complete);

private slots:
	void updateBroadcastChanged();
	void updateBroadcastGrpsChanged(const std::list<RsGxsGroupId>& grpIds, const std::list<RsGxsGroupId> &grpIdsMeta);
	void updateBroadcastMsgsChanged(const std::map<RsGxsGroupId, std::set<RsGxsMessageId> >& msgIds, const std::map<RsGxsGroupId, std::set<RsGxsMessageId> >& msgIdsMeta);
	void updateBroadcastDistantSearchResultsChanged(const std::list<TurtleRequestId>& ids);
	void securedUpdateDisplay();

private:
	RsGxsUpdateBroadcast *mUpdateBroadcast;
	bool mFillComplete;
	bool mUpdateWhenInvisible; // Update also when not visible
	std::set<RsGxsGroupId> mGrpIds;
	std::set<RsGxsGroupId> mGrpIdsMeta;
	std::map<RsGxsGroupId, std::set<RsGxsMessageId> > mMsgIds;
	std::map<RsGxsGroupId, std::set<RsGxsMessageId> > mMsgIdsMeta;
    std::set<TurtleRequestId> mTurtleResults;
};
