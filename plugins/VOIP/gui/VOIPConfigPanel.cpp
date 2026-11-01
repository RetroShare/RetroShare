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

#include "AudioStats.h"
#include "VOIPConfigPanel.h"
#include "audiodevicehelper.h"
#include "AudioWizard.h"
#include "QVideoDevice.h"
#include "gui/VideoProcessor.h"
#include "gui/common/RSComboBox.h"
#include "util/misc.h"
#include "util/RsProtectedTimer.h"
#include "VOIPTrace.h"

#include <QFormLayout>

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
VOIPConfigPanel::VOIPConfigPanel(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags)
{
    std::cerr << "Creating audioInputConfig object" << std::endl;

    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    loaded = false;

    inputAudioProcessor = NULL;
    inputAudioDevice = NULL;
    graph_source = nullptr;
    videoInput = nullptr;
    videoProcessor = nullptr;
    qtTick = NULL;

    ui.qcbTransmit->addItem(tr("Continuous"), RsVOIP::AudioTransmitContinous);
    ui.qcbTransmit->addItem(tr("Voice Activity"), RsVOIP::AudioTransmitVAD);
    ui.qcbTransmit->addItem(tr("Push To Talk"), RsVOIP::AudioTransmitPushToTalk);

    ui.abSpeech->qcBelow = Qt::red;
    ui.abSpeech->qcInside = Qt::yellow;
    ui.abSpeech->qcAbove = Qt::green;

    ui.qpbAudioWizard->setText(tr("Audio && Video Wizard..."));
    ui.qpbAudioWizard->setToolTip(tr("Step by step selection and tuning of microphone, speakers and camera"));
    ui.label_3->setText(tr("Camera:"));

    // Audio device selection, added on top of the audio group box.
    QFormLayout *audioDevLayout = new QFormLayout;
    qcbAudioInput = new RSComboBox(ui.qgbAudio);
    qcbAudioOutput = new RSComboBox(ui.qgbAudio);
    qcbAudioInput->setToolTip(tr("Microphone used for calls"));
    qcbAudioOutput->setToolTip(tr("Speakers or headphones used for calls"));
    audioDevLayout->addRow(tr("Microphone:"), qcbAudioInput);
    audioDevLayout->addRow(tr("Speakers:"), qcbAudioOutput);
    ui.verticalLayout->insertLayout(0, audioDevLayout);
    fillAudioCombos();

    fillCameraCombo(QString::fromStdString(rsVOIP->getVoipVideoInputDevice()));

    ui.availableBW_SB->setValue(rsVOIP->getVoipVideoMaxBandwidth() / 1024.0);

    connect( ui.qsTransmitHold, SIGNAL( valueChanged ( int ) ), this, SLOT( on_qsTransmitHold_valueChanged(int) ) );
    connect( ui.qsNoise, SIGNAL( valueChanged ( int ) ), this, SLOT( on_qsNoise_valueChanged(int) ) );
    connect( ui.qsAmp, SIGNAL( valueChanged ( int ) ), this, SLOT( on_qsAmp_valueChanged(int) ) );
    connect( ui.qcbTransmit, SIGNAL( currentIndexChanged ( int ) ), this, SLOT( on_qcbTransmit_currentIndexChanged(int) ) );
    connect( ui.inputDevice_CB, SIGNAL( currentIndexChanged ( int ) ), this, SLOT( on_changedCurrentInputDevice(int) ) );
    connect( qcbAudioInput, SIGNAL( currentIndexChanged ( int ) ), this, SLOT( audioInputDeviceChanged(int) ) );
    connect( qcbAudioOutput, SIGNAL( currentIndexChanged ( int ) ), this, SLOT( audioOutputDeviceChanged(int) ) );
    QObject::connect(ui.showEncoded_CB,SIGNAL(toggled(bool)),this,SLOT(togglePreview(bool))) ;
    QObject::connect(ui.availableBW_SB,SIGNAL(valueChanged(double)),this,SLOT(updateAvailableBW(double))) ;
}

