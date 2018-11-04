/*******************************************************************************
 * plugins/VOIP/gui/AudioInputConfig.h                                         *
 *                                                                             *
 * Copyright (C) 2008, Andreas Messer <andi@bupfen.de>                         *
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

#include "AudioStats.h"
#include "AudioInputConfig.h"
#include "audiodevicehelper.h"
#include "AudioWizard.h"
#include "gui/VideoProcessor.h"
#include "gui/common/RSGraphWidget.h"
#include "util/RsProtectedTimer.h"

#include <interface/rsVOIP.h>

#define iroundf(x) ( static_cast<int>(x) )

class voipGraphSource: public RSGraphSource
{
public:
    voipGraphSource() : video_input(NULL) {}
    
    void setVideoInput(const QVideoInputDevice *vid) { video_input = vid ; }
    
     virtual QString displayName(int) const { return tr("Required bandwidth") ;}
    
    virtual QString displayValue(float v) const
    {
        if(v < 1000)
            return QString::number(v,10,2) + " B/s" ;
        else if(v < 1000*1024)
            return QString::number(v/1024,10,2) + " KB/s" ;
        else
            return QString::number(v/(1024*1024),10,2) + " MB/s" ;
    }
    
    virtual void getValues(std::map<std::string,float>& vals) const
	 {
        vals.clear() ;
        
	if(video_input)
		vals[std::string("bw")] = video_input->currentBandwidth() ;
    }
    
private:
    const QVideoInputDevice *video_input ;
};

void voipGraph::setVoipSource(voipGraphSource *gs) 
{
_src = gs ;
RSGraphWidget::setSource(gs) ;
}

voipGraph::voipGraph(QWidget *parent)
	: RSGraphWidget(parent)
{
     setFlags(RSGraphWidget::RSGRAPH_FLAGS_SHOW_LEGEND) ;
     setFlags(RSGraphWidget::RSGRAPH_FLAGS_PAINT_STYLE_PLAIN) ;
     
     _src = NULL ;
}

/** Constructor */
AudioInputConfig::AudioInputConfig(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags)
{
    std::cerr << "Creating audioInputConfig object" << std::endl;

    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    loaded = false;

    inputAudioProcessor = NULL;
    inputAudioDevice = NULL;
    abSpeech = NULL;
    qtTick = NULL;

    // Create the video pipeline.
    //
    videoInput = new QVideoInputDevice(this) ;
    videoInput->setEchoVideoTarget(ui.videoDisplay) ;

    videoProcessor = new VideoProcessor() ;
    videoProcessor->setDisplayTarget(NULL) ;

    videoProcessor->setMaximumBandwidth(ui.availableBW_SB->value()) ;
                                        
    videoInput->setVideoProcessor(videoProcessor) ;

    graph_source = new voipGraphSource ;
    ui.voipBwGraph->setSource(graph_source);

    graph_source->setVideoInput(videoInput) ;
    graph_source->setCollectionTimeLimit(1000*300) ;
    graph_source->start() ;

    QObject::connect(ui.showEncoded_CB,SIGNAL(toggled(bool)),this,SLOT(togglePreview(bool))) ;
    QObject::connect(ui.availableBW_SB,SIGNAL(valueChanged(double)),this,SLOT(updateAvailableBW(double))) ;
}

void AudioInputConfig::updateAvailableBW(double r)
{
    std::cerr << "Setting max bandwidth to " << r << " KB/s" << std::endl;
    videoProcessor->setMaximumBandwidth((uint32_t)(r*1024)) ;
}

void AudioInputConfig::togglePreview(bool b)
{
    if(b)
    {
        videoInput->setEchoVideoTarget(NULL) ;
        videoProcessor->setDisplayTarget(ui.videoDisplay) ;
    }
    else
    {
        videoProcessor->setDisplayTarget(NULL) ;
        videoInput->setEchoVideoTarget(ui.videoDisplay) ;
    }
}

