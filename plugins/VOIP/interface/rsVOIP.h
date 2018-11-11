/*******************************************************************************
 * plugins/VOIP/interface/rsVOIP.h                                             *
 *                                                                             *
 * Copyright 2015 by retroshare team <retroshare.project@gmail.com>            *
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

// interface class for p3VOIP service
//

#pragma once

#include <stdint.h>
#include <string>
#include <list>
#include <vector>
#include <retroshare/rstypes.h>

class RsVOIP ;
extern RsVOIP *rsVOIP;
 
static const uint32_t CONFIG_TYPE_VOIP_PLUGIN 		= 0xe001 ;

class RsVOIPPongResult
{
	public:
	RsVOIPPongResult()
	:mTS(0), mRTT(0), mOffset(0) { return; }

	RsVOIPPongResult(double ts, double rtt, double offset)
	:mTS(ts), mRTT(rtt), mOffset(offset) { return; }

	double mTS;
	double mRTT;
	double mOffset;
};

struct RsVOIPDataChunk
{
	typedef enum { RS_VOIP_DATA_TYPE_UNKNOWN = 0x00,
	               RS_VOIP_DATA_TYPE_AUDIO   = 0x01,
	               RS_VOIP_DATA_TYPE_VIDEO   = 0x02 } RsVOIPDataType ;

	void *data ; // create/delete using malloc/free.
	uint32_t size ;
	RsVOIPDataType type ;	// video or audio

	void clear() ;
};

class RsVOIP
{
	public:
		virtual int sendVoipHangUpCall(const RsPeerId& peer_id, uint32_t flags) = 0;
		virtual int sendVoipRinging(const RsPeerId& peer_id, uint32_t flags) = 0;
		virtual int sendVoipAcceptCall(const RsPeerId& peer_id, uint32_t flags) = 0;

		// Sending data. The client keeps the memory ownership and must delete it after calling this.
		virtual int sendVoipData(const RsPeerId& peer_id,const RsVOIPDataChunk& chunk) = 0;

		// The server fill in the data and gives up memory ownership. The client must delete the memory
		// in each chunk once it has been used.
		//
		virtual bool getIncomingData(const RsPeerId& peer_id,std::vector<RsVOIPDataChunk>& chunks) = 0;

		typedef enum { AudioTransmitContinous = 0, AudioTransmitVAD = 1, AudioTransmitPushToTalk = 2 } enumAudioTransmit ;

		// Config stuff

		virtual int getVoipATransmit() const = 0 ;
		virtual void setVoipATransmit(int) = 0 ;
		virtual int getVoipVoiceHold() const = 0 ;
		virtual void setVoipVoiceHold(int) = 0 ;
		virtual int getVoipfVADmin() const = 0 ;
		virtual void setVoipfVADmin(int) = 0 ;
		virtual int getVoipfVADmax() const = 0 ;
		virtual void setVoipfVADmax(int) = 0 ;
		virtual int getVoipiNoiseSuppress() const = 0 ;
		virtual void setVoipiNoiseSuppress(int) = 0 ;
		virtual int getVoipiMinLoudness() const = 0 ;
		virtual void setVoipiMinLoudness(int) = 0 ;
		virtual bool getVoipEchoCancel() const = 0 ;
		virtual void setVoipEchoCancel(bool) = 0 ;

		virtual uint32_t getPongResults(const RsPeerId& id, int n, std::list<RsVOIPPongResult> &results) = 0;
};


