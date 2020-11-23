/*******************************************************************************
 * libretroshare/src/pqi: p3cfgmgr.h                                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2008 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef P3_CONFIG_MGR_HEADER
#define P3_CONFIG_MGR_HEADER

#include <string>
#include <map>
#include <set>

#include "pqi/pqi_base.h"
#include "pqi/pqiindic.h"
#include "pqi/pqinetwork.h"
#include "util/rsthreads.h"
#include "pqi/pqibin.h"

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

/**** THESE are STORED in CONFIGURATION FILES....
 * Cannot be changed
 *
 *********************/

/* CACHE ID Must be at the END so that other configurations
 * are loaded First (Cache Config --> Cache Loading)
 */

class p3ConfigMgr;



//! abstract class for configuration saving
/*!
 * Aim is that active objects in retroshare can dervie from this class
 * and implement their method of saving and loading their configuration
 */
class pqiConfig
{
	public:
	pqiConfig();
virtual ~pqiConfig();

/**
 * loads configuration of object
 * @param loadHash This is the hash that will be compared to confirm saved configuration has not
 * been tampered with
 */
virtual bool	loadConfiguration(RsFileHash &loadHash) = 0;

/**
 * save configuration of object
 */
virtual bool	saveConfiguration() = 0;

/**
 *  The name of the configuration file
 */
const std::string& Filename();

/**
 * The hash computed for this configuration, can use this to compare to externally stored hash
 * for validation checking
 */
const RsFileHash& Hash();

	protected:

/**
 * Checks if configuration has changed
 */
virtual void	IndicateConfigChanged();
void	setHash(const RsFileHash& h);

	RsMutex cfgMtx;

	/**
	 * This sets the name of the pqi configuation file
	 */
	void    setFilename(const std::string& name);

private:
	/**
	 * @param an index for the Confind which contains list of configuarations that can be tracked
	 */
	bool    HasConfigChanged(uint16_t idx);

	Indicator ConfInd;

	std::string filename;
	RsFileHash hash;

	friend class p3ConfigMgr;
	/* so it can access:
	 * setFilename() and HasConfigChanged()
	 */
};



/***********************************************************************************************/

/*!
 * MUTEX NOTE
 * Class data is protected by mutex's so that anyone can call these
 * functions, at any time.
 */
class p3ConfigMgr
{
	public:

		/**
		 * @param bdir base directory: where config files will be saved
		 */
	explicit p3ConfigMgr(std::string bdir);

        /**
         * checks and update all added configurations
         * @see rsserver
         */
        void	tick();

        /**
         * save all added configuation including configuration files
         * creates global signature file
         */
        void	saveConfiguration();

        /**
         * loads all configurations
         */
        void	loadConfiguration();

        /**
         * @param file The name for new configuration
         * @param conf to the configuration to use
         */
        void	addConfiguration(std::string file, pqiConfig *conf);

		/** saves config, and disables further saving
		 * used for exiting the system
		 */
		void	completeConfiguration();



	private:

		/**
		 * saves configuration of pqiconfigs in object configs
		 */
		void saveConfig();

		/**
		 *
		 */
		void loadConfig();

	const std::string basedir;

	RsMutex cfgMtx; /* below is protected */

	bool	mConfigSaveActive;
	std::list<pqiConfig *> mConfigs;
};



/***************************************************************************************************/


/**
 * @brief Abstract class for configuration saving.
 * Aimed at rs services that uses RsItem config data, provide a way for RS
 * services to save and load particular configurations as items.
 */
class p3Config : public pqiConfig
{
public:
	p3Config();

	virtual bool loadConfiguration(RsFileHash &loadHash);
	virtual bool saveConfiguration();

protected:

	/// Key Functions to be overloaded for Full Configuration
	virtual RsSerialiser *setupSerialiser() = 0;

	/**
	 * saves list of derived object
	 * @param cleanup this inform you if you need to call saveDone() to
	 *   unlock/allow access to resources pointed to by handles (list)
	 *   returned by function: thus false, call saveDone after returned list
	 *   finished with and vice versa
	 * @return list of config items derived object wants to saves
	 */
	virtual bool saveList(bool &cleanup, std::list<RsItem *>&) = 0;

	/**
	 * loads up list of configs items for derived object
	 * @param load list of config items to load up
	 */
	virtual bool loadList(std::list<RsItem *>& load) = 0;

	/**
	 * callback for mutex unlocking
	 * in derived classes (should only be needed if cleanup = false)
	 */
	virtual void saveDone() {}

private:

	bool loadConfig();
	bool saveConfig();

	bool loadAttempt( const std::string&, const std::string&,
	                  std::list<RsItem *>& load );
}; // end of p3Config


class p3GeneralConfig: public p3Config
{
	public:
	p3GeneralConfig();

// General Configuration System
std::string 	getSetting(const std::string &opt);
void 		setSetting(const std::string &opt, const std::string &val);

	protected:

	/* Key Functions to be overloaded for Full Configuration */
virtual RsSerialiser *setupSerialiser();
virtual bool saveList(bool &cleanup, std::list<RsItem* >&);
virtual bool	loadList(std::list<RsItem *>& );

	private:

	/* protected by pqiConfig mutex as well! */
std::map<std::string, std::string> settings;


};







#endif // P3_CONFIG_MGR_HEADER
