/*
 * libretroshare/src/pqi: p3cfgmgr.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2007-2008 by Robert Fernie.
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



#ifndef P3_CONFIG_MGR_HEADER
#define P3_CONFIG_MGR_HEADER

#include <string>
#include <map>

#include "pqi/pqi_base.h"
#include "pqi/pqiindic.h"
#include "pqi/pqinetwork.h"

/***** Configuration Management *****
 *
 * we need to store:
 * (1) Certificates.
 * (2) List of Friends / Net Configuration
 * (3) Stun List. / DHT peers.
 * (4) general config.
 *
 *
 * At top level we need:
 *
 * - type / filename / size / hash - 
 * and the file signed...
 *
 *
 */

const uint32_t CONFIG_TYPE_GENERAL 	= 0x0001;
const uint32_t CONFIG_TYPE_STUNLIST 	= 0x0002;
const uint32_t CONFIG_TYPE_FSERVER 	= 0x0003;
const uint32_t CONFIG_TYPE_PEERS 	= 0x0004;

class pqiConfig
{
	public:	
	pqiConfig(uint32_t t, std::string defaultname)
	:ConfInd(2), type(t), filename(defaultname)
	{
		return;
	}
virtual ~pqiConfig() { return; }

virtual bool	loadConfiguration(std::string filename, std::string &load) = 0;
virtual bool	saveConfiguration(std::string filename) = 0;

	Indicator ConfInd;

uint32_t   Type()      { return type;     }
std::string Filename() { return filename; }
std::string Hash()     { return hash;     }

	protected:
void	setHash(std::string h) { hash = h; }

	private:
	uint32_t    type;
	std::string filename;
	std::string hash;
};


class p3ConfigMgr
{
	public:
	p3ConfigMgr(std::string bdir, std::string fname, std::string signame);

void	tick();
void	saveConfiguration();
void	loadConfiguration();
void	addConfiguration(uint32_t type, pqiConfig *conf);

	private:

std::map<uint32_t, pqiConfig *> configs;

std::string basedir;
std::string metafname;
std::string metasigfname;

};


class p3Config: public pqiConfig
{
	public:

	p3Config(uint32_t t, std::string name);

virtual bool	loadConfiguration(std::string basedir, std::string &loadHash);
virtual bool	saveConfiguration(std::string basedir);

	protected:

	/* Key Functions to be overloaded for Full Configuration */
virtual RsSerialiser *setupSerialiser() = 0;
virtual std::list<RsItem *> saveList(bool &cleanup) = 0;
virtual bool	loadList(std::list<RsItem *> load) = 0;


}; /* end of p3Config */


class p3GeneralConfig: public p3Config
{
	public:
	p3GeneralConfig();

// General Configuration System
std::string 	getSetting(std::string opt);
void 		setSetting(std::string opt, std::string val);

	protected:

	/* Key Functions to be overloaded for Full Configuration */
virtual RsSerialiser *setupSerialiser();
virtual std::list<RsItem *> saveList(bool &cleanup);
virtual bool	loadList(std::list<RsItem *> load);

	private:

std::map<std::string, std::string> settings;
};



	



#endif // P3_CONFIG_MGR_HEADER
