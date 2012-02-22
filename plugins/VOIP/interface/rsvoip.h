// interface class for p3Voip service
//

#pragma once

#include <stdint.h>

class RsVoip ;
extern RsVoip *rsVoip;
 
static const uint32_t CONFIG_TYPE_VOIP_PLUGIN = 0xe001 ;
static const uint32_t RS_SERVICE_TYPE_VOIP_PLUGIN = 0xe001 ;

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

class RsVoip
{
	public:
		virtual int sendVoipData(const std::string& peer_id,const void *data,uint32_t size) = 0;
		virtual int sendVoipHangUpCall(const std::string& peer_id) = 0;
		virtual int sendVoipRinging(const std::string& peer_id) = 0;
		virtual int sendVoipAcceptCall(const std::string& peer_id) = 0;

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


