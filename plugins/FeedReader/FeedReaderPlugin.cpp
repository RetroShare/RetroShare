/*******************************************************************************
 * plugins/FeedReader/FeedReaderPlugin.cpp                                     *
 *                                                                             *
 * Copyright (C) 2012 by Thunder <retroshare.project@gmail.com>                *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include <QApplication>
#include <QIcon>

#include <retroshare/rsplugin.h>
#include <QTranslator>

#include "FeedReaderPlugin.h"
#include "gui/FeedReaderDialog.h"
#include "gui/FeedReaderNotify.h"
#include "gui/FeedReaderConfig.h"
#include "gui/FeedReaderFeedNotify.h"
#include "services/p3FeedReader.h"

#include <libxml/xmlversion.h>
#include <libxslt/xsltconfig.h>
#include <curl/curlver.h>

#define IMAGE_FEEDREADER ":/images/FeedReader.png"

static void *inited = new FeedReaderPlugin();

extern "C" {
#ifdef WIN32
	__declspec(dllexport)
#endif
	RsPlugin *RETROSHARE_PLUGIN_provide()
	{
		return new FeedReaderPlugin();
	}

	// This symbol contains the svn revision number grabbed from the executable.
	// It will be tested by RS to load the plugin automatically, since it is safe to load plugins
	// with same revision numbers, assuming that the revision numbers are up-to-date.
	//
#ifdef WIN32
	__declspec(dllexport)
#endif
	uint32_t RETROSHARE_PLUGIN_revision = (uint32_t)atoi(RS_EXTRA_VERSION);

	// This symbol contains the svn revision number grabbed from the executable.
	// It will be tested by RS to load the plugin automatically, since it is safe to load plugins
	// with same revision numbers, assuming that the revision numbers are up-to-date.
	//
#ifdef WIN32
	__declspec(dllexport)
#endif
	uint32_t RETROSHARE_PLUGIN_api = RS_PLUGIN_API_VERSION ;
}

void FeedReaderPlugin::getPluginVersion(int& major, int& minor, int &build, int& svn_rev) const
{
	major = RS_MAJOR_VERSION;
	minor = RS_MINOR_VERSION;
	build = RS_MINI_VERSION;
	svn_rev = abs(atoi(RS_EXTRA_VERSION)) ;
}

FeedReaderPlugin::FeedReaderPlugin()
{
	mainpage = NULL ;
	mIcon = NULL ;
	mPlugInHandler = NULL;
	mFeedReader = NULL;
	mNotify = NULL;
	mFeedNotify = NULL;

	Q_INIT_RESOURCE(FeedReader_images);
	Q_INIT_RESOURCE(FeedReader_qss);
}

void FeedReaderPlugin::setInterfaces(RsPlugInInterfaces &interfaces)
{
	mInterfaces = interfaces;

	mFeedReader = new p3FeedReader(mPlugInHandler, mInterfaces.mGxsForums);
	rsFeedReader = mFeedReader;

	mNotify = new FeedReaderNotify();
	mFeedReader->setNotify(mNotify);
}

ConfigPage *FeedReaderPlugin::qt_config_page() const
{
	return new FeedReaderConfig();
}

MainPage *FeedReaderPlugin::qt_page() const
{
	if (mainpage == NULL) {
		mainpage = new FeedReaderDialog(mFeedReader, mNotify);
	}

	return mainpage;
}

FeedNotify *FeedReaderPlugin::qt_feedNotify()
{
	if (!mFeedNotify) {
		mFeedNotify = new FeedReaderFeedNotify(mFeedReader, mNotify);
	}
	return mFeedNotify;
}

void FeedReaderPlugin::stop()
{
	if (mFeedReader) {
		mFeedReader->setNotify(NULL);
		mFeedReader->stop();
	}
	if (mNotify) {
		delete(mNotify);
		mNotify = NULL;
	}
	if (mFeedNotify) {
		delete mFeedNotify;
		mFeedNotify = NULL;
	}
}

void FeedReaderPlugin::setPlugInHandler(RsPluginHandler *pgHandler)
{
	mPlugInHandler = pgHandler;
}

QIcon *FeedReaderPlugin::qt_icon() const
{
	if (mIcon == NULL) {
		mIcon = new QIcon(IMAGE_FEEDREADER);
	}

	return mIcon;
}

std::string FeedReaderPlugin::getShortPluginDescription() const
{
	return QApplication::translate("FeedReaderPlugin", "This plugin provides a Feedreader.").toUtf8().constData();
}

std::string FeedReaderPlugin::getPluginName() const
{
	return QApplication::translate("FeedReaderPlugin", "FeedReader").toUtf8().constData();
}

void FeedReaderPlugin::getLibraries(std::list<RsLibraryInfo> &libraries)
{
	libraries.push_back(RsLibraryInfo("LibCurl", LIBCURL_VERSION));
	libraries.push_back(RsLibraryInfo("Libxml2", LIBXML_DOTTED_VERSION));
	libraries.push_back(RsLibraryInfo("libxslt", LIBXSLT_DOTTED_VERSION));
}

QTranslator* FeedReaderPlugin::qt_translator(QApplication */*app*/, const QString& languageCode, const QString& externalDir) const
{
	if (languageCode == "en") {
		return NULL;
	}

	QTranslator* translator = new QTranslator();
	if (translator->load(externalDir + "/FeedReader_" + languageCode + ".qm")) {
		return translator;
	} else if (translator->load(":/lang/FeedReader_" + languageCode + ".qm")) {
		return translator;
	}

	delete(translator);
	return NULL;
}
