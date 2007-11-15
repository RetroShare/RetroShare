/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2007 Ricardo Villalba <rvm@escomposlinux.org>

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

#include "myprocess.h"

#ifdef Q_OS_WIN

#if QT_VERSION < 0x040300
#define USE_TEMP_FILE 1
#else
#define USE_TEMP_FILE 0
#endif

#else
#define USE_TEMP_FILE 0
#endif


MyProcess::MyProcess(QObject * parent) : QProcess(parent)
{
	clearArguments();
	setProcessChannelMode( QProcess::MergedChannels );
	
#if USE_TEMP_FILE
	temp_file.open(); // Create temporary file
	QString filename = temp_file.fileName();
	setStandardOutputFile( filename );
	qDebug("MyProcess::MyProcess: temporary file: %s", filename.toUtf8().data());
	temp_file.close();

	//connect(&temp_file, SIGNAL(readyRead()), this, SLOT(readTmpFile()) );
	connect(&timer, SIGNAL(timeout()), this, SLOT(readTmpFile()) );
#else
	connect(this, SIGNAL(readyReadStandardOutput()), this, SLOT(readStdOut()) );
#endif

	connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), 
            this, SLOT(procFinished()) ); 
}

void MyProcess::clearArguments() {
	program = "";
	arg.clear();
}

bool MyProcess::isRunning() {
	return (state() == QProcess::Running);
}

void MyProcess::addArgument(const QString & a) {
	if (program.isEmpty()) {
		program = a;
	} else {
		arg.append(a);
	}
}

QStringList MyProcess::arguments() {
	QStringList l = arg;
	l.prepend(program);
	return l;
}

void MyProcess::start() {
	remaining_output.clear();

	QProcess::start(program, arg);

#if USE_TEMP_FILE
	//bool r = temp_file.open(QIODevice::ReadOnly);
	bool r = temp_file.open();
	timer.start(50);
	qDebug("MyProcess::start: r: %d", r);
#endif
}

void MyProcess::readStdOut() {
	genericRead( readAllStandardOutput() );
}


void MyProcess::readTmpFile() {
	genericRead( temp_file.readAll() );
}

void MyProcess::genericRead(QByteArray buffer) {
	QByteArray ba = remaining_output + buffer;
	int start = 0;
	int from_pos = 0;
	int pos = canReadLine(ba, from_pos);

	//qDebug("MyProcess::read: pos: %d", pos);
	while ( pos > -1 ) {
		// Readline
		//QByteArray line = ba.left(pos);
		QByteArray line = ba.mid(start, pos-start);
		//ba = ba.mid(pos+1);
		from_pos = pos + 1;
#ifdef Q_OS_WIN
		if ((from_pos < ba.size()) && (ba.at(from_pos)=='\n')) from_pos++;
#endif
		start = from_pos;

		emit lineAvailable(line);

		pos = canReadLine(ba, from_pos);
	}

	remaining_output = ba.mid(from_pos);
}

int MyProcess::canReadLine(const QByteArray & ba, int from) {
	int pos1 = ba.indexOf('\n', from);
	int pos2 = ba.indexOf('\r', from);

	//qDebug("MyProcess::canReadLine: pos2: %d", pos2);

	if ( (pos1 == -1) && (pos2 == -1) ) return -1;

	int pos = pos1;
	if ( (pos1 != -1) && (pos2 != -1) ) {
		/*
		if (pos2 == (pos1+1)) pos = pos2; // \r\n
		else
		*/
		if (pos1 < pos2) pos = pos1; else pos = pos2;
	} else {
		if (pos1 == -1) pos = pos2;
		else
		if (pos2 == -1) pos = pos1;
	}

	return pos;
}

/*!
Do some clean up, and be sure that all output has been read.
*/
void MyProcess::procFinished() {
	qDebug("MyProcess::procFinished");

#if !USE_TEMP_FILE
	qDebug("MyProcess::procFinished: Bytes available: %ld", bytesAvailable());
	if ( bytesAvailable() > 0 ) readStdOut();
#else
	timer.stop();

	qDebug("MyProcess::procFinished: Bytes available: %ld", temp_file.bytesAvailable());
	if ( temp_file.bytesAvailable() > 0 ) readTmpFile();
	qDebug("MyProcess::procFinished: Bytes available: %ld", temp_file.bytesAvailable());

	temp_file.close();
#endif
}

#include "moc_myprocess.cpp"