AudioInputConfig::~AudioInputConfig()
{
    disconnect( qtTick, SIGNAL( timeout ( ) ), this, SLOT( on_Tick_timeout() ) );
    
        graph_source->stop() ;
        graph_source->setVideoInput(NULL) ;
        
	std::cerr << "Deleting audioInputConfig object" << std::endl;
	if(videoInput != NULL)
	{
		videoInput->stop() ;
		delete videoInput ;
	}

    if (inputAudioDevice) {
        inputAudioDevice->stop();
		  delete inputAudioDevice ;
		  inputAudioDevice = NULL ;
    }

	 if(inputAudioProcessor)
	 {
		 delete inputAudioProcessor ;
		 inputAudioProcessor = NULL ;
	 }
}

/** Loads the settings for this page */
void AudioInputConfig::load()
{
    //connect( ui.allowIpDeterminationCB, SIGNAL( toggled( bool ) ), this, SLOT( toggleIpDetermination(bool) ) );
    //connect( ui.allowTunnelConnectionCB, SIGNAL( toggled( bool ) ), this, SLOT( toggleTunnelConnection(bool) ) );

    qtTick = new RsProtectedTimer(this);
    connect( qtTick, SIGNAL( timeout ( ) ), this, SLOT( on_Tick_timeout() ) );
    qtTick->start(20);
    /*if (AudioInputRegistrar::qmNew) {
            QList<QString> keys = AudioInputRegistrar::qmNew->keys();
            foreach(QString key, keys) {
                    qcbSystem->addItem(key);
            }
    }
    qcbSystem->setEnabled(qcbSystem->count() > 1);*/

    ui.qcbTransmit->addItem(tr("Continuous"), RsVOIP::AudioTransmitContinous);
    ui.qcbTransmit->addItem(tr("Voice Activity"), RsVOIP::AudioTransmitVAD);
    ui.qcbTransmit->addItem(tr("Push To Talk"), RsVOIP::AudioTransmitPushToTalk);

    abSpeech = new AudioBar();
    abSpeech->qcBelow = Qt::red;
    abSpeech->qcInside = Qt::yellow;
    abSpeech->qcAbove = Qt::green;
    //abSpeech->setGeometry(9,20,50,10);
    ui.qwVadLayout_2->addWidget(abSpeech,0,0,1,0);

    //on_qcbPushClick_clicked(g.s.bPushClick);
    //ui.on_Tick_timeout();
    loadSettings();
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
        ui.qcbTransmit->setCurrentIndex(rsVOIP->getVoipATransmit());
        on_qcbTransmit_currentIndexChanged(rsVOIP->getVoipATransmit());
        ui.qsTransmitHold->setValue(rsVOIP->getVoipVoiceHold());
        on_qsTransmitHold_valueChanged(rsVOIP->getVoipVoiceHold());
        ui.qsTransmitMin->setValue(rsVOIP->getVoipfVADmin());
        ui.qsTransmitMax->setValue(rsVOIP->getVoipfVADmax());
        ui.qcbEchoCancel->setChecked(rsVOIP->getVoipEchoCancel());
        //ui.qsDoublePush->setValue(iroundf(static_cast<float>(r.uiDoublePush) / 1000.f + 0.5f));

        //loadCheckBox(qcbPushClick, r.bPushClick);
        //loadSlider(qsQuality, r.iQuality);
        if (rsVOIP->getVoipiNoiseSuppress() != 0)
                ui.qsNoise->setValue(-rsVOIP->getVoipiNoiseSuppress());
	else
                ui.qsNoise->setValue(14);

        on_qsNoise_valueChanged(-rsVOIP->getVoipiNoiseSuppress());

        ui.qsAmp->setValue(20000 - rsVOIP->getVoipiMinLoudness());
        on_qsAmp_valueChanged(20000 - rsVOIP->getVoipiMinLoudness());
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

		  std::cerr << "AudioInputConfig:: starting video." << std::endl;
		  videoInput->start() ;
}

