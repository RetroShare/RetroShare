// interface class for p3Voip service
//

#pragma once

#include <stdint.h>

class RsVoip ;
extern RsVoip *rsVoip;

class RsVoip
{
	public:
		virtual int sendVoipData(const void *data,uint32_t size) = 0;

		typedef enum { AudioTransmitContinous = 0, AudioTransmitVAD = 1, AudioTransmitPushToTalk = 2 } enumAudioTransmit ;

		// Config stuff

		virtual int getVoipATransmit() const = 0 ;
		virtual void setVoipATransmit(int) const = 0 ;

		virtual int getVoipVoiceHold() const = 0 ;
		virtual void setVoipVoiceHold(int) const = 0 ;

		virtual int getVoipfVADmin() const = 0 ;
		virtual void setVoipfVADmin(int) const = 0 ;

		virtual int getVoipfVADmax() const = 0 ;
		virtual void setVoipfVADmax(int) const = 0 ;
		virtual int getVoipiNoiseSuppress() const = 0 ;
		virtual void setVoipiNoiseSuppress(int) const = 0 ;
		virtual int getVoipiMinLoudness() const = 0 ;
		virtual void setVoipiMinLoudness(int) const = 0 ;
		virtual bool getVoipEchoCancel() const = 0 ;
		virtual void setVoipEchoCancel(bool) const = 0 ;
};
