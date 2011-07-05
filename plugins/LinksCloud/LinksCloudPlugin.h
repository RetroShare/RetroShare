#pragma once

#include <retroshare/rsplugin.h>
#include <gui/mainpage.h>
#include "p3ranking.h"

class LinksCloudPlugin: public RsPlugin
{
	public:
		LinksCloudPlugin() ;
		virtual ~LinksCloudPlugin() {}

		virtual RsCacheService *rs_cache_service() 		const	;
		virtual MainPage       *qt_page()       			const	;
		virtual QIcon          *qt_icon()       			const	;
		virtual uint16_t        rs_service_id()         const { return RS_SERVICE_TYPE_RANK ; }

		virtual void getPluginVersion(int& major,int& minor,int& svn_rev) const ;

		virtual std::string configurationFileName() const { return std::string() ; }

		virtual std::string getShortPluginDescription() const ;
		virtual std::string getPluginName() const { return "LinksCloud" ; }
	private:
		mutable p3Ranking *mRanking ;
		mutable MainPage  *mainpage ;
		mutable QIcon	   *mIcon ;
};

