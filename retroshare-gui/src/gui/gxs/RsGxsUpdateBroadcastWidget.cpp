/*******************************************************************************
 * retroshare-gui/src/gui/gxs/RsGxsUpdateBroadcastWidget.cpp                   *
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

#include "RsGxsUpdateBroadcastWidget.h"
#include "RsGxsUpdateBroadcastBase.h"

RsGxsUpdateBroadcastWidget::RsGxsUpdateBroadcastWidget(RsGxsIfaceHelper *ifaceImpl, QWidget *parent, Qt::WindowFlags flags)
    : QWidget(parent, flags)
{
    mBase = new RsGxsUpdateBroadcastBase(ifaceImpl, this);
    connect(mBase, SIGNAL(fillDisplay(bool)), this, SLOT(fillDisplay(bool)));

    mInterfaceHelper = ifaceImpl;
}

RsGxsUpdateBroadcastWidget::~RsGxsUpdateBroadcastWidget()
{
}

void RsGxsUpdateBroadcastWidget::fillComplete()
{
    mBase->fillComplete();
}

void RsGxsUpdateBroadcastWidget::setUpdateWhenInvisible(bool update)
{
    mBase->setUpdateWhenInvisible(update);
}

const std::set<RsGxsGroupId> &RsGxsUpdateBroadcastWidget::getGrpIds()
{
    return mBase->getGrpIds();
}

const std::set<TurtleRequestId>& RsGxsUpdateBroadcastWidget::getSearchResults()
{
    return mBase->getSearchResults();
}
const std::set<RsGxsGroupId> &RsGxsUpdateBroadcastWidget::getGrpIdsMeta()
{
    return mBase->getGrpIdsMeta();
}

void RsGxsUpdateBroadcastWidget::getAllGrpIds(std::set<RsGxsGroupId> &grpIds)
{
    mBase->getAllGrpIds(grpIds);
}

const std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &RsGxsUpdateBroadcastWidget::getMsgIds()
{
    return mBase->getMsgIds();
}

const std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &RsGxsUpdateBroadcastWidget::getMsgIdsMeta()
{
    return mBase->getMsgIdsMeta();
}

void RsGxsUpdateBroadcastWidget::getAllMsgIds(std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &msgIds)
{
    mBase->getAllMsgIds(msgIds);
}

void RsGxsUpdateBroadcastWidget::fillDisplay(bool complete)
{
    updateDisplay(complete);
    update(); // Qt flush
}

void RsGxsUpdateBroadcastWidget::showEvent(QShowEvent *event)
{
    mBase->showEvent(event);
    QWidget::showEvent(event);
}
