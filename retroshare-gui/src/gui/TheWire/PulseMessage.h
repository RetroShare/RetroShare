/*******************************************************************************
 * gui/TheWire/PulseMessage.h                                                  *
 *                                                                             *
 * Copyright (c) 2020-2020 Robert Fernie   <retroshare.project@gmail.com>      *
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

#ifndef MRK_PULSE_MSG_H
#define MRK_PULSE_MSG_H

#include "ui_PulseMessage.h"

#include <retroshare/rswire.h>

class QMovie;

class PulseMessage : public QWidget, private Ui::PulseMessage
{
  Q_OBJECT

public:
	PulseMessage(QWidget *parent); 

	void setup(RsWirePulseSPtr pulse);
	void setMessage(QString msg);
	void setRefImageCount(uint32_t count);
	void setLargeText(bool large); // detail view: bigger text, like Twitter

signals:
	void pictureOpened(); // the card marks the pulse as read

protected:
	// animated GIFs: in the feed they play only while the mouse is over the image,
	// in the thread view while it is shown; always paused when hidden.
	void showEvent(QShowEvent *event) override;
	void hideEvent(QHideEvent *event) override;
	bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
	void viewPictureOne();
	void viewPictureTwo();
	void viewPictureThree();
	void viewPictureFour();

private:
	RsWirePulseSPtr mPulse;
	bool mLarge = false; // thread (detail) view: bigger text, images over the full width

	QMovie *mMovies[4] = { nullptr, nullptr, nullptr, nullptr }; // animated GIFs (owned by the labels)
	void openPhotoView(const RsGxsImage &image);

};

#endif
