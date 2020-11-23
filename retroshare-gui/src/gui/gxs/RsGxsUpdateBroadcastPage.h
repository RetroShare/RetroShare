/*******************************************************************************
 * retroshare-gui/src/gui/gxs/RsGxsUpdateBroadcastPage.h                       *
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

#include <retroshare-gui/mainpage.h>
#include <retroshare/rsgxsifacetypes.h>

// This class implement a basic RS functionality which is that widgets displaying info
// should update regularly. They also should update only when visible, to save CPU time.
//
// Using this class simply needs to derive your widget from RsGxsUpdateBroadcastPage
// and oveload the updateDisplay() function with the actual code that updates the
// widget.
//

struct RsGxsIfaceHelper;
class RsGxsUpdateBroadcastBase;
typedef uint32_t TurtleRequestId ;

class RsGxsUpdateBroadcastPage : public MainPage
{
	Q_OBJECT

public:
	RsGxsUpdateBroadcastPage(RsGxsIfaceHelper* ifaceImpl, QWidget *parent = NULL, Qt::WindowFlags flags = 0);
	virtual ~RsGxsUpdateBroadcastPage();

	void fillComplete();
	void setUpdateWhenInvisible(bool update);
	const std::set<RsGxsGroupId> &getGrpIds();
	const std::set<RsGxsGroupId> &getGrpIdsMeta();
	void getAllGrpIds(std::set<RsGxsGroupId> &grpIds);
	const std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &getMsgIds();
	const std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &getMsgIdsMeta();
	void getAllMsgIds(std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &msgIds);

    virtual const std::set<TurtleRequestId> getSearchRequests() const { return std::set<TurtleRequestId>(); } // overload this for subclasses that provide distant search

protected:
	virtual void showEvent(QShowEvent *event);

	// This is overloaded in subclasses.
	virtual void updateDisplay(bool complete) = 0;

public slots:
	void fillDisplay(bool complete);

private:
	RsGxsUpdateBroadcastBase *mBase;
};
