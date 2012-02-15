#pragma once

const uint8_t RS_VOIP_SUBTYPE_RINGING	= 0x01 ;
const uint8_t RS_VOIP_SUBTYPE_ACKNOWL	= 0x02 ;
const uint8_t RS_VOIP_SUBTYPE_DATA   	= 0x03 ;

class RsVoipItem: public RsItem
{
	public:
		RsVoipItem(uint8_t turtle_subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_VOIP,voip_subtype) {}

		virtual bool serialize(void *data,uint32_t& size) = 0 ;	// Isn't it better that items can serialize themselves ?
		virtual uint32_t serial_size() = 0 ; 							// deserialise is handled using a constructor

		virtual void clear() {} 
};

// to derive: hanshake items, data items etc.
