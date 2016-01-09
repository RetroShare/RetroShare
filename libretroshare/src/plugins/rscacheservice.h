#pragma once

#include <dbase/cachestrapper.h>
#include "plugins/pluginmanager.h"

// This code needs to be re-written to work with GXS. For now it is obsolete.
//

// The following class abstracts the construction of a cache service. The user only has to 
// supply RS with a type ID. If the ID is already in use, RS will complain.
//
class RsCacheService: public CacheSource, public CacheStore, public p3Config
{
	public:
                RsCacheService(uint16_t type,uint32_t tick_delay_in_seconds, RsPluginHandler* pgHandler) ;

		uint32_t tickDelay() const { return _tick_delay_in_seconds ; }
		virtual void tick() {}

		// Functions from p3config
		//
		virtual RsSerialiser *setupSerialiser() { return NULL ; }
		virtual bool saveList(bool&, std::list<RsItem*>&) =0;
		virtual bool loadList(std::list<RsItem*>&) =0;

	private:
		uint32_t _tick_delay_in_seconds ;
};
