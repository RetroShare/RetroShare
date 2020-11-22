/*******************************************************************************
 * libretroshare/src/rsserver: p3history.cc                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011 by Thunder.                                                  *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#include "p3history.h"
#include "pqi/p3historymgr.h"

p3History::p3History(p3HistoryMgr* historyMgr)
	: mHistoryMgr(historyMgr)
{
}

p3History::~p3History()
{
}

bool p3History::chatIdToVirtualPeerId(const ChatId &chat_id, RsPeerId &peer_id)
{
	return mHistoryMgr->chatIdToVirtualPeerId(chat_id, peer_id);
}

void p3History::setMaxStorageDuration(uint32_t seconds)
{
	mHistoryMgr->setMaxStorageDuration(seconds) ;
}
uint32_t p3History::getMaxStorageDuration()
{
	return mHistoryMgr->getMaxStorageDuration() ;
}
bool p3History::getMessages(const ChatId &chatPeerId, std::list<HistoryMsg> &msgs, const uint32_t loadCount)
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

void p3History::clear(const ChatId &chatPeerId)
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
