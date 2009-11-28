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

#include "videopreviewconfigdialog.h"

VideoPreviewConfigDialog::VideoPreviewConfigDialog( QWidget* parent, Qt::WindowFlags f )
	: QDialog(parent, f)
{
	setupUi(this);

	connect(filename_edit->lineEdit(), SIGNAL(textChanged(const QString &)),
            this, SLOT(filenameChanged(const QString &)) );

	dvd_device_label->setVisible(false);
	dvd_device_edit->setVisible(false);

	aspect_ratio_combo->addItem(tr("Default"), 0);
	aspect_ratio_combo->addItem("4:3", (double) 4/3);
	aspect_ratio_combo->addItem("16:9", (double) 16/9);
	aspect_ratio_combo->addItem("2.35:1", 2.35);

	format_combo->addItem("jpg", VideoPreview::JPEG);
	format_combo->addItem("png", VideoPreview::PNG);

	filename_edit->setWhatsThis( tr("The preview will be created for the video you specify here.") );
	dvd_device_edit->setWhatsThis( tr("Enter here the DVD device or a folder with a DVD image.") );
	columns_spin->setWhatsThis( tr("The thumbnails will be arranged on a table.") +" "+ tr("This option specifies the number of columns of the table.") );
	rows_spin->setWhatsThis( tr("The thumbnails will be arranged on a table.") +" "+ tr("This option specifies the number of rows of the table.") );
	osd_check->setWhatsThis( tr("If you check this option, the playing time will be displayed at the bottom of each thumbnail.") );
	aspect_ratio_combo->setWhatsThis( tr("If the aspect ratio of the video is wrong, you can specify a different one here.") );
	initial_step_spin->setWhatsThis( tr("Usually the first frames are black, so it's a good idea to skip some seconds at the beginning of the video. "
                                        "This option allows to specify how many seconds will be skipped.") );
	max_width_spin->setWhatsThis( tr("This option specifies the maximum width in pixels that the generated preview image will have.") );
	format_combo->setWhatsThis( tr("Some frames will be extracted from the video in order to create the preview. Here you can choose "
                                   "the image format for the extracted frames. PNG may give better quality.") );

	layout()->setSizeConstraint(QLayout::SetFixedSize);
}

VideoPreviewConfigDialog::~VideoPreviewConfigDialog() {
}

void VideoPreviewConfigDialog::setVideoFile(const QString & video_file) {
	filename_edit->setText(video_file);
}

QString VideoPreviewConfigDialog::videoFile() {
	return filename_edit->text();
}

void VideoPreviewConfigDialog::setDVDDevice(const QString & dvd_device) {
	dvd_device_edit->setText(dvd_device);
}

QString VideoPreviewConfigDialog::DVDDevice() {
	return dvd_device_edit->text();
}

void VideoPreviewConfigDialog::setCols(int cols) {
	columns_spin->setValue(cols);
}

int VideoPreviewConfigDialog::cols() {
	return columns_spin->value();
}

void VideoPreviewConfigDialog::setRows(int rows) {
	rows_spin->setValue(rows);
}

int VideoPreviewConfigDialog::rows() {
	return rows_spin->value();
}

void VideoPreviewConfigDialog::setInitialStep(int step) {
	initial_step_spin->setValue(step);
}

int VideoPreviewConfigDialog::initialStep() {
	return initial_step_spin->value();
}

void VideoPreviewConfigDialog::setMaxWidth(int w) {
	max_width_spin->setValue(w);
}

int VideoPreviewConfigDialog::maxWidth() {
	return max_width_spin->value();
}

void VideoPreviewConfigDialog::setDisplayOSD(bool b) {
	osd_check->setChecked(b);
}

bool VideoPreviewConfigDialog::displayOSD() {
	return osd_check->isChecked();
}

void VideoPreviewConfigDialog::setAspectRatio(double asp) {
	int idx = aspect_ratio_combo->findData(asp);
	if (idx < 0) idx = 0;
	aspect_ratio_combo->setCurrentIndex(idx);
}

double VideoPreviewConfigDialog::aspectRatio() {
	int idx = aspect_ratio_combo->currentIndex();
	return aspect_ratio_combo->itemData(idx).toDouble();
}

void VideoPreviewConfigDialog::setFormat(VideoPreview::ExtractFormat format) {
	int idx = format_combo->findData(format);
	if (idx < 0) idx = 0;
	format_combo->setCurrentIndex(idx);
}

VideoPreview::ExtractFormat VideoPreviewConfigDialog::format() {
	int idx = format_combo->currentIndex();
	return (VideoPreview::ExtractFormat) format_combo->itemData(idx).toInt();
}

void VideoPreviewConfigDialog::filenameChanged(const QString & text) {
	qDebug("VideoPreviewConfigDialog::filenameChanged");

	bool b = text.startsWith("dvd:");
	dvd_device_label->setVisible(b);
	dvd_device_edit->setVisible(b);
}

#include "moc_videopreviewconfigdialog.cpp"
