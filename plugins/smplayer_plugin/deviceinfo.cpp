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

#include "deviceinfo.h"
#include <QProcess>
#include <QFile>

#ifdef Q_OS_WIN

DeviceList DeviceInfo::retrieveDevices(DeviceType type) {
	qDebug("DeviceInfo::retrieveDevices: %d", type);
	
	DeviceList l;
	QRegExp rx_device("^(\\d+): (.*)");
	
	if (QFile::exists("dxlist.exe")) {
		QProcess p;
		p.setProcessChannelMode( QProcess::MergedChannels );
		QStringList arg;
		if (type == Sound) arg << "-s"; else arg << "-d";
		p.start("dxlist", arg);

		if (p.waitForFinished()) {
			QByteArray line;
			while (p.canReadLine()) {
				line = p.readLine().trimmed();
				qDebug("DeviceInfo::retrieveDevices: '%s'", line.constData());
				if ( rx_device.indexIn(line) > -1 ) {
					int id = rx_device.cap(1).toInt();
					QString desc = rx_device.cap(2);
					qDebug("DeviceInfo::retrieveDevices: found device: '%d' '%s'", id, desc.toUtf8().constData());
					l.append( DeviceData(id, desc) );
				}
			}
		}
	}
	
	return l;
}

DeviceList DeviceInfo::dsoundDevices() { 
	return retrieveDevices(Sound);
}

DeviceList DeviceInfo::displayDevices() {
	return retrieveDevices(Display);
}

#else

DeviceList DeviceInfo::alsaDevices() {
	qDebug("DeviceInfo::alsaDevices");

	DeviceList l;
	QRegExp rx_device("^card\\s([0-9]+).*\\[(.*)\\],\\sdevice\\s([0-9]+):");

	QProcess p;
	p.setProcessChannelMode( QProcess::MergedChannels );
	p.setEnvironment( QStringList() << "LC_ALL=C" );
	p.start("aplay", QStringList() << "-l");

	if (p.waitForFinished()) {
		QByteArray line;
		while (p.canReadLine()) {
			line = p.readLine();
			qDebug("DeviceInfo::alsaDevices: '%s'", line.constData());
			if ( rx_device.indexIn(line) > -1 ) {
				QString id = rx_device.cap(1);
				id.append(".");
				id.append(rx_device.cap(3));
				QString desc = rx_device.cap(2);
				qDebug("DeviceInfo::alsaDevices: found device: '%s' '%s'", id.toUtf8().constData(), desc.toUtf8().constData());
				l.append( DeviceData(id, desc) );
			}
		}
	} else {
		qDebug("DeviceInfo::alsaDevices: could not start aplay, error %d", p.error());
	}

	return l;
}

DeviceList DeviceInfo::xvAdaptors() {
	qDebug("DeviceInfo::xvAdaptors");

	DeviceList l;
	QRegExp rx_device("^.*Adaptor #([0-9]+): \"(.*)\"");

	QProcess p;
	p.setProcessChannelMode( QProcess::MergedChannels );
	p.setEnvironment( QProcess::systemEnvironment() << "LC_ALL=C" );
	p.start("xvinfo");

	if (p.waitForFinished()) {
		QByteArray line;
		while (p.canReadLine()) {
			line = p.readLine();
			qDebug("DeviceInfo::xvAdaptors: '%s'", line.constData());
			if ( rx_device.indexIn(line) > -1 ) {
				QString id = rx_device.cap(1);
				QString desc = rx_device.cap(2);
				qDebug("DeviceInfo::xvAdaptors: found adaptor: '%s' '%s'", id.toUtf8().constData(), desc.toUtf8().constData());
				l.append( DeviceData(id, desc) );
			}
		}
	} else {
		qDebug("DeviceInfo::xvAdaptors: could not start xvinfo, error %d", p.error());
	}

	return l;
}

#endif
