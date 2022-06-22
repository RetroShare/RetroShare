/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsMessageFrameWidget.h                          *
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

#ifndef GXSMESSAGEFRAMEWIDGET_H
#define GXSMESSAGEFRAMEWIDGET_H

#include "gui/gxs/RsGxsUpdateBroadcastWidget.h"

struct RsGxsIfaceHelper;
class UIStateHelper;

class GxsMessageFrameWidget : public QWidget
{
	Q_OBJECT

public:
	explicit GxsMessageFrameWidget(RsGxsIfaceHelper *ifaceImpl, QWidget *parent = NULL);
	virtual ~GxsMessageFrameWidget();

	const RsGxsGroupId &groupId();
	void setGroupId(const RsGxsGroupId &groupId);
	void setAllMessagesRead(bool read);

	virtual void groupIdChanged() = 0;
	virtual QString groupName(bool withUnreadCount) = 0;
	virtual QIcon groupIcon() = 0;
    virtual void blank() =0;
	virtual bool navigate(const RsGxsMessageId& msgId) = 0;
	virtual bool isLoading();
	virtual bool isWaiting();

	/* GXS functions */
	uint32_t nextTokenType() { return ++mNextTokenType; }

signals:
	void groupChanged(QWidget *widget);
	void waitingChanged(QWidget *widget);
	void loadComment(const RsGxsGroupId &groupId, const QVector<RsGxsMessageId>& msg_versions,const RsGxsMessageId &msgId, const QString &title);
    void groupDataLoaded();

protected:
    virtual void setAllMessagesReadDo(bool read) = 0;

protected:
	UIStateHelper *mStateHelper;

	/* Set read status */
	uint32_t mTokenTypeAcknowledgeReadStatus;
	uint32_t mAcknowledgeReadStatusToken;

private:
	RsGxsGroupId mGroupId; /* current group */
	uint32_t mNextTokenType;
};

#endif // GXSMESSAGEFRAMEWIDGET_H
