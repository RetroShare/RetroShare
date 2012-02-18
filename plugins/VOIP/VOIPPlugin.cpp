#include <retroshare/rsplugin.h>
#include <QTranslator>
#include <QApplication>

#include "VOIPPlugin.h"
#include "AudioInputConfig.h"

static void *inited = new VOIPPlugin() ;

extern "C" {
	void *RETROSHARE_PLUGIN_provide()
	{
		static VOIPPlugin *p = new VOIPPlugin() ;

		return (void*)p ;
	}
}

void VOIPPlugin::getPluginVersion(int& major,int& minor,int& svn_rev) const
{
	major = 5 ;
	minor = 1 ;
	svn_rev = 4350 ;
}

VOIPPlugin::VOIPPlugin()
{
	mVoip = NULL ;
	mPlugInHandler = NULL;
	mPeers = NULL;
	config_page = NULL ;
}

void VOIPPlugin::setInterfaces(RsPlugInInterfaces &interfaces)
{
    mPeers = interfaces.mPeers;
}

ConfigPage *VOIPPlugin::qt_config_page() const
{
	if(config_page == NULL)
		config_page = new AudioInputConfig() ;

	return config_page ;
}

RsPQIService *VOIPPlugin::rs_pqi_service() const
{
	if(mVoip == NULL)
	{
		mVoip = new p3VoipService(mPlugInHandler) ; // , 3600 * 24 * 30 * 6); // 6 Months
		rsVoipSI = mVoip ;
	}

	return mVoip ;
}

void VOIPPlugin::setPlugInHandler(RsPluginHandler *pgHandler)
{
    mPlugInHandler = pgHandler;
}

std::string VOIPPlugin::getShortPluginDescription() const
{
	return QApplication::translate("VOIP", "This plugin provides voice communication between friends in RetroShare.").toUtf8().constData();
}

std::string VOIPPlugin::getPluginName() const
{
	return QApplication::translate("VOIPPlugin", "VOIP").toUtf8().constData();
}

QTranslator* VOIPPlugin::qt_translator(QApplication *app, const QString& languageCode) const
{
	if (languageCode == "en") {
		return NULL;
	}

	QTranslator* translator = new QTranslator(app);
	if (translator->load(":/lang/VOIP_" + languageCode + ".qm")) {
		return translator;
	}

	delete(translator);
	return NULL;
}


