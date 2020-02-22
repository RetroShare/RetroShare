/*******************************************************************************
 * gui/PlayerPage.cpp                                                          *
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

#include "PlayerPage.h"
#include "ui_PlayerPage.h"

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#include <QUrlQuery>
#endif

#include <iostream>
#include <string>

#include <QtWidgets>
#include <QVideoWidget>


PlayerPage::PlayerPage(QWidget *parent) :
	MainPage(parent),
	ui(new Ui::PlayerPage)
{
	ui->setupUi(this);

	mediaPlayer = new QMediaPlayer(this, QMediaPlayer::VideoSurface);
	videoWidget = new QVideoWidget;

	videoWidget->show();
	videoWidget->setStyleSheet("background-color: #1f1f1f;");

	playButton = new QPushButton;
	playButton->setEnabled(false);
	playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));

	stopButton = new QPushButton;
	stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
	stopButton->setEnabled(false);

	fullScreenButton = new QPushButton(tr("FullScreen"));
	fullScreenButton->setCheckable(true);
	fullScreenButton->setEnabled(false);

	positionSlider = new QSlider(Qt::Horizontal);
	positionSlider->setRange(0, mediaPlayer->duration() / 1000);

	labelDuration = new QLabel;

	errorLabel = new QLabel;
	errorLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

	QBoxLayout *controlLayout = new QHBoxLayout;
	controlLayout->setMargin(0);
	
	controlLayout->addWidget(playButton);
	controlLayout->addWidget(stopButton);
	controlLayout->addWidget(positionSlider);
	controlLayout->addWidget(labelDuration);
	controlLayout->addWidget(fullScreenButton);

	QBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(videoWidget);
	layout->addLayout(controlLayout);
	layout->addWidget(errorLabel);

	ui->widget->setLayout(layout);

	mediaPlayer->setVideoOutput(videoWidget);

	connect(playButton, &QAbstractButton::clicked,this, &PlayerPage::play);
	connect(stopButton, &QAbstractButton::clicked,this, &PlayerPage::stopVideo);
	connect(mediaPlayer, &QMediaPlayer::stateChanged,this, &PlayerPage::mediaStateChanged);
	connect(mediaPlayer, &QMediaPlayer::positionChanged, this, &PlayerPage::positionChanged);
	connect(mediaPlayer, &QMediaPlayer::durationChanged, this, &PlayerPage::durationChanged);
	connect(mediaPlayer, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error),this, &PlayerPage::handleError);
	connect(mediaPlayer, SIGNAL(videoAvailableChanged(bool)), this, SLOT(videoAvailableChanged(bool)));
	connect(positionSlider, &QSlider::sliderMoved, this, &PlayerPage::seek);
}

PlayerPage::~PlayerPage()
{
	delete ui;
}

void PlayerPage::setUrl(const QUrl &url)
{
	errorLabel->setText(QString());
	mediaPlayer->setMedia(url);
	playButton->setEnabled(true);
	play();
}

void PlayerPage::play()
{
	switch (mediaPlayer->state()) {
	//case QMediaPlayer::StoppedState:
	case QMediaPlayer::PlayingState:
		mediaPlayer->pause();
		break;
	default:
		mediaPlayer->play();
		break;
	}
}

void PlayerPage::stopVideo()
{
	mediaPlayer->stop();
}

void PlayerPage::mediaStateChanged(QMediaPlayer::State state)
{
	switch (state) {
		case QMediaPlayer::StoppedState:
			stopButton->setEnabled(false);
			playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
			break;
		case QMediaPlayer::PlayingState:
			stopButton->setEnabled(true);
			playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
			break;
		case QMediaPlayer::PausedState:
			stopButton->setEnabled(true);
			playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
			break;
	}
}

void PlayerPage::positionChanged(qint64 progress)
{
	 if (!positionSlider->isSliderDown())
		positionSlider->setValue(progress / 1000);

	updateDurationInfo(progress / 1000);
}

void PlayerPage::durationChanged(qint64 duration)
{
	duration = duration / 1000;
	positionSlider->setMaximum(duration);
}

void PlayerPage::seek(int seconds)
{
	mediaPlayer->setPosition(seconds * 1000);
}

void PlayerPage::handleError()
{
	playButton->setEnabled(false);
	const QString errorString = mediaPlayer->errorString();
	QString message = "Error: ";
	if (errorString.isEmpty())
		message += " #" + QString::number(int(mediaPlayer->error()));
	else
		message += errorString;
	errorLabel->setText(message);
}

void PlayerPage::videoAvailableChanged(bool available)
{
    if (!available) {
        disconnect(fullScreenButton, SIGNAL(clicked(bool)),
                    videoWidget, SLOT(setFullScreen(bool)));
        disconnect(videoWidget, SIGNAL(fullScreenChanged(bool)),
                fullScreenButton, SLOT(setChecked(bool)));
        videoWidget->setFullScreen(false);
    } else {
        connect(fullScreenButton, SIGNAL(clicked(bool)),
                videoWidget, SLOT(setFullScreen(bool)));
        connect(videoWidget, SIGNAL(fullScreenChanged(bool)),
                fullScreenButton, SLOT(setChecked(bool)));

        if (fullScreenButton->isChecked())
            videoWidget->setFullScreen(true);
    }

}

void PlayerPage::updateDurationInfo(qint64 currentInfo)
{
    QString tStr;
    if (currentInfo || duration) {
        QTime currentTime((currentInfo / 3600) % 60, (currentInfo / 60) % 60,
            currentInfo % 60, (currentInfo * 1000) % 1000);
        QTime totalTime((duration / 3600) % 60, (duration / 60) % 60,
            duration % 60, (duration * 1000) % 1000);
        QString format = "mm:ss";
        if (duration > 3600)
            format = "hh:mm:ss";
        tStr = currentTime.toString(format) + " / " + totalTime.toString(format);
    }
    labelDuration->setText(tStr);
}
