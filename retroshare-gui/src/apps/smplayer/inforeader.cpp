/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2008 Ricardo Villalba <rvm@escomposlinux.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "inforeader.h"
#include <QStringList>
#include <QApplication>
#include <QRegExp>

#include "helper.h"
#include "global.h"
#include "preferences.h"
#include "mplayerversion.h"

#if USE_QPROCESS
#include <QProcess>
#else
#include "myprocess.h"
#endif

using namespace Global;

#define NOME 0
#define VO 1
#define AO 2
#define DEMUXER 3
#define VC 4
#define AC 5

InfoReader * InfoReader::static_obj = 0;

InfoReader * InfoReader::obj() {
	if (!static_obj) {
		static_obj = new InfoReader( pref->mplayer_bin );
		static_obj->getInfo();
	}
	return static_obj;
}

InfoReader::InfoReader( QString mplayer_bin, QObject * parent )
	: QObject(parent)
{
	mplayerbin = mplayer_bin;

#if USE_QPROCESS
	proc = new QProcess(this);
	proc->setProcessChannelMode( QProcess::MergedChannels );
#else
	proc = new MyProcess(this);

	connect( proc, SIGNAL(lineAvailable(QByteArray)), 
             this, SLOT(readLine(QByteArray)) );
#endif
}

InfoReader::~InfoReader() {
}

void InfoReader::getInfo() {
	waiting_for_key = TRUE;
	vo_list.clear();
	ao_list.clear();
	demuxer_list.clear();
	mplayer_svn = -1;

	run("-identify -vo help -ao help -demuxer help -vc help -ac help");

	//list();
}

void InfoReader::list() {
	qDebug("InfoReader::list");

	InfoList::iterator it;

	qDebug(" vo_list:");
	for ( it = vo_list.begin(); it != vo_list.end(); ++it ) {
		qDebug( "driver: '%s', desc: '%s'", (*it).name().toUtf8().data(), (*it).desc().toUtf8().data());
	}

	qDebug(" ao_list:");
	for ( it = ao_list.begin(); it != ao_list.end(); ++it ) {
		qDebug( "driver: '%s', desc: '%s'", (*it).name().toUtf8().data(), (*it).desc().toUtf8().data());
	}

	qDebug(" demuxer_list:");
	for ( it = demuxer_list.begin(); it != demuxer_list.end(); ++it ) {
		qDebug( "demuxer: '%s', desc: '%s'", (*it).name().toUtf8().data(), (*it).desc().toUtf8().data());
	}

	qDebug(" vc_list:");
	for ( it = vc_list.begin(); it != vc_list.end(); ++it ) {
		qDebug( "codec: '%s', desc: '%s'", (*it).name().toUtf8().data(), (*it).desc().toUtf8().data());
	}

	qDebug(" ac_list:");
	for ( it = ac_list.begin(); it != ac_list.end(); ++it ) {
		qDebug( "codec: '%s', desc: '%s'", (*it).name().toUtf8().data(), (*it).desc().toUtf8().data());
	}

}

static QRegExp rx_vo_key("^ID_VIDEO_OUTPUTS");
static QRegExp rx_ao_key("^ID_AUDIO_OUTPUTS");
static QRegExp rx_demuxer_key("^ID_DEMUXERS");
static QRegExp rx_ac_key("^ID_AUDIO_CODECS");
static QRegExp rx_vc_key("^ID_VIDEO_CODECS");

static QRegExp rx_driver("\\t(.*)\\t(.*)");
static QRegExp rx_demuxer("^\\s+([A-Z,a-z,0-9]+)\\s+(\\d+)\\s+(\\S.*)");
static QRegExp rx_codec("^([A-Z,a-z,0-9]+)\\s+([A-Z,a-z,0-9]+)\\s+([A-Z,a-z,0-9]+)\\s+(\\S.*)");

