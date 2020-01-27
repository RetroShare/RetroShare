/*******************************************************************************
 * gui/PlayerPage.h                                                            *
 *                                                                             *
 * Copyright (C) 2020 RetroShare Team          <retroshare.project@gmail.com>  *
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

#ifndef PLAYERPAGE_H
#define PLAYERPAGE_H

#include <retroshare-gui/mainpage.h>
#include <retroshare/rsfiles.h>
#include <retroshare/rspeers.h>

#include <QMediaPlayer>
#include <QWidget>
#include <QVideoWidget>

class QAbstractButton;
class QPushButton;
class QSlider;
class QLabel;
class QUrl;


namespace Ui {
class PlayerPage;
}

class PlayerPage : public MainPage
{
	Q_OBJECT

public:
	explicit PlayerPage(QWidget *parent);
	~PlayerPage();
	
	virtual QIcon iconPixmap() const { return QIcon(":/icons/png/video-camera.png") ; } //MainPage
	virtual QString pageName() const { return tr("Player") ; } //MainPage
	virtual QString helpText() const { return ""; } //MainPage
	    
	void setUrl(const QUrl &url);
protected:
	//virtual void keyPressEvent(QKeyEvent *e) ;
	
public slots:
	
	void play();
	void stopVideo();

private slots:
	void mediaStateChanged(QMediaPlayer::State state);
	void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void handleError();
    void videoAvailableChanged(bool available);

	void seek(int seconds);

private:
	void updateDurationInfo(qint64 currentInfo);
	
	QMediaPlayer* mediaPlayer;
	QVideoWidget *videoWidget; 
    QPushButton *playButton;
	QPushButton *fullScreenButton;
	QPushButton *stopButton;
    QSlider *positionSlider;
    QLabel *errorLabel;
	QLabel *labelDuration;
	qint64 duration;
	
	Ui::PlayerPage *ui;


};

#endif // PlayerPage_H
