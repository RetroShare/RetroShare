#include <retroshare/rsplugin.h>

#include "LinksCloudPlugin.h"
#include "LinksDialog.h"

static void *inited = new LinksCloudPlugin() ;

extern "C" {
	void *RETROSHARE_PLUGIN_provide()
	{
		static LinksCloudPlugin *p = new LinksCloudPlugin() ;

		return (void*)p ;
	}
}

#define IMAGE_LINKS ":/images/irkick.png"

void LinksCloudPlugin::getPluginVersion(int& major,int& minor,int& svn_rev) const
{
	major = 5 ;
	minor = 1 ;
	svn_rev = 4350 ;
}

LinksCloudPlugin::LinksCloudPlugin()
{
	mRanking = NULL ;
	mainpage = NULL ;
	mIcon		= NULL ;
}

MainPage *LinksCloudPlugin::qt_page() const
{
	if(mainpage == NULL)
		mainpage = new LinksDialog ;

	return mainpage ;
}

RsCacheService *LinksCloudPlugin::rs_cache_service() const
{
	if(mRanking == NULL)
	{
		mRanking = new p3Ranking ; // , 3600 * 24 * 30 * 6); // 6 Months
		rsRanks = mRanking ;
	}

	return mRanking ;
}

QIcon *LinksCloudPlugin::qt_icon() const
{
	if(mIcon == NULL)
		mIcon = new QIcon(IMAGE_LINKS) ;

	return mIcon ;
}

std::string LinksCloudPlugin::getShortPluginDescription() const
{
	return "This plugin provides a set of cached links, and a voting system to promote them." ;
}


