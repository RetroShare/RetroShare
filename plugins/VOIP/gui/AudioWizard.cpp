/*******************************************************************************
 * plugins/VOIP/gui/AudioWizard.cpp                                            *
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
#include <QTimer>

#include "AudioWizard.h"
#include "audiodevicehelper.h"
#include "interface/rsVOIP.h"

#define iroundf(x) ( static_cast<int>(x) )

AudioWizard::~AudioWizard()
{
    if (ticker) {
        ticker->stop();
    }
    if (inputDevice) {
        inputDevice->stop();
        delete(inputDevice);
        inputDevice = NULL;
    }
    if (inputProcessor) {
        inputProcessor->close();
        delete(inputProcessor);
        inputProcessor = NULL;
    }
    if (outputDevice) {
        outputDevice->stop();
        delete(outputDevice);
        outputDevice = NULL;
    }
    if (outputProcessor) {
        outputProcessor->close();
        delete(outputProcessor);
        outputProcessor = NULL;
    }
}

AudioWizard::AudioWizard(QWidget *p) : QWizard(p) {
	bInit = true;
	bLastActive = false;
        //g.bInAudioWizard = true;

	ticker = new QTimer(this);
	ticker->setObjectName(QLatin1String("Ticker"));

        setupUi(this);
        inputProcessor = NULL;
        inputDevice = NULL;
        outputProcessor = NULL;
        outputDevice = NULL;

        abAmplify = new AudioBar(this);
        abAmplify->qcBelow = Qt::green;
        abAmplify->qcInside = QColor::fromRgb(255,128,0);
	abAmplify->qcAbove = Qt::red;

        verticalLayout_3->addWidget(abAmplify);

        if (rsVOIP->getVoipATransmit() == RsVOIP::AudioTransmitPushToTalk)
		qrPTT->setChecked(true);
        else if (rsVOIP->getVoipATransmit() == RsVOIP::AudioTransmitVAD)
                qrVAD->setChecked(true);
	else
                qrContinuous->setChecked(true);

        abVAD = new AudioBar(this);
	abVAD->qcBelow = Qt::red;
	abVAD->qcInside = Qt::yellow;
	abVAD->qcAbove = Qt::green;

        qsTransmitMin->setValue(rsVOIP->getVoipfVADmin());
        qsTransmitMax->setValue(rsVOIP->getVoipfVADmax());

        verticalLayout_6->addWidget(abVAD);

	// Volume
        qsMaxAmp->setValue(rsVOIP->getVoipiMinLoudness());

	setOption(QWizard::NoCancelButton, false);
	resize(700, 500);

        updateTriggerWidgets(qrVAD->isChecked());

	bTransmitChanged = false;

	iMaxPeak = 0;
	iTicks = 0;

	qpTalkingOn = QPixmap::fromImage(QImage(QLatin1String(":/images/talking_on.svg")).scaled(64,64));
	qpTalkingOff = QPixmap::fromImage(QImage(QLatin1String(":/images/talking_off.svg")).scaled(64,64));

	bInit = false;

	connect(this, SIGNAL(currentIdChanged(int)), this, SLOT(showPage(int)));

	ticker->setSingleShot(false);
	ticker->start(20);
        connect( ticker, SIGNAL( timeout ( ) ), this, SLOT( on_Ticker_timeout() ) );
}

void AudioWizard::on_qsMaxAmp_valueChanged(int v) {
        rsVOIP->setVoipiMinLoudness(qMin(v, 30000));
}

void AudioWizard::on_Ticker_timeout() {
        if (!inputProcessor) {
            inputProcessor = new QtSpeex::SpeexInputProcessor();
            inputProcessor->open(QIODevice::WriteOnly | QIODevice::Unbuffered);

            if (!inputDevice) {
                inputDevice = AudioDeviceHelper::getPreferedInputDevice();
            }
            inputDevice->start(inputProcessor);
            connect(inputProcessor, SIGNAL(networkPacketReady()), this, SLOT(loopAudio()));
        }

        if (!outputProcessor) {
            outputProcessor = new QtSpeex::SpeexOutputProcessor();
            outputProcessor->open(QIODevice::ReadOnly | QIODevice::Unbuffered);

            if (!outputDevice) {
                outputDevice = AudioDeviceHelper::getPreferedOutputDevice();
            }
            outputDevice->start(outputProcessor);
            connect(outputProcessor, SIGNAL(playingFrame(QByteArray*)), inputProcessor, SLOT(addEchoFrame(QByteArray*)));
        }

        abVAD->iBelow = qsTransmitMin->value();
        abVAD->iAbove = qsTransmitMax->value();
        rsVOIP->setVoipfVADmin(qsTransmitMin->value());
        rsVOIP->setVoipfVADmax(qsTransmitMax->value());

        abVAD->iValue = iroundf(inputProcessor->dVoiceAcivityLevel * 32767.0f + 0.5f);

        abVAD->update();

        int iPeak = inputProcessor->dMaxMic;

	if (iTicks++ >= 50) {
		iMaxPeak = 0;
		iTicks = 0;
	}
	if (iPeak > iMaxPeak)
		iMaxPeak = iPeak;

	abAmplify->iBelow = qsMaxAmp->value();
	abAmplify->iValue = iPeak;
	abAmplify->iPeak = iMaxPeak;
	abAmplify->update();

        bool active = inputProcessor->bPreviousVoice;
	if (active != bLastActive) {
		bLastActive = active;
		qlTalkIcon->setPixmap(active ? qpTalkingOn : qpTalkingOff);
	}
}

void AudioWizard::loopAudio() {
    while(inputProcessor && inputProcessor->hasPendingPackets()) {
        packetQueue.enqueue(inputProcessor->getNetworkPacket());
        QTimer* playEcho = new QTimer();
        playEcho->setSingleShot(true);
        connect( playEcho, SIGNAL( timeout ( ) ), this, SLOT( on_playEcho_timeout() ) );
        playEcho->start(1500);
    }
}

void AudioWizard::on_playEcho_timeout() {
        if(!packetQueue.isEmpty()) {
            if (!qcbStopEcho->isChecked()) {
                if (outputDevice && outputDevice->error() != QAudio::NoError) {
                    std::cerr << "Stopping output device. Error " << outputDevice->error() << std::endl;
                    outputDevice->stop();
                    //TODO : find a way to restart output device, but there is a pulseaudio locks that prevents it here but it works in ChatWidget.cpp
                    //outputDevice->start(outputProcessor);
                }
                outputProcessor->putNetworkPacket("myself_loop",packetQueue.dequeue());
            } else {
                packetQueue.dequeue();
            }
        }
}

void AudioWizard::on_qsTransmitMax_valueChanged(int v) {
        if (! bInit) {
                rsVOIP->setVoipfVADmax(v);
        }
}

void AudioWizard::on_qsTransmitMin_valueChanged(int v) {
        if (! bInit) {
                rsVOIP->setVoipfVADmin(v);
        }
}

void AudioWizard::on_qrVAD_clicked(bool on) {
	if (on) {
                rsVOIP->setVoipATransmit(RsVOIP::AudioTransmitVAD);
                updateTriggerWidgets(true);
		bTransmitChanged = true;
	}
}

void AudioWizard::on_qrPTT_clicked(bool on) {
	if (on) {
                rsVOIP->setVoipATransmit(RsVOIP::AudioTransmitPushToTalk);
                updateTriggerWidgets(false);
		bTransmitChanged = true;
	}
}

void AudioWizard::on_qrContinuous_clicked(bool on) {
        if (on) {
                rsVOIP->setVoipATransmit(RsVOIP::AudioTransmitContinous);
                updateTriggerWidgets(false);
                bTransmitChanged = true;
        }
}

void AudioWizard::updateTriggerWidgets(bool vad_on) {
        if (!vad_on)
            qwVAD->hide();
        else
            qwVAD->show();
}

void AudioWizard::on_qcbHighContrast_clicked(bool on) {
        abAmplify->highContrast = on;
        abVAD->highContrast = on;
}