void InfoReader::readLine(QByteArray ba) {
#if COLOR_OUTPUT_SUPPORT
    QString line = Helper::stripColorsTags(QString::fromLocal8Bit(ba));
#else
	QString line = QString::fromLocal8Bit(ba);
#endif

	qDebug("InfoReader::readLine: line: '%s'", line.toUtf8().data());
	//qDebug("waiting_for_key: %d", waiting_for_key);

	if (!waiting_for_key) {
		if ( rx_driver.indexIn(line) > -1 ) {
			QString name = rx_driver.cap(1);
			QString desc = rx_driver.cap(2);
			qDebug("InfoReader::readLine: found driver: '%s' '%s'", name.toUtf8().data(), desc.toUtf8().data());
			if (reading_type==VO) {
				vo_list.append( InfoData(name, desc) );
			} 
			else
			if (reading_type==AO) {
				ao_list.append( InfoData(name, desc) );
			}
			else
			qWarning("InfoReader::readLine: Unknown type! Ignoring");
		}
		else
		if ( rx_demuxer.indexIn(line) > -1 ) {
			QString name = rx_demuxer.cap(1);
			QString desc = rx_demuxer.cap(3);
			qDebug("InfoReader::readLine: found demuxer: '%s' '%s'", name.toUtf8().data(), desc.toUtf8().data());
			demuxer_list.append( InfoData(name, desc) );
		}
		else
		if ( rx_codec.indexIn(line) > -1 ) {
			QString name = rx_codec.cap(1);
			QString desc = rx_codec.cap(4);
			qDebug("InfoReader::readLine: found codec: '%s' '%s'", name.toUtf8().data(), desc.toUtf8().data());
			if (reading_type==VC) {
				vc_list.append( InfoData(name, desc) );
			} 
			else
			if (reading_type==AC) {
				ac_list.append( InfoData(name, desc) );
			}
			else
			qWarning("InfoReader::readLine: Unknown type! Ignoring");
		}
	}

	if ( rx_vo_key.indexIn(line) > -1 ) {
		reading_type = VO;
		waiting_for_key = FALSE;
		qDebug("InfoReader::readLine: found key: vo");
	}

	if ( rx_ao_key.indexIn(line) > -1 ) {
		reading_type = AO;
		waiting_for_key = FALSE;
		qDebug("InfoReader::readLine: found key: ao");
	}

	if ( rx_demuxer_key.indexIn(line) > -1 ) {
		reading_type = DEMUXER;
		waiting_for_key = FALSE;
		qDebug("InfoReader::readLine: found key: demuxer");
	}

	if ( rx_ac_key.indexIn(line) > -1 ) {
		reading_type = AC;
		waiting_for_key = FALSE;
		qDebug("InfoReader::readLine: found key: ac");
	}

	if ( rx_vc_key.indexIn(line) > -1 ) {
		reading_type = VC;
		waiting_for_key = FALSE;
		qDebug("InfoReader::readLines: found key: vc");
	}

	if (line.startsWith("MPlayer ")) {
		mplayer_svn = MplayerVersion::mplayerVersion(line);
	}
}

#if USE_QPROCESS
bool InfoReader::run(QString options) {
	qDebug("InfoReader::run: '%s'", options.toUtf8().data());
	qDebug("InfoReader::run: using QProcess");

	if (proc->state() == QProcess::Running) {
		qWarning("InfoReader::run: process already running");
		return false;
	}

	QStringList args = options.split(" ");

	proc->start(mplayerbin, args);
	if (!proc->waitForStarted()) {
		qWarning("InfoReader::run: process can't start!");
		return false;
	}

	//Wait until finish
	proc->waitForFinished();

	qDebug("InfoReader::run : terminating");

	QByteArray ba;
	while (proc->canReadLine()) {
		ba = proc->readLine();
		ba.replace("\n", "");
		ba.replace("\r", "");
		readLine( ba );
	}

	return true;
}
#else
bool InfoReader::run(QString options) {
	qDebug("InfoReader::run: '%s'", options.toUtf8().data());
	qDebug("InfoReader::run: using myprocess");

	if (proc->isRunning()) {
		qWarning("InfoReader::run: process already running");
		return false;
	}

	proc->clearArguments();

	proc->addArgument(mplayerbin);

	QStringList args = options.split(" ");
	QStringList::Iterator it = args.begin();
	while( it != args.end() ) {
		proc->addArgument( (*it) );
		++it;
	}

	proc->start();
	if (!proc->waitForStarted()) {
		qWarning("InfoReader::run: process can't start!");
		return false;
	}

	//Wait until finish
	proc->waitForFinished();

	qDebug("InfoReader::run : terminating");

	return true;
}
#endif

#include "moc_inforeader.cpp"