bool AudioInputConfig::save(QString &/*errmsg*/) {//mainly useless beacause saving occurs in realtime
        //s.iQuality = qsQuality->value();
        rsVOIP->setVoipiNoiseSuppress((ui.qsNoise->value() == 14) ? 0 : - ui.qsNoise->value());
        rsVOIP->setVoipiMinLoudness(20000 - ui.qsAmp->value());
        rsVOIP->setVoipVoiceHold(ui.qsTransmitHold->value());
        rsVOIP->setVoipfVADmin(ui.qsTransmitMin->value());
        rsVOIP->setVoipfVADmax(ui.qsTransmitMax->value());
        /*s.uiDoublePush = qsDoublePush->value() * 1000;*/
        rsVOIP->setVoipATransmit(static_cast<RsVOIP::enumAudioTransmit>(ui.qcbTransmit->currentIndex() ));
        rsVOIP->setVoipEchoCancel(ui.qcbEchoCancel->isChecked());

        return true;
}

void AudioInputConfig::on_qsTransmitHold_valueChanged(int v) {
        float val = static_cast<float>(v * FRAME_SIZE);
        val = val / SAMPLING_RATE;
        ui.qlTransmitHold->setText(tr("%1 s").arg(val, 0, 'f', 2));
        rsVOIP->setVoipVoiceHold(v);
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
        rsVOIP->setVoipiNoiseSuppress(- ui.qsNoise->value());
}

void AudioInputConfig::on_qsAmp_valueChanged(int v) {
        v = 20000 - v;
	float d = 20000.0f/static_cast<float>(v);
        ui.qlAmp->setText(QString::fromLatin1("%1").arg(d, 0, 'f', 2));
        rsVOIP->setVoipiMinLoudness(20000 - ui.qsAmp->value());
}

void AudioInputConfig::on_qcbEchoCancel_clicked() {
    rsVOIP->setVoipEchoCancel(ui.qcbEchoCancel->isChecked());
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
            rsVOIP->setVoipATransmit(static_cast<RsVOIP::enumAudioTransmit>(ui.qcbTransmit->currentIndex() ));
}


void AudioInputConfig::on_Tick_timeout() 
{
        if (!inputAudioProcessor) 
        {
            inputAudioProcessor = new QtSpeex::SpeexInputProcessor();
            inputAudioProcessor->open(QIODevice::WriteOnly | QIODevice::Unbuffered);

            if (!inputAudioDevice) {
                inputAudioDevice = AudioDeviceHelper::getPreferedInputDevice();
            }
            inputAudioDevice->start(inputAudioProcessor);
            connect(inputAudioProcessor, SIGNAL(networkPacketReady()), this, SLOT(emptyBuffer()));
        }

        abSpeech->iBelow = ui.qsTransmitMin->value();
        abSpeech->iAbove = ui.qsTransmitMax->value();
        if (loaded) {
            rsVOIP->setVoipfVADmin(ui.qsTransmitMin->value());
            rsVOIP->setVoipfVADmax(ui.qsTransmitMax->value());
        }

        abSpeech->iValue = iroundf(inputAudioProcessor->dVoiceAcivityLevel * 32767.0f + 0.5f);

        abSpeech->update();
        
        // also transmit encoded video
    RsVOIPDataChunk chunk ;
    
    while((!videoInput->stopped()) && videoInput->getNextEncodedPacket(chunk))
    {
        videoProcessor->receiveEncodedData(chunk) ;
        chunk.clear() ;
    }
}

void AudioInputConfig::emptyBuffer() {
    while(inputAudioProcessor->hasPendingPackets()) {
        inputAudioProcessor->getNetworkPacket(); //that will purge the buffer
    }
}

void AudioInputConfig::on_qpbAudioWizard_clicked() {
    AudioWizard aw(this);
    aw.exec();
    loadSettings();
}
