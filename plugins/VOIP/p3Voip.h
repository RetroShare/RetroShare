/*
 * Services for RetroShare.
 *
 * Copyright 2011-2012 by Cyril Soler
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


#pragma once

#include <list>
#include <string>
#include <vector>

#include "serialiser/rsmsgitems.h"
#include "services/p3service.h"
#include "retroshare/rsmsgs.h"
#include "plugins/rspqiservice.h"
#include "rsvoip.h"

//!The basic VOIP service.
  
class p3VoipService: public RsPQIService, public RsVoip
{
	public:
		p3VoipService() : RsPQIService(RS_SERVICE_TYPE_VOIP) {}

		/***** overloaded from p3Service *****/
		/*!
		 * This retrieves all chat msg items and also (important!)
		 * processes chat-status items that are in service item queue. chat msg item requests are also processed and not returned
		 * (important! also) notifications sent to notify base  on receipt avatar, immediate status and custom status
		 * : notifyCustomState, notifyChatStatus, notifyPeerHasNewAvatar
		 * @see NotifyBase
		 */
		virtual void   tick();
		virtual int   status();

		// /*************** pqiMonitor callback ***********************/
		// virtual void statusChange(const std::list<pqipeer> &plist);

		/*!
		 * public chat sent to all peers
		 */
		int sendVoipData(const void *data,uint32_t size);

		// config values

		virtual int getVoipATransmit() const ;
		virtual void setVoipATransmit(int) const ;

		virtual int getVoipVoiceHold() const ;
		virtual void setVoipVoiceHold(int) const ;

		virtual int getVoipfVADmin() const ;
		virtual void setVoipfVADmin(int) const ;

		virtual int getVoipfVADmax() const ;
		virtual void setVoipfVADmax(int) const ;
		virtual int getVoipiNoiseSuppress() const ;
		virtual void setVoipiNoiseSuppress(int) const ;
		virtual int getVoipiMinLoudness() const ;
		virtual void setVoipiMinLoudness(int) const ;
		virtual bool getVoipEchoCancel() const ;
		virtual void setVoipEchoCancel(bool) const ;

	protected:
		/************* from p3Config *******************/
		virtual RsSerialiser *setupSerialiser() ;

		/*!
		 * chat msg items and custom status are saved
		 */
		virtual bool saveList(bool& cleanup, std::list<RsItem*>&) ;
		virtual bool loadList(std::list<RsItem*>& load) ;

	private:
		//RsMutex mChatMtx;

		void receiveVoipQueue();
};

