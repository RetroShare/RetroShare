/*
 * libretroshare/src/rsserver: p3history.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2011 by Thunder.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
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


#include "p3history.h"
#include "pqi/p3historymgr.h"

p3History::p3History(p3HistoryMgr* historyMgr)
	: mHistoryMgr(historyMgr)
{
}

p3History::~p3History()
{
}

void p3History::setMaxStorageDuration(uint32_t seconds)
{
	mHistoryMgr->setMaxStorageDuration(seconds) ;
}
uint32_t p3History::getMaxStorageDuration()
{
	return mHistoryMgr->getMaxStorageDuration() ;
}
bool p3History::getMessages(const RsPeerId &chatPeerId, std::list<HistoryMsg> &msgs, const uint32_t loadCount)
{
	return mHistoryMgr->getMessages(chatPeerId, msgs, loadCount);
}

bool p3History::getMessage(uint32_t msgId, HistoryMsg &msg)
{
	return mHistoryMgr->getMessage(msgId, msg);
}

void p3History::removeMessages(const std::list<uint32_t> &msgIds)
{
	mHistoryMgr->removeMessages(msgIds);
}

void p3History::clear(const RsPeerId &chatPeerId)
{
	mHistoryMgr->clear(chatPeerId);
}

bool p3History::getEnable(uint32_t chat_type)
{
	return mHistoryMgr->getEnable(chat_type);
}

void p3History::setEnable(uint32_t chat_type, bool enable)
{
	mHistoryMgr->setEnable(chat_type, enable);
}

uint32_t p3History::getSaveCount(uint32_t chat_type)
{
	return mHistoryMgr->getSaveCount(chat_type);
}

void p3History::setSaveCount(uint32_t chat_type, uint32_t count)
{
	mHistoryMgr->setSaveCount(chat_type, count);
}
