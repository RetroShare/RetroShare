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

#ifndef _MPLAYERPROCESS_H_
#define _MPLAYERPROCESS_H_

#include <QString>
#include "myprocess.h"
#include "mediadata.h"

class QStringList;

class MplayerProcess : public MyProcess 
{
	Q_OBJECT

public:
	MplayerProcess(QObject * parent = 0);
	~MplayerProcess();

	bool start();
	void writeToStdin(QString text);

	MediaData mediaData() { return md; };

signals:
	void processExited();
	void lineAvailable(QString line);

	void receivedCurrentSec(double sec);
	void receivedCurrentFrame(int frame);
	void receivedPause();
	void receivedWindowResolution(int,int);
	void receivedNoVideo();
	void receivedVO(QString);
	void receivedAO(QString);
	void receivedEndOfFile();
	void mplayerFullyLoaded();
	void receivedStartingTime(double sec);

	void receivedCacheMessage(QString);
	void receivedCreatingIndex(QString);
	void receivedConnectingToMessage(QString);
	void receivedResolvingMessage(QString);
	void receivedScreenshot(QString);

	void receivedStreamTitleAndUrl(QString,QString);

protected slots:
	void parseLine(QByteArray ba);
	void processFinished();
	void gotError(QProcess::ProcessError);

protected:
	void init_rx();

private:
	bool notified_mplayer_is_running;
	bool received_end_of_file;

	MediaData md;

	int last_sub_id;
};


#endif
