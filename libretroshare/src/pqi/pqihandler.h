/*******************************************************************************
 * libretroshare/src/pqi: pqihandler.h                                         *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2006 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef MRK_PQI_HANDLER_HEADER
#define MRK_PQI_HANDLER_HEADER

#include <stdint.h>              // for uint32_t
#include "util/rstime.h"                // for rstime_t, NULL
#include <list>                  // for list
#include <map>                   // for map

#include "pqi/pqi.h"             // for P3Interface, pqiPublisher
#include "retroshare/rstypes.h"  // for RsPeerId
#include "util/rsthreads.h"      // for RsStackMutex, RsMutex

class PQInterface;
struct RSTrafficClue;
class RsBwRates;
struct RsItem;
class RsRawItem;

class SearchModule
{
	public:
        	SearchModule() : pqi(NULL) {}
            
		RsPeerId peerid	;
		PQInterface *pqi;
};

// Presents a P3 Face to the world!
// and funnels data through to a PQInterface.
//
class pqihandler: public P3Interface, public pqiPublisher
{
	public:
        pqihandler();

		/**** Overloaded from pqiPublisher ****/
		virtual bool sendItem(RsRawItem *item)
		{
			return SendRsRawItem(item);
		}

		bool	AddSearchModule(SearchModule *mod);
		bool	RemoveSearchModule(SearchModule *mod);

		// Rest of P3Interface
		virtual int 	tick();
		virtual int 	status();

		// Service Data Interface
		virtual int     SendRsRawItem(RsRawItem *);
#ifdef TO_BE_REMOVED
		virtual RsRawItem *GetRsRawItem();
#endif

		// rate control.
		//void	setMaxRate(const RsPeerId& pid,bool in, uint32_t val_kBs);
		void	setMaxRate(bool in, float val);
		float	getMaxRate(bool in);
                
                void   setMaxRates(const RsPeerId& pid,bool in,float val) ;
                float  getMaxRates(const RsPeerId& pid,bool in) ;

		void	getCurrentRates(float &in, float &out);

		// TESTING INTERFACE.
		int     ExtractRates(std::map<RsPeerId, RsBwRates> &ratemap, RsBwRates &totals);
		int 	ExtractTrafficInfo(std::list<RSTrafficClue> &out_lst, std::list<RSTrafficClue> &in_lst);

		uint64_t traffInSum;
		uint64_t traffOutSum;
		void GetTraffic(uint64_t &in, uint64_t &out);

protected:
		/* check to be overloaded by those that can
		 * generates warnings otherwise
		 */

        int	locked_HandleRsItem(RsItem *ns, uint32_t& size);
		bool  queueOutRsItem(RsItem *) ;

#ifdef TO_BE_REMOVED
		int		locked_GetItems();
		void	locked_SortnStoreItem(RsItem *item);
#endif

		RsMutex coreMtx; /* MUTEX */

		std::map<RsPeerId, SearchModule *> mods;

		std::list<RsItem *> in_service;

	private:

		// rate control.
		int	UpdateRates();
		void	locked_StoreCurrentRates(float in, float out);

		float rateIndiv_in;
		float rateIndiv_out;
		float rateMax_in;
		float rateMax_out;

		float rateTotal_in;
		float rateTotal_out;

		uint32_t nb_ticks ;
		rstime_t last_m ;
        	rstime_t mLastRateCapUpdate ;
		float ticks_per_sec ;
};

inline void pqihandler::setMaxRate(bool in, float val)
{
	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/
	if (in)
		rateMax_in = val;
	else
		rateMax_out = val;
	return;
}

inline float pqihandler::getMaxRate(bool in)
{
	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/
	if (in)
		return rateMax_in;
	else
		return rateMax_out;
}

#endif // MRK_PQI_HANDLER_HEADER
