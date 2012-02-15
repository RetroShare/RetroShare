/* Copyright (C) 2005-2010, Thorvald Natvig <thorvald@natvig.com>
   Copyright (C) 2008, Andreas Messer <andi@bupfen.de>

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

//#include "AudioInput.h"
//#include "AudioOutput.h"
#include "AudioStats.h"
#include "AudioInputConfig.h"
//#include "Global.h"
//#include "NetworkConfig.h"
#include "rsharesettings.h"
#include "util/audiodevicehelper.h"
#include "AudioWizard.h"

#define iroundf(x) ( static_cast<int>(x) )

/*void AudioInputDialog::hideEvent(QHideEvent *) {
	qtTick->stop();
}

void AudioInputDialog::showEvent(QShowEvent *) {
	qtTick->start(20);
}*/

/** Constructor */
AudioInputConfig::AudioInputConfig(QWidget * parent, Qt::WFlags flags)
    : ConfigPage(parent, flags)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    loaded = false;
}

AudioInputConfig::~AudioInputConfig()
{
    if (inputDevice) {
        inputDevice->stop();
    }
}

/** Loads the settings for this page */
void AudioInputConfig::load()
{
    //connect( ui.allowIpDeterminationCB, SIGNAL( toggled( bool ) ), this, SLOT( toggleIpDetermination(bool) ) );
    //connect( ui.allowTunnelConnectionCB, SIGNAL( toggled( bool ) ), this, SLOT( toggleTunnelConnection(bool) ) );

    qtTick = new QTimer(this);
    connect( qtTick, SIGNAL( timeout ( ) ), this, SLOT( on_Tick_timeout() ) );
    qtTick->start(20);
    /*if (AudioInputRegistrar::qmNew) {
            QList<QString> keys = AudioInputRegistrar::qmNew->keys();
            foreach(QString key, keys) {
                    qcbSystem->addItem(key);
            }
    }
    qcbSystem->setEnabled(qcbSystem->count() > 1);*/

    ui.qcbTransmit->addItem(tr("Continuous"), RshareSettings::AudioTransmitContinous);
    ui.qcbTransmit->addItem(tr("Voice Activity"), RshareSettings::AudioTransmitVAD);
    ui.qcbTransmit->addItem(tr("Push To Talk"), RshareSettings::AudioTransmitPushToTalk);

    abSpeech = new AudioBar();
    abSpeech->qcBelow = Qt::red;
    abSpeech->qcInside = Qt::yellow;
    abSpeech->qcAbove = Qt::green;
    //abSpeech->setGeometry(9,20,50,10);
    ui.qwVadLayout_2->addWidget(abSpeech,0,0,1,0);


    //on_qcbPushClick_clicked(g.s.bPushClick);
    //ui.on_Tick_timeout();
    loadSettings();
    inputProcessor = NULL;
    inputDevice = NULL;
}


void AudioInputConfig::loadSettings() {
        /*QList<QString> keys;

        if (AudioInputRegistrar::qmNew)
		keys=AudioInputRegistrar::qmNew->keys();
	else
		keys.clear();
	i=keys.indexOf(AudioInputRegistrar::current);
	if (i >= 0)
                loadComboBox(qcbSystem, i);

        loadCheckBox(qcbExclusive, r.bExclusiveInput);*/

        //qlePushClickPathOn->setText(r.qsPushClickOn);
        //qlePushClickPathOff->setText(r.qsPushClickOff);

        /*loadComboBox(qcbTransmit, r.atTransmit);
	loadSlider(qsTransmitHold, r.iVoiceHold);
	loadSlider(qsTransmitMin, iroundf(r.fVADmin * 32767.0f + 0.5f));
	loadSlider(qsTransmitMax, iroundf(r.fVADmax * 32767.0f + 0.5f));
	loadSlider(qsFrames, (r.iFramesPerPacket == 1) ? 1 : (r.iFramesPerPacket/2 + 1));
        loadSlider(qsDoublePush, iroundf(static_cast<float>(r.uiDoublePush) / 1000.f + 0.5f));*/
        ui.qcbTransmit->setCurrentIndex(Settings->getVoipATransmit()-1);
        on_qcbTransmit_currentIndexChanged(Settings->getVoipATransmit()-1);
        ui.qsTransmitHold->setValue(Settings->getVoipVoiceHold());
        on_qsTransmitHold_valueChanged(Settings->getVoipVoiceHold());
        ui.qsTransmitMin->setValue(Settings->getVoipfVADmin());
        ui.qsTransmitMax->setValue(Settings->getVoipfVADmax());
        ui.qcbEchoCancel->setChecked(Settings->getVoipEchoCancel());
        //ui.qsDoublePush->setValue(iroundf(static_cast<float>(r.uiDoublePush) / 1000.f + 0.5f));

        //loadCheckBox(qcbPushClick, r.bPushClick);
        //loadSlider(qsQuality, r.iQuality);
        if (Settings->getVoipiNoiseSuppress() != 0)
                ui.qsNoise->setValue(-Settings->getVoipiNoiseSuppress());
	else
                ui.qsNoise->setValue(14);

        on_qsNoise_valueChanged(-Settings->getVoipiNoiseSuppress());

        ui.qsAmp->setValue(20000 - Settings->getVoipiMinLoudness());
        on_qsAmp_valueChanged(20000 - Settings->getVoipiMinLoudness());
        //loadSlider(qsIdle, r.iIdleTime);

        /*int echo = 0;
	if (r.bEcho)
		echo = r.bEchoMulti ? 2 : 1;

        loadComboBox(qcbEcho, echo);*/
        connect( ui.qsTransmitHold, SIGNAL( valueChanged ( int ) ), this, SLOT( on_qsTransmitHold_valueChanged(int) ) );
        connect( ui.qsNoise, SIGNAL( valueChanged ( int ) ), this, SLOT( on_qsNoise_valueChanged(int) ) );
        connect( ui.qsAmp, SIGNAL( valueChanged ( int ) ), this, SLOT( on_qsAmp_valueChanged(int) ) );
        connect( ui.qcbTransmit, SIGNAL( currentIndexChanged ( int ) ), this, SLOT( on_qcbTransmit_currentIndexChanged(int) ) );
        loaded = true;
}

