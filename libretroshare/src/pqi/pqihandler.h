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
#include "pqi/pqisecurity.h"

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)

#include "pqi/xpgpcert.h"

#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

#include "pqi/sslcert.h"

#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

#include <map>
#include <list>

class SearchModule
{
	public:
	int smi; // id.
	PQInterface *pqi;
	SecurityPolicy *sp;
};

// Presents a P3 Face to the world!
// and funnels data through to a PQInterface.
//
class pqihandler: public P3Interface
{
	public:
	pqihandler(SecurityPolicy *Global);
int	AddSearchModule(SearchModule *mod, int chanid = -1);
int	RemoveSearchModule(SearchModule *mod);

// P3Interface.
// 4 very similar outputs.....
virtual int	Search(SearchItem *ns);
virtual int	SearchSpecific(SearchItem *ns); /* search one person only */
virtual int	CancelSearch(SearchItem *ns); /* no longer used? */
virtual int	SendSearchResult(PQFileItem *);

// inputs.
virtual PQFileItem * 	GetSearchResult();
virtual SearchItem *	RequestedSearch();
virtual SearchItem *	CancelledSearch();

// file i/o
virtual int     SendFileItem(PQFileItem *ns);
virtual PQFileItem *	GetFileItem();

// Chat Interface
virtual int     SendMsg(ChatItem *item);
virtual int     SendGlobalMsg(ChatItem *ns); /* needed until chat complete */

virtual ChatItem *GetMsg();

// Rest of P3Interface
virtual int 	tick();
virtual int 	status();

// Control Interface
virtual int     SendOtherPQItem(PQItem *);
virtual PQItem *GetOtherPQItem();
virtual PQItem *SelectOtherPQItem(bool (*tst)(PQItem *));

	// rate control.
void	setMaxIndivRate(bool in, float val);
float	getMaxIndivRate(bool in);
void	setMaxRate(bool in, float val);
float	getMaxRate(bool in);

	protected:
	/* check to be overloaded by those that can
	 * generates warnings otherwise
	 */
virtual int checkOutgoingPQItem(PQItem *item, int global);

int	HandlePQItem(PQItem *ns, int allowglobal);

int	GetItems();
void	SortnStoreItem(PQItem *item);

	std::map<int, SearchModule *> mods;
	SecurityPolicy *globsec;

	// Temporary storage...
	std::list<PQItem *> in_result, in_endsrch, in_reqsrch, in_reqfile,
		in_file, in_chat, in_info, in_host, in_other;

	private:

	// rate control.
int	UpdateRates();

	float rateIndiv_in;
	float rateIndiv_out;
	float rateMax_in;
	float rateMax_out;

};

inline void pqihandler::setMaxIndivRate(bool in, float val)
{
	if (in)
		rateIndiv_in = val;
	else
		rateIndiv_out = val;
	return;
}

inline float pqihandler::getMaxIndivRate(bool in)
{
	if (in)
		return rateIndiv_in;
	else
		return rateIndiv_out;
}

inline void pqihandler::setMaxRate(bool in, float val)
{
	if (in)
		rateMax_in = val;
	else
		rateMax_out = val;
	return;
}

inline float pqihandler::getMaxRate(bool in)
{
	if (in)
		return rateMax_in;
	else
		return rateMax_out;
}

#endif // MRK_PQI_HANDLER_HEADER
