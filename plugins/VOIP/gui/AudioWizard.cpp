/* Copyright (C) 2005-2010, Thorvald Natvig <thorvald@natvig.com>

   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
   - Neither the name of the Mumble Developers nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <QTimer>

#include "AudioWizard.h"
//#include "AudioInput.h"
//#include "Global.h"
//#include "Settings.h"
//#include "Log.h"
//#include "MainWindow.h"
#include "audiodevicehelper.h"
#include "interface/rsvoip.h"

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

        if (rsVoip->getVoipATransmit() == RsVoip::AudioTransmitPushToTalk)
		qrPTT->setChecked(true);
        else if (rsVoip->getVoipATransmit() == RsVoip::AudioTransmitVAD)
                qrVAD->setChecked(true);
	else
                qrContinuous->setChecked(true);

        abVAD = new AudioBar(this);
	abVAD->qcBelow = Qt::red;
	abVAD->qcInside = Qt::yellow;
	abVAD->qcAbove = Qt::green;

        qsTransmitMin->setValue(rsVoip->getVoipfVADmin());
        qsTransmitMax->setValue(rsVoip->getVoipfVADmax());

        verticalLayout_6->addWidget(abVAD);

	// Volume
        qsMaxAmp->setValue(rsVoip->getVoipiMinLoudness());

	setOption(QWizard::NoCancelButton, false);
	resize(700, 500);

        updateTriggerWidgets(qrVAD->isChecked());

	bTransmitChanged = false;

	iMaxPeak = 0;
	iTicks = 0;

	qpTalkingOn = QPixmap::fromImage(QImage(QLatin1String("skin:talking_on.svg")).scaled(64,64));
	qpTalkingOff = QPixmap::fromImage(QImage(QLatin1String("skin:talking_off.svg")).scaled(64,64));

	bInit = false;

	connect(this, SIGNAL(currentIdChanged(int)), this, SLOT(showPage(int)));

	ticker->setSingleShot(false);
	ticker->start(20);
        connect( ticker, SIGNAL( timeout ( ) ), this, SLOT( on_Ticker_timeout() ) );
}

/*bool AudioWizard::eventFilter(QObject *obj, QEvent *evt) {
	if ((evt->type() == QEvent::MouseButtonPress) ||
	        (evt->type() == QEvent::MouseMove)) {
		QMouseEvent *qme = dynamic_cast<QMouseEvent *>(evt);
		if (qme) {
			if (qme->buttons() & Qt::LeftButton) {
				QPointF qpf = qgvView->mapToScene(qme->pos());
				fX = static_cast<float>(qpf.x());
				fY = static_cast<float>(qpf.y());
			}
		}
	}
	return QWizard::eventFilter(obj, evt);
}*/



void AudioWizard::on_qsMaxAmp_valueChanged(int v) {
        rsVoip->setVoipiMinLoudness(qMin(v, 30000));
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
        rsVoip->setVoipfVADmin(qsTransmitMin->value());
        rsVoip->setVoipfVADmax(qsTransmitMax->value());

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
                rsVoip->setVoipfVADmax(v);
        }
}

void AudioWizard::on_qsTransmitMin_valueChanged(int v) {
        if (! bInit) {
                rsVoip->setVoipfVADmin(v);
        }
}

void AudioWizard::on_qrVAD_clicked(bool on) {
	if (on) {
                rsVoip->setVoipATransmit(RsVoip::AudioTransmitVAD);
                updateTriggerWidgets(true);
		bTransmitChanged = true;
	}
}

void AudioWizard::on_qrPTT_clicked(bool on) {
	if (on) {
                rsVoip->setVoipATransmit(RsVoip::AudioTransmitPushToTalk);
                updateTriggerWidgets(false);
		bTransmitChanged = true;
	}
}

void AudioWizard::on_qrContinuous_clicked(bool on) {
        if (on) {
                rsVoip->setVoipATransmit(RsVoip::AudioTransmitContinous);
                updateTriggerWidgets(false);
                bTransmitChanged = true;
        }
}

/*void AudioWizard::on_skwPTT_keySet(bool valid, bool last) {
	if (valid)
		qrPTT->setChecked(true);
	else if (qrPTT->isChecked())
		qrAmplitude->setChecked(true);
	updateTriggerWidgets(valid);
	bTransmitChanged = true;

	if (last) {

		const QList<QVariant> &buttons = skwPTT->getShortcut();
		QList<Shortcut> ql;
		bool found = false;
		foreach(Shortcut s, g.s.qlShortcuts) {
			if (s.iIndex == g.mw->gsPushTalk->idx) {
				if (buttons.isEmpty())
					continue;
				else if (! found) {
					s.qlButtons = buttons;
					found = true;
				}
			}
			ql << s;
		}
		if (! found && ! buttons.isEmpty()) {
			Shortcut s;
			s.iIndex = g.mw->gsPushTalk->idx;
			s.bSuppress = false;
			s.qlButtons = buttons;
			ql << s;
		}
		g.s.qlShortcuts = ql;
		GlobalShortcutEngine::engine->bNeedRemap = true;
		GlobalShortcutEngine::engine->needRemap();
	}
}*/


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
