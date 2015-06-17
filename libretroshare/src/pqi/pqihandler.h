/*
 * "$Id: pqihandler.h,v 1.10 2007-03-31 09:41:32 rmf24 Exp $"
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
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

#ifndef MRK_PQI_HANDLER_HEADER
#define MRK_PQI_HANDLER_HEADER

#include "pqi/pqi.h"
#include "pqi/pqiqos.h"

#include "util/rsthreads.h"
#include "retroshare/rstypes.h"
#include "retroshare/rsconfig.h"

#include <map>
#include <list>

class SearchModule
{
	public:
	RsPeerId peerid;
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
		void	setMaxRate(bool in, float val);
		float	getMaxRate(bool in);

		void	getCurrentRates(float &in, float &out);

		// TESTING INTERFACE.
		int     ExtractRates(std::map<RsPeerId, RsBwRates> &ratemap, RsBwRates &totals);
        int     ExtractOutQueueStatistics(OutQueueStatistics& stats) ;

	protected:
		/* check to be overloaded by those that can
		 * generates warnings otherwise
		 */

		int	locked_HandleRsItem(RsItem *ns, int allowglobal,uint32_t& size);
		bool  queueOutRsItem(RsItem *) ;

		virtual int locked_checkOutgoingRsItem(RsItem *item, int global);
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
		time_t last_m ;
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