void VOIPConfigPanel::fillAudioCombos()
{
    QString in  = QString::fromStdString(rsVOIP->getVoipAudioInputDevice());
    QString out = QString::fromStdString(rsVOIP->getVoipAudioOutputDevice());

    for(int pass=0;pass<2;++pass)
    {
        bool input = (pass == 0);
        RSComboBox *cb = input ? qcbAudioInput : qcbAudioOutput;
        const QString& selected = input ? in : out;

        whileBlocking(cb)->clear();

        RsAudioDevice def = input ? AudioDeviceHelper::inputDeviceInfo(QString()) : AudioDeviceHelper::outputDeviceInfo(QString());
        whileBlocking(cb)->addItem(def.isNull() ? tr("System default") : tr("System default (%1)").arg(AudioDeviceHelper::deviceLabel(def)), QString());

        const QList<RsAudioDevice> devs = input ? AudioDeviceHelper::availableInputDevices() : AudioDeviceHelper::availableOutputDevices();
        for(const RsAudioDevice& dev : devs)
            whileBlocking(cb)->addItem(AudioDeviceHelper::deviceLabel(dev), AudioDeviceHelper::deviceKey(dev));

        int idx = cb->findData(selected);
        if(idx < 0 && !selected.isEmpty())
        {
            whileBlocking(cb)->addItem(tr("%1 (not connected)").arg(selected), selected);
            idx = cb->count()-1;
        }
        whileBlocking(cb)->setCurrentIndex(idx < 0 ? 0 : idx);
    }
}

void VOIPConfigPanel::fillCameraCombo(const QString& selected)
{
    QList<QPair<QString,QString> > cams;
    QVideoInputDevice::getAvailableDevices(cams);

    whileBlocking(ui.inputDevice_CB)->clear();

    if(cams.empty())
    {
        whileBlocking(ui.inputDevice_CB)->addItem(tr("[No camera found]"), QString());
        ui.inputDevice_CB->setEnabled(false);
        return;
    }
    ui.inputDevice_CB->setEnabled(true);

    const QString def = QVideoInputDevice::defaultCameraLabel();
    whileBlocking(ui.inputDevice_CB)->addItem(def.isEmpty() ? tr("System default camera") : tr("System default camera (%1)").arg(def), QString());

    for(auto& c:cams)
    {
        whileBlocking(ui.inputDevice_CB)->addItem(c.second, c.first);
        whileBlocking(ui.inputDevice_CB)->setItemData(ui.inputDevice_CB->count()-1, c.second, Qt::ToolTipRole);
    }

    int idx = ui.inputDevice_CB->findData(selected);
    whileBlocking(ui.inputDevice_CB)->setCurrentIndex(idx < 0 ? 0 : idx);
}

void VOIPConfigPanel::startAudioInput()
{
    stopAudioInput();

    inputAudioProcessor = new QtSpeex::SpeexInputProcessor();
    inputAudioProcessor->open(QIODevice::WriteOnly | QIODevice::Unbuffered);

    inputAudioDevice = AudioDeviceHelper::createInputDevice(qcbAudioInput->currentData().toString());
    inputAudioDevice->start(inputAudioProcessor);

    connect(inputAudioProcessor, SIGNAL(networkPacketReady()), this, SLOT(emptyBuffer()));
}

void VOIPConfigPanel::stopAudioInput()
{
    if (inputAudioDevice) {
        inputAudioDevice->stop();
        delete inputAudioDevice ;
        inputAudioDevice = nullptr ;
    }
    if(inputAudioProcessor)
    {
        inputAudioProcessor->disconnect();
        delete inputAudioProcessor ;
        inputAudioProcessor = nullptr ;
    }
}

void VOIPConfigPanel::showEvent(QShowEvent *)
{
    createPipeline();
}

void VOIPConfigPanel::createPipeline()
{
    if(videoInput != nullptr)	// already running
        return;

    std::cerr << "Creating the audio pipeline" << std::endl;
    startAudioInput();

    std::cerr << "Creating the video pipeline" << std::endl;

    videoInput = new QVideoInputDevice(this) ;

    videoProcessor = new VideoProcessor() ;
    videoProcessor->setDisplayTarget(NULL) ;
    videoProcessor->setMaximumBandwidth((uint32_t)(ui.availableBW_SB->value()*1024)) ;

    videoInput->setVideoProcessor(videoProcessor) ;

    graph_source = new voipGraphSource ;
    ui.voipBwGraph->setSource(graph_source);

    graph_source->setVideoInput(videoInput) ;
    graph_source->setCollectionTimeLimit(1000*300) ;
    graph_source->start() ;

    togglePreview(ui.showEncoded_CB->isChecked());

    loadSettings();

    qtTick = new RsProtectedTimer(this);
    connect( qtTick, SIGNAL( timeout ( ) ), this, SLOT( on_Tick_timeout() ) );
    qtTick->start(20);

    startVideoPreview();
}

void VOIPConfigPanel::startVideoPreview()
{
    if(!videoInput)
        return;

    videoInput->stop();

    if(!ui.inputDevice_CB->isEnabled())	// no camera
        return;

    const QString cam = ui.inputDevice_CB->currentData().toString();
    videoInput->start(cam.isEmpty() ? QString() : cam);	// null string = default camera
}

void VOIPConfigPanel::hideEvent(QHideEvent *)
{
    std::cerr << "Deleting the video pipeline" << std::endl;

    clearPipeline();
}

