/*
 * "$Id: rsiface.h,v 1.9 2007-04-21 19:08:51 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2011-2011 by Cyril Soler
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

#pragma once

#include <time.h>
#include <stdint.h>
#include <string>
#include <vector>

class RsPluginHandler ;
extern RsPluginHandler *rsPlugins ;

class p3Service ;
class p3ConnectMgr ;
class MainPage ;
class QIcon ;
class RsCacheService ;
class ftServer ;
class pqiService ;

class RsPlugin
{
	public:
		virtual RsCacheService *rs_cache_service() 	const	{ return NULL ; }
		virtual pqiService     *rs_pqi_service() 		const	{ return NULL ; }
		virtual uint16_t        rs_service_id() 	   const	{ return 0    ; }
		virtual MainPage       *qt_page()       		const	{ return NULL ; }
		virtual QIcon          *qt_icon()       		const	{ return NULL ; }

		virtual std::string configurationFileName() const { return std::string() ; }
		virtual std::string getShortPluginDescription() const = 0 ;
		virtual std::string getPluginName() const = 0 ;
};

class RsPluginHandler
{
	public:
		// Returns the number of loaded plugins.
		//
		virtual int nbPlugins() const = 0 ;
		virtual RsPlugin *plugin(int i) = 0 ;
		virtual const std::vector<std::string>& getPluginDirectories() const = 0;

		virtual void slowTickPlugins(time_t sec) = 0 ;

		virtual const std::string& getLocalCacheDir() const =0;
		virtual const std::string& getRemoteCacheDir() const =0;
		virtual ftServer *getFileServer() const = 0;
		virtual p3ConnectMgr *getConnectMgr() const = 0;
};



