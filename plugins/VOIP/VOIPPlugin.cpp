/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2015
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/
#include <retroshare/rsplugin.h>
#include <retroshare/rsversion.h>
#include <retroshare-gui/RsAutoUpdatePage.h>
#include <QTranslator>
#include <QApplication>
#include <QString>
#include <QIcon>
#include <QMessageBox>

#include "VOIPPlugin.h"
#include "interface/rsVOIP.h"

#include "gui/AudioInputConfig.h"
#include "gui/VOIPChatWidgetHolder.h"
#include "gui/VOIPGUIHandler.h"
#include "gui/VOIPNotify.h"
#include "gui/SoundManager.h"
#include "gui/chat/ChatWidget.h"

#define IMAGE_VOIP ":/images/talking_on.svg"

static void *inited = new VOIPPlugin() ;

extern "C" {

	// This is *the* functions required by RS plugin system to give RS access to the plugin.
	// Be careful to:
	// - always respect the C linkage convention
	// - always return an object of type RsPlugin*
	//
	void *RETROSHARE_PLUGIN_provide()
	{
		static VOIPPlugin *p = new VOIPPlugin() ;

		return (void*)p ;
	}

	// This symbol contains the svn revision number grabbed from the executable. 
	// It will be tested by RS to load the plugin automatically, since it is safe to load plugins
	// with same revision numbers, assuming that the revision numbers are up-to-date.
	//
	uint32_t RETROSHARE_PLUGIN_revision = RS_REVISION_NUMBER ;

	// This symbol contains the svn revision number grabbed from the executable. 
	// It will be tested by RS to load the plugin automatically, since it is safe to load plugins
	// with same revision numbers, assuming that the revision numbers are up-to-date.
	//
	uint32_t RETROSHARE_PLUGIN_api = RS_PLUGIN_API_VERSION ;
}

void VOIPPlugin::getPluginVersion(int& major, int& minor, int& build, int& svn_rev) const
{
	major = RS_MAJOR_VERSION ;
	minor = RS_MINOR_VERSION ;
	build = RS_BUILD_NUMBER ;
	svn_rev = RS_REVISION_NUMBER ;
}

VOIPPlugin::VOIPPlugin()
{
	mVOIP = NULL ;
	mPlugInHandler = NULL;
	mPeers = NULL;
	config_page = NULL ;
	mIcon = NULL ;

	mVOIPGUIHandler = new VOIPGUIHandler ;
	mVOIPNotify = new VOIPNotify ;

	QObject::connect(mVOIPNotify,SIGNAL(voipInvitationReceived(const QString&)),mVOIPGUIHandler,SLOT(ReceivedInvitation(const QString&)),Qt::QueuedConnection) ;
	QObject::connect(mVOIPNotify,SIGNAL(voipDataReceived(const QString&)),mVOIPGUIHandler,SLOT(ReceivedVoipData(const QString&)),Qt::QueuedConnection) ;
	QObject::connect(mVOIPNotify,SIGNAL(voipAcceptReceived(const QString&)),mVOIPGUIHandler,SLOT(ReceivedVoipAccept(const QString&)),Qt::QueuedConnection) ;
	QObject::connect(mVOIPNotify,SIGNAL(voipHangUpReceived(const QString&)),mVOIPGUIHandler,SLOT(ReceivedVoipHangUp(const QString&)),Qt::QueuedConnection) ;
	QObject::connect(mVOIPNotify,SIGNAL(voipBandwidthInfoReceived(const QString&,int)),mVOIPGUIHandler,SLOT(ReceivedVoipBandwidthInfo(const QString&,int)),Qt::QueuedConnection) ;
}

void VOIPPlugin::setInterfaces(RsPlugInInterfaces &interfaces)
{
    mPeers = interfaces.mPeers;
}

ConfigPage *VOIPPlugin::qt_config_page() const
{
	// The config pages are deleted when config is closed, so it's important not to static the
	// created object.
	//
	return new AudioInputConfig() ;
}

QDialog *VOIPPlugin::qt_about_page() const
{
	static QMessageBox *about_dialog = NULL ;
	
	if(about_dialog == NULL)
	{
		about_dialog = new QMessageBox() ;

		QString text ;
		text += QObject::tr("<h3>RetroShare VOIP plugin</h3><br/>   * Contributors: Cyril Soler, Josselin Jacquard<br/>") ;
		text += QObject::tr("<br/>The VOIP plugin adds VOIP to the private chat window of RetroShare. to use it, proceed as follows:<UL>") ;
		text += QObject::tr("<li> setup microphone levels using the configuration panel</li>") ;
		text += QObject::tr("<li> check your microphone by looking at the VU-metters</li>") ;
		text += QObject::tr("<li> in the private chat, enable sound input/output by clicking on the two VOIP icons</li></ul>") ;
		text += QObject::tr("Your friend needs to run the plugin to talk/listen to you, or course.") ;
		text += QObject::tr("<br/><br/>This is an experimental feature. Don't hesitate to send comments and suggestion to the RS dev team.") ;

		about_dialog->setText(text) ;
		about_dialog->setStandardButtons(QMessageBox::Ok) ;
	}

	return about_dialog ;
}

ChatWidgetHolder *VOIPPlugin::qt_get_chat_widget_holder(ChatWidget *chatWidget) const
{
	switch (chatWidget->chatType()) {
	case ChatWidget::CHATTYPE_PRIVATE:
		return new VOIPChatWidgetHolder(chatWidget);
	case ChatWidget::CHATTYPE_UNKNOWN:
	case ChatWidget::CHATTYPE_LOBBY:
	case ChatWidget::CHATTYPE_DISTANT:
		break;
	}

	return NULL;
}

p3Service *VOIPPlugin::p3_service() const
{
    if(mVOIP == NULL)
        rsVOIP = mVOIP = new p3VOIP(mPlugInHandler,mVOIPNotify) ; // , 3600 * 24 * 30 * 6); // 6 Months

	return mVOIP ;
}

void VOIPPlugin::setPlugInHandler(RsPluginHandler *pgHandler)
{
    mPlugInHandler = pgHandler;
}

QIcon *VOIPPlugin::qt_icon() const
{
	if (mIcon == NULL) {
		Q_INIT_RESOURCE(VOIP_images);

		mIcon = new QIcon(IMAGE_VOIP);
	}

	return mIcon;
}

std::string VOIPPlugin::getShortPluginDescription() const
{
	return QApplication::translate("VOIP", "This plugin provides voice communication between friends in RetroShare.").toUtf8().constData();
}

std::string VOIPPlugin::getPluginName() const
{
	return QApplication::translate("VOIPPlugin", "VOIP").toUtf8().constData();
}

QTranslator* VOIPPlugin::qt_translator(QApplication */*app*/, const QString& languageCode, const QString& externalDir) const
{
	if (languageCode == "en") {
		return NULL;
	}

	QTranslator* translator = new QTranslator();

	if (translator->load(externalDir + "/VOIP_" + languageCode + ".qm")) {
		return translator;
	} else if (translator->load(":/lang/VOIP_" + languageCode + ".qm")) {
		return translator;
	}

	delete(translator);
	return NULL;
}

void VOIPPlugin::qt_sound_events(SoundEvents &/*events*/) const
{
//	events.addEvent(QApplication::translate("VOIP", "VOIP"), QApplication::translate("VOIP", "Incoming call"), VOIP_SOUND_INCOMING_CALL);
}
