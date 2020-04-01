/*******************************************************************************
 * util/RsGxsUpdateBroadcast.h                                                 *
 *                                                                             *
 * Copyright (c) 2014 Retroshare Team <retroshare.project@gmail.com>           *
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

#ifndef RSGXSUPDATEBROADCAST_H
#define RSGXSUPDATEBROADCAST_H

#include <QObject>

#include <retroshare/rsgxsifacetypes.h>
#include <retroshare/rsevents.h>

struct RsGxsIfaceHelper;
struct RsGxsChanges;

typedef uint32_t TurtleRequestId ;

class RS_DEPRECATED RsGxsUpdateBroadcast : public QObject
{
	Q_OBJECT

public:
	static void cleanup();

	static RsGxsUpdateBroadcast *get(RsGxsIfaceHelper* ifaceImpl);

protected:
	virtual ~RsGxsUpdateBroadcast();

signals:
	void changed();
	void msgsChanged(const std::map<RsGxsGroupId, std::set<RsGxsMessageId> >& msgIds, const std::map<RsGxsGroupId, std::set<RsGxsMessageId> >& msgIdsMeta);
	void grpsChanged(const std::list<RsGxsGroupId>& grpIds, const std::list<RsGxsGroupId>& grpIdsMeta);
	void distantSearchResultsChanged(const std::list<TurtleRequestId>& reqs);

private slots:
    void onChangesReceived(const RsGxsChanges& changes);

private:
	explicit RsGxsUpdateBroadcast(RsGxsIfaceHelper* ifaceImpl);

private:
	RsGxsIfaceHelper* mIfaceImpl;
	RsEventsHandlerId_t mEventHandlerId ;
};

#endif // RSGXSUPDATEBROADCAST_H
