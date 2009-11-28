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

#ifndef _VIDEOPREVIEWCONFIGDIALOG_H_
#define _VIDEOPREVIEWCONFIGDIALOG_H_

#include "ui_videopreviewconfigdialog.h"
#include "videopreview.h"

class VideoPreviewConfigDialog : public QDialog, public Ui::VideoPreviewConfigDialog
{
	Q_OBJECT

public:
	VideoPreviewConfigDialog( QWidget* parent = 0, Qt::WindowFlags f = 0 );
	~VideoPreviewConfigDialog();

	void setVideoFile(const QString & video_file);
	QString videoFile();

	void setDVDDevice(const QString & dvd_device);
	QString DVDDevice();

	void setCols(int cols);
	int cols();

	void setRows(int rows);
	int rows();

	void setInitialStep(int step);
	int initialStep();

	void setMaxWidth(int w);
	int maxWidth();

	void setDisplayOSD(bool b);
	bool displayOSD();

	void setAspectRatio(double asp);
	double aspectRatio();

	void setFormat(VideoPreview::ExtractFormat format);
	VideoPreview::ExtractFormat format();

protected slots:
	void filenameChanged(const QString &);
};

#endif
