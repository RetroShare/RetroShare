#pragma once

#include "services/p3service.h"
#include "plugins/pluginmanager.h"

// The following class abstracts the construction of a peer-to-peer service. The user only has to 
// supply RS with a type ID. If the ID is already in use, RS will complain.
//
class RsPQIService: public p3Service, public p3Config
{
	public:
		RsPQIService(uint16_t type,uint32_t config_type,uint32_t tick_delay_in_seconds, RsPluginHandler* pgHandler) ;

		uint32_t tickDelay() const { return _tick_delay_in_seconds ; }
		virtual int tick() = 0 ;

		// Functions from p3config
		//
		virtual RsSerialiser *setupSerialiser() { return NULL ; }
		virtual bool saveList(bool&, std::list<RsItem*>&) { return false ;}
		virtual bool loadList(std::list<RsItem*>&) { return false ;}

	private:
		uint32_t _tick_delay_in_seconds ;
};

