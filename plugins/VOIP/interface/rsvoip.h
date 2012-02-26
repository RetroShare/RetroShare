// interface class for p3Voip service
//

#pragma once

#include <stdint.h>

class RsVoip ;
extern RsVoip *rsVoip;
 
static const uint32_t CONFIG_TYPE_VOIP_PLUGIN 		= 0xe001 ;

class RsVoipPongResult
{
	public:
	RsVoipPongResult()
	:mTS(0), mRTT(0), mOffset(0) { return; }

	RsVoipPongResult(double ts, double rtt, double offset)
	:mTS(ts), mRTT(rtt), mOffset(offset) { return; }

	double mTS;
	double mRTT;
	double mOffset;
};

struct RsVoipDataChunk
{
	void *data ; // create/delete using malloc/free.
	uint32_t size ;
};

class RsVoip
{
	public:
		virtual int sendVoipHangUpCall(const std::string& peer_id) = 0;
		virtual int sendVoipRinging(const std::string& peer_id) = 0;
		virtual int sendVoipAcceptCall(const std::string& peer_id) = 0;

		// Sending data. The client keeps the memory ownership and must delete it after calling this.
		virtual int sendVoipData(const std::string& peer_id,const RsVoipDataChunk& chunk) = 0;

		// The server fill in the data and gives up memory ownership. The client must delete the memory
		// in each chunk once it has been used.
		//
		virtual bool getIncomingData(const std::string& peer_id,std::vector<RsVoipDataChunk>& chunks) = 0;

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

		virtual uint32_t getPongResults(std::string id, int n, std::list<RsVoipPongResult> &results) = 0;
};


