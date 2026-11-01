/*******************************************************************************
 * plugins/VOIP/gui/AudioWizard.h                                              *
 *                                                                             *
 * Copyright (C) 2005-2010 Thorvald Natvig <thorvald@natvig.com>               *
 * Copyright (C) 2012 by Retroshare Team <retroshare.project@gmail.com>        *
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

#pragma once

#include <QElapsedTimer>
#include <QPair>
#include <QPixmap>
#include <QQueue>
#include <QDialog>

#include "AudioStats.h"
#include "SpeexProcessor.h"
#include "gui/qtmultimedia_compat.h"

#include "ui_AudioWizard.h"

class QBuffer;
class QComboBox;
class QTimer;
class QVideoInputDevice;
class VideoProcessor;

// Audio & video setup wizard of the VOIP plugin. Everything is designed in AudioWizard.ui:
// a QDialog with a QStackedWidget (qswPages) holding one QWidget per page, and
// Back / Next / Finish / Cancel buttons. Each page has two dynamic properties,
// "pageTitle" and "pageSubTitle", shown in the header (editable in Designer).
//
// (QWizard is not used on purpose: QWizard in ClassicStyle froze RetroShare when opened.)
//
//   0. qwpIntro         Introduction
//   1. qwpAudioDevices  choose microphone and speakers, live microphone level, speaker test tone
//   2. qwpVolume        amplification, local echo to hear yourself
//   3. qwpTransmit      voice activity detection / continuous, thresholds, hold time, noise suppression
//   4. qwpVideo         choose the camera, live preview, maximum video bandwidth
//   5. qwpDone          summary
//
// Settings are changed live while the wizard runs (so that the meters show the real
// behaviour) and are restored if the wizard is cancelled.
class AudioWizard: public QDialog, private Ui::AudioWizard
{
	Q_OBJECT
	Q_DISABLE_COPY(AudioWizard)

public:
	// Page indexes, in the order of the pages of qswPages in AudioWizard.ui
	enum PageId { PageIntro = 0, PageAudioDevices, PageVolume, PageTransmit, PageVideo, PageDone };

	AudioWizard(QWidget *parent);
	~AudioWizard();

	virtual void accept() override;
	virtual void reject() override;

private slots:
	void goBack();
	void goNext();
	void onPageChanged(int id);
	void onTicker();
	void loopAudio();

	// audio device page
	void onInputDeviceChanged(int);
	void onOutputDeviceChanged(int);
	void refreshAudioDevices();
	void playTestSound();
	void onTestSoundStateChanged(QAudio::State);
	void onTestSoundFinished() { stopTestSound() ; }
	void onEchoCancelToggled(bool);

	// volume page
	void onMaxAmpChanged(int);
	void onHighContrastToggled(bool);

	// transmission page
	void onTransmitModeChanged();
	void onVADMinChanged(int);
	void onVADMaxChanged(int);
	void onVoiceHoldChanged(int);
	void onNoiseChanged(int);

	// video page
	void onCameraChanged(int);
	void refreshCameras();
	void onShowEncodedToggled(bool);
	void onBandwidthChanged(double);
	void onCameraError(QString);

private:
	int  currentId() const;
	void updateHeaderAndButtons();
	void fillAudioDeviceCombo(QComboBox *cb, bool input, const QString& selected);
	QString selectedInputDevice() const;
	QString selectedOutputDevice() const;
	QString selectedCamera() const;

	void startAudio();
	void stopAudio();
	void stopTestSound();

	void startVideo();
	void stopVideo();

	void stopEverything();
	void updateTransmitWidgets();
	void updateSummary();
	void restoreSettings();
	int  chosenTransmitMode() const;

	// ---- saved settings, restored on cancel ----
	struct SavedSettings {
		int atransmit, voiceHold, vadMin, vadMax, noiseSuppress, minLoudness, videoBandwidth;
		bool echoCancel;
		std::string audioIn, audioOut, videoIn;
	} mSaved;

	// device lists are only filled when their page is shown for the first time
	bool mAudioDevicesFilled;
	bool mCamerasFilled;

	// ---- audio engine ----
	RsAudioInput  *inputDevice;
	RsAudioOutput *outputDevice;
	QtSpeex::SpeexInputProcessor  *inputProcessor;
	QtSpeex::SpeexOutputProcessor *outputProcessor;
	QQueue<QPair<qint64,QByteArray> > packetQueue;   // (time stamp in ms, packet) for the delayed local echo
	QElapsedTimer echoClock;

	RsAudioOutput *testOutput;
	QBuffer         *testBuffer;
	QByteArray       testTone;

	// ---- video engine ----
	QVideoInputDevice *videoInput;
	VideoProcessor    *videoProcessor;
	QElapsedTimer      videoStartTime;    // to warn when the camera delivers no image
	bool               videoNoFrameWarned;

	QTimer *ticker;
	bool bInit;
	bool bLastActive;
	int iMaxPeak;
	int iTicks;
	int iHeartbeats;
	QPixmap qpTalkingOn, qpTalkingOff;
};