bool AudioInputConfig::save(QString &/*errmsg*/) {//mainly useless beacause saving occurs in realtime
        //s.iQuality = qsQuality->value();
        Settings->setVoipiNoiseSuppress((ui.qsNoise->value() == 14) ? 0 : - ui.qsNoise->value());
        Settings->setVoipiMinLoudness(20000 - ui.qsAmp->value());
        Settings->setVoipVoiceHold(ui.qsTransmitHold->value());
        Settings->setVoipfVADmin(ui.qsTransmitMin->value());
        Settings->setVoipfVADmax(ui.qsTransmitMax->value());
        /*s.uiDoublePush = qsDoublePush->value() * 1000;*/
        Settings->setVoipATransmit(static_cast<RshareSettings::enumAudioTransmit>(ui.qcbTransmit->currentIndex() + 1));
        Settings->setVoipEchoCancel(ui.qcbEchoCancel->isChecked());

        return true;
}

/*bool AudioInputDialog::expert(bool b) {
        qgbInterfaces->setVisible(b);
	qgbAudio->setVisible(b);
	qliFrames->setVisible(b);
	qsFrames->setVisible(b);
	qlFrames->setVisible(b);
	qswTransmit->setVisible(b);
	qliIdle->setVisible(b);
	qsIdle->setVisible(b);
        qlIdle->setVisible(b);
	return true;
}*/


void AudioInputConfig::on_qsTransmitHold_valueChanged(int v) {
        float val = static_cast<float>(v * FRAME_SIZE);
        val = val / SAMPLING_RATE;
        ui.qlTransmitHold->setText(tr("%1 s").arg(val, 0, 'f', 2));
        Settings->setVoipVoiceHold(v);
}

void AudioInputConfig::on_qsNoise_valueChanged(int v) {
	QPalette pal;

	if (v < 15) {
                ui.qlNoise->setText(tr("Off"));
                pal.setColor(ui.qlNoise->foregroundRole(), Qt::red);
	} else {
                ui.qlNoise->setText(tr("-%1 dB").arg(v));
	}
        ui.qlNoise->setPalette(pal);
        Settings->setVoipiNoiseSuppress(- ui.qsNoise->value());
}

void AudioInputConfig::on_qsAmp_valueChanged(int v) {
        v = 20000 - v;
	float d = 20000.0f/static_cast<float>(v);
        ui.qlAmp->setText(QString::fromLatin1("%1").arg(d, 0, 'f', 2));
        Settings->setVoipiMinLoudness(20000 - ui.qsAmp->value());
}

void AudioInputConfig::on_qcbEchoCancel_clicked() {
    Settings->setVoipEchoCancel(ui.qcbEchoCancel->isChecked());
}


void AudioInputConfig::on_qcbTransmit_currentIndexChanged(int v) {
	switch (v) {
		case 0:
                        ui.qswTransmit->setCurrentWidget(ui.qwContinuous);
			break;
		case 1:
                        ui.qswTransmit->setCurrentWidget(ui.qwVAD);
			break;
		case 2:
                        ui.qswTransmit->setCurrentWidget(ui.qwPTT);
			break;
	}
        if (loaded)
            Settings->setVoipATransmit(static_cast<RshareSettings::enumAudioTransmit>(ui.qcbTransmit->currentIndex() + 1));
}


void AudioInputConfig::on_Tick_timeout() {
        if (!inputProcessor) {
            inputProcessor = new QtSpeex::SpeexInputProcessor();
            inputProcessor->open(QIODevice::WriteOnly | QIODevice::Unbuffered);

            if (!inputDevice) {
                inputDevice = AudioDeviceHelper::getPreferedInputDevice();
            }
            inputDevice->start(inputProcessor);
            connect(inputProcessor, SIGNAL(networkPacketReady()), this, SLOT(emptyBuffer()));
        }

        abSpeech->iBelow = ui.qsTransmitMin->value();
        abSpeech->iAbove = ui.qsTransmitMax->value();
        if (loaded) {
            Settings->setVoipfVADmin(ui.qsTransmitMin->value());
            Settings->setVoipfVADmax(ui.qsTransmitMax->value());
        }

        abSpeech->iValue = iroundf(inputProcessor->dVoiceAcivityLevel * 32767.0f + 0.5f);

        abSpeech->update();
}

void AudioInputConfig::emptyBuffer() {
    while(inputProcessor->hasPendingPackets()) {
        inputProcessor->getNetworkPacket(); //that will purge the buffer
    }
}

void AudioInputConfig::on_qpbAudioWizard_clicked() {
    AudioWizard *aw = new AudioWizard(this);
    aw->exec();
    delete aw;
    loadSettings();
}
