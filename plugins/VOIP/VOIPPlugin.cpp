#include <retroshare/rsplugin.h>
#include <retroshare-gui/RsAutoUpdatePage.h>
#include <QTranslator>
#include <QApplication>
#include <QString>

#include "VOIPPlugin.h"
#include "interface/rsvoip.h"

#include "gui/VoipStatistics.h"
#include "gui/AudioInputConfig.h"
#include "gui/AudioPopupChatDialog.h"
#include "gui/PluginGUIHandler.h"
#include "gui/PluginNotifier.h"

static void *inited = new VOIPPlugin() ;

extern "C" {

	// This is *the* function required by RS plugin system to give RS access to the plugin.
	// Be careful to:
	// - always respect the C linkage convention
	// - always return an object of type RsPlugin*
	//
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

	mPluginGUIHandler = new PluginGUIHandler ;
	mPluginNotifier = new PluginNotifier ;

	QObject::connect(mPluginNotifier,SIGNAL(voipInvitationReceived(const QString&)),mPluginGUIHandler,SLOT(ReceivedInvitation(const QString&)),Qt::QueuedConnection) ;
	QObject::connect(mPluginNotifier,SIGNAL(voipDataReceived(const QString&)),mPluginGUIHandler,SLOT(ReceivedVoipData(const QString&)),Qt::QueuedConnection) ;
	QObject::connect(mPluginNotifier,SIGNAL(voipAcceptReceived(const QString&)),mPluginGUIHandler,SLOT(ReceivedVoipAccept(const QString&)),Qt::QueuedConnection) ;
	QObject::connect(mPluginNotifier,SIGNAL(voipHangUpReceived(const QString&)),mPluginGUIHandler,SLOT(ReceivedVoipHangUp(const QString&)),Qt::QueuedConnection) ;
}

void VOIPPlugin::setInterfaces(RsPlugInInterfaces &interfaces)
{
    mPeers = interfaces.mPeers;
}

ConfigPage *VOIPPlugin::qt_config_page() const
{
	return new AudioInputConfig() ;
}

PopupChatDialog *VOIPPlugin::qt_allocate_new_popup_chat_dialog() const
{
	AudioPopupChatDialog *ap =	new AudioPopupChatDialog() ;

	return ap ;
}

std::string VOIPPlugin::qt_transfers_tab_name() const
{
	return QObject::tr("RTT Statistics").toStdString() ;
}
RsAutoUpdatePage *VOIPPlugin::qt_transfers_tab() const
{
	return new VoipStatistics ;
}

RsPQIService *VOIPPlugin::rs_pqi_service() const
{
	if(mVoip == NULL)
	{
		mVoip = new p3VoRS(mPlugInHandler,mPluginNotifier) ; // , 3600 * 24 * 30 * 6); // 6 Months
		rsVoip = mVoip ;
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