void VOIPConfigPanel::updateAvailableBW(double r)
{
    std::cerr << "Setting max bandwidth to " << r << " KB/s" << std::endl;
    if(videoProcessor)
        videoProcessor->setMaximumBandwidth((uint32_t)(r*1024)) ;

    if(loaded)
        rsVOIP->setVoipVideoMaxBandwidth((int)(r*1024));
}

void VOIPConfigPanel::togglePreview(bool b)
{
    if(!videoInput || !videoProcessor)
        return;

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

VOIPConfigPanel::~VOIPConfigPanel()
{
    clearPipeline();
}

void VOIPConfigPanel::clearPipeline()
{
    if (qtTick) {
        delete qtTick;
        qtTick = nullptr;
    }

    if (graph_source) {
        graph_source->stop() ;
        graph_source->setVideoInput(NULL) ;
        graph_source=nullptr; // is deleted by setSource below. This is a bad design.
    }

    // Never give a NULL source to the graph: RSGraphWidget::paintLegend() dereferences it
    // without checking, so painting the (still visible) page with a NULL source crashes
    // RetroShare. That is what froze RetroShare when opening the wizard. An empty source is safe.
    ui.voipBwGraph->setSource(new voipGraphSource);

	VOIP_TRACE("panel: clearPipeline video");
	if(videoInput != NULL)
	{
		videoInput->stop() ;
		delete videoInput ;

        videoInput = nullptr;
	}
    if (videoProcessor) {
        delete videoProcessor;
        videoProcessor = nullptr;
    }

    VOIP_TRACE("panel: clearPipeline audio");
    stopAudioInput();
    VOIP_TRACE("panel: clearPipeline done");
}

void VOIPConfigPanel::load()
{
}

/** Loads the settings for this page */

void VOIPConfigPanel::loadSettings()
{
    loaded = false;

    ui.qcbTransmit->setCurrentIndex(rsVOIP->getVoipATransmit());
    on_qcbTransmit_currentIndexChanged(rsVOIP->getVoipATransmit());
    ui.qsTransmitHold->setValue(rsVOIP->getVoipVoiceHold());
    on_qsTransmitHold_valueChanged(rsVOIP->getVoipVoiceHold());
    ui.qsTransmitMin->setValue(rsVOIP->getVoipfVADmin());
    ui.qsTransmitMax->setValue(rsVOIP->getVoipfVADmax());
    ui.qcbEchoCancel->setChecked(rsVOIP->getVoipEchoCancel());

    int noise = rsVOIP->getVoipiNoiseSuppress();
    if (noise != 0)
        ui.qsNoise->setValue(-noise);
    else
        ui.qsNoise->setValue(14);

    on_qsNoise_valueChanged(ui.qsNoise->value());

    ui.qsAmp->setValue(20000 - rsVOIP->getVoipiMinLoudness());
    on_qsAmp_valueChanged(20000 - rsVOIP->getVoipiMinLoudness());

    whileBlocking(ui.availableBW_SB)->setValue(rsVOIP->getVoipVideoMaxBandwidth() / 1024.0);

    loaded = true;
}

bool VOIPConfigPanel::save(QString &/*errmsg*/)
{
    //mainly useless beacause saving occurs in realtime
    rsVOIP->setVoipiNoiseSuppress((ui.qsNoise->value() == 14) ? 0 : - ui.qsNoise->value());
    rsVOIP->setVoipiMinLoudness(20000 - ui.qsAmp->value());
    rsVOIP->setVoipVoiceHold(ui.qsTransmitHold->value());
    rsVOIP->setVoipfVADmin(ui.qsTransmitMin->value());
    rsVOIP->setVoipfVADmax(ui.qsTransmitMax->value());
    rsVOIP->setVoipATransmit(static_cast<RsVOIP::enumAudioTransmit>(ui.qcbTransmit->currentIndex() ));
    rsVOIP->setVoipEchoCancel(ui.qcbEchoCancel->isChecked());
    rsVOIP->setVoipAudioInputDevice(qcbAudioInput->currentData().toString().toStdString());
    rsVOIP->setVoipAudioOutputDevice(qcbAudioOutput->currentData().toString().toStdString());
    if(ui.inputDevice_CB->isEnabled())
        rsVOIP->setVoipVideoInputDevice(ui.inputDevice_CB->currentData().toString().toStdString());
    rsVOIP->setVoipVideoMaxBandwidth((int)(ui.availableBW_SB->value()*1024));

    return true;
}

void VOIPConfigPanel::on_qsTransmitHold_valueChanged(int v) {
        float val = static_cast<float>(v * FRAME_SIZE);
        val = val / SAMPLING_RATE;
        ui.qlTransmitHold->setText(tr("%1 s").arg(val, 0, 'f', 2));
        if (loaded)
            rsVOIP->setVoipVoiceHold(v);
}

void VOIPConfigPanel::on_qsNoise_valueChanged(int v) {
	QPalette pal;

	if (v < 15) {
                ui.qlNoise->setText(tr("Off"));
                pal.setColor(ui.qlNoise->foregroundRole(), Qt::red);
	} else {
                ui.qlNoise->setText(tr("-%1 dB").arg(v));
	}
        ui.qlNoise->setPalette(pal);
        if (loaded)
            rsVOIP->setVoipiNoiseSuppress((v < 15) ? 0 : - v);
}

void VOIPConfigPanel::on_qsAmp_valueChanged(int v) {
        v = 20000 - v;
        if (v <= 0) v = 1;
	float d = 20000.0f/static_cast<float>(v);
        ui.qlAmp->setText(QString::fromLatin1("%1").arg(d, 0, 'f', 2));
        if (loaded)
            rsVOIP->setVoipiMinLoudness(20000 - ui.qsAmp->value());
}

void VOIPConfigPanel::on_qcbEchoCancel_clicked() {
    rsVOIP->setVoipEchoCancel(ui.qcbEchoCancel->isChecked());
}


void VOIPConfigPanel::on_qcbTransmit_currentIndexChanged(int v) {
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


void VOIPConfigPanel::on_Tick_timeout()
{
    if(!inputAudioProcessor || !videoInput)
        return;

    // update the sound capture bar

    ui.abSpeech->iBelow = ui.qsTransmitMin->value();
    ui.abSpeech->iAbove = ui.qsTransmitMax->value();

    if (loaded) {
        if (rsVOIP->getVoipfVADmin() != ui.qsTransmitMin->value())
            rsVOIP->setVoipfVADmin(ui.qsTransmitMin->value());
        if (rsVOIP->getVoipfVADmax() != ui.qsTransmitMax->value())
            rsVOIP->setVoipfVADmax(ui.qsTransmitMax->value());
    }

    ui.abSpeech->iValue = iroundf(inputAudioProcessor->dVoiceAcivityLevel * 32767.0f + 0.5f);
    ui.abSpeech->update();

    // also transmit encoded video

    RsVOIPDataChunk chunk ;

    while((!videoInput->stopped()) && videoInput->getNextEncodedPacket(chunk))
    {
        videoProcessor->receiveEncodedData(chunk) ;
        chunk.clear() ;
    }
}

void VOIPConfigPanel::emptyBuffer() {
    while(inputAudioProcessor && inputAudioProcessor->hasPendingPackets()) {
        inputAudioProcessor->getNetworkPacket(); //that will purge the buffer
    }
}

void VOIPConfigPanel::on_qpbAudioWizard_clicked()
{
    // The wizard needs the microphone and the camera for itself: some systems (e.g. Windows)
    // do not allow two applications/objects to open the same camera.
    VOIP_TRACE("panel: wizard button clicked, stopping preview");
    clearPipeline();

    {
        VOIP_TRACE("panel: creating wizard");
        AudioWizard aw(this);
        VOIP_TRACE("panel: wizard exec()");
        aw.exec();
        VOIP_TRACE("panel: wizard closed");
    }

    fillAudioCombos();
    fillCameraCombo(QString::fromStdString(rsVOIP->getVoipVideoInputDevice()));
    loadSettings();

    if(isVisible())
        createPipeline();
}

void VOIPConfigPanel::audioInputDeviceChanged(int)
{
    rsVOIP->setVoipAudioInputDevice(qcbAudioInput->currentData().toString().toStdString());

    if(inputAudioProcessor)	// pipeline running: restart the level meter on the new device
        startAudioInput();
}

void VOIPConfigPanel::audioOutputDeviceChanged(int)
{
    rsVOIP->setVoipAudioOutputDevice(qcbAudioOutput->currentData().toString().toStdString());
}

void VOIPConfigPanel::on_changedCurrentInputDevice(int)
{
    QString s = ui.inputDevice_CB->currentData().toString();

    // check that the camera still exists (empty string = default camera)

    if(!s.isEmpty() && !QVideoInputDevice::cameraExists(s))
    {
        checkAvailableCameras();
        return;
    }

    std::cerr << "Switching to camera \"" << s.toStdString() << "\"" << std::endl;
    rsVOIP->setVoipVideoInputDevice(s.toStdString());

    startVideoPreview();
}

void VOIPConfigPanel::checkAvailableCameras()
{
    fillCameraCombo(ui.inputDevice_CB->currentData().toString());
    startVideoPreview();
}
