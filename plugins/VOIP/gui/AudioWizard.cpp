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

#include <QBuffer>
#include <QComboBox>
#include <QSignalBlocker>
#include <QTimer>

#include <cmath>
#include <iostream>

#include "AudioWizard.h"
#include "audiodevicehelper.h"
#include "QVideoDevice.h"
#include "VideoProcessor.h"
#include "VOIPTrace.h"
#include "interface/rsVOIP.h"

#define iroundf(x) ( static_cast<int>(x) )

static const qint64 LOCAL_ECHO_DELAY_MS = 1500 ;	// delay before your own voice is played back

static QString errorText(const QString& s) { return QString("<font color=\"#c00000\">%1</font>").arg(s) ; }

// =====================================================================================
//                                    Construction
// =====================================================================================

AudioWizard::AudioWizard(QWidget *p)
    : QDialog(p),
      mAudioDevicesFilled(false), mCamerasFilled(false),
      inputDevice(NULL), outputDevice(NULL), inputProcessor(NULL), outputProcessor(NULL),
      testOutput(NULL), testBuffer(NULL),
      videoInput(NULL), videoProcessor(NULL), videoNoFrameWarned(false),
      ticker(NULL), bInit(true), bLastActive(false), iMaxPeak(0), iTicks(0), iHeartbeats(0)
{
	VOIP_TRACE("wizard: constructor begin");

	setupUi(this) ;

	// Save current settings so that "Cancel" can restore them.
	mSaved.atransmit      = rsVOIP->getVoipATransmit() ;
	mSaved.voiceHold      = rsVOIP->getVoipVoiceHold() ;
	mSaved.vadMin         = rsVOIP->getVoipfVADmin() ;
	mSaved.vadMax         = rsVOIP->getVoipfVADmax() ;
	mSaved.noiseSuppress  = rsVOIP->getVoipiNoiseSuppress() ;
	mSaved.minLoudness    = rsVOIP->getVoipiMinLoudness() ;
	mSaved.echoCancel     = rsVOIP->getVoipEchoCancel() ;
	mSaved.audioIn        = rsVOIP->getVoipAudioInputDevice() ;
	mSaved.audioOut       = rsVOIP->getVoipAudioOutputDevice() ;
	mSaved.videoIn        = rsVOIP->getVoipVideoInputDevice() ;
	mSaved.videoBandwidth = rsVOIP->getVoipVideoMaxBandwidth() ;

	qpTalkingOn  = QPixmap::fromImage(QImage(QLatin1String(":/images/talking_on.svg")).scaled(64,64)) ;
	qpTalkingOff = QPixmap::fromImage(QImage(QLatin1String(":/images/talking_off.svg")).scaled(64,64)) ;

	// ---- audio devices page (the lists are filled when the page is shown) ----
	abDeviceLevel->qcBelow  = Qt::green ;
	abDeviceLevel->qcInside = QColor::fromRgb(255,128,0) ;
	abDeviceLevel->qcAbove  = Qt::red ;
	abDeviceLevel->iBelow   = 24000 ;
	abDeviceLevel->iAbove   = 31000 ;
	qcbEcho->setChecked(mSaved.echoCancel) ;

	// ---- volume page ----
	abAmplify->qcBelow  = Qt::green ;
	abAmplify->qcInside = QColor::fromRgb(255,128,0) ;
	abAmplify->qcAbove  = Qt::red ;
	qsMaxAmp->setValue(qBound(1000, mSaved.minLoudness, 30000)) ;

	// ---- transmission page ----
	abVAD->qcBelow  = Qt::red ;
	abVAD->qcInside = Qt::yellow ;
	abVAD->qcAbove  = Qt::green ;
	qsTransmitMin->setValue(mSaved.vadMin) ;
	qsTransmitMax->setValue(mSaved.vadMax) ;
	qsVoiceHold->setValue(qBound(20, mSaved.voiceHold, 250)) ;
	qsNoise->setValue(mSaved.noiseSuppress == 0 ? 14 : qBound(14, -mSaved.noiseSuppress, 60)) ;
	qlTalkIcon->setPixmap(qpTalkingOff) ;

	if (mSaved.atransmit == RsVOIP::AudioTransmitContinous)
		qrContinuous->setChecked(true) ;
	else
		qrVAD->setChecked(true) ;
	qlPTTNote->setVisible(mSaved.atransmit == RsVOIP::AudioTransmitPushToTalk) ;

	// ---- video page (the camera list is filled when the page is shown) ----
	qsbBandwidth->setValue(qMax(5, mSaved.videoBandwidth / 1024)) ;

	// ---- connections ----
	connect(qcbInput,        SIGNAL(currentIndexChanged(int)), this, SLOT(onInputDeviceChanged(int))) ;
	connect(qcbOutput,       SIGNAL(currentIndexChanged(int)), this, SLOT(onOutputDeviceChanged(int))) ;
	connect(qpbRefreshAudio, SIGNAL(clicked()),                this, SLOT(refreshAudioDevices())) ;
	connect(qpbTestSound,    SIGNAL(clicked()),                this, SLOT(playTestSound())) ;
	connect(qcbEcho,         SIGNAL(toggled(bool)),            this, SLOT(onEchoCancelToggled(bool))) ;

	connect(qsMaxAmp,        SIGNAL(valueChanged(int)),        this, SLOT(onMaxAmpChanged(int))) ;
	connect(qcbHighContrast, SIGNAL(toggled(bool)),            this, SLOT(onHighContrastToggled(bool))) ;

	connect(qrVAD,           SIGNAL(toggled(bool)),            this, SLOT(onTransmitModeChanged())) ;
	connect(qrContinuous,    SIGNAL(toggled(bool)),            this, SLOT(onTransmitModeChanged())) ;
	connect(qsTransmitMin,   SIGNAL(valueChanged(int)),        this, SLOT(onVADMinChanged(int))) ;
	connect(qsTransmitMax,   SIGNAL(valueChanged(int)),        this, SLOT(onVADMaxChanged(int))) ;
	connect(qsVoiceHold,     SIGNAL(valueChanged(int)),        this, SLOT(onVoiceHoldChanged(int))) ;
	connect(qsNoise,         SIGNAL(valueChanged(int)),        this, SLOT(onNoiseChanged(int))) ;

	connect(qcbCamera,         SIGNAL(currentIndexChanged(int)), this, SLOT(onCameraChanged(int))) ;
	connect(qpbRefreshCameras, SIGNAL(clicked()),                this, SLOT(refreshCameras())) ;
	connect(qcbShowEncoded,    SIGNAL(toggled(bool)),            this, SLOT(onShowEncodedToggled(bool))) ;
	connect(qsbBandwidth,      SIGNAL(valueChanged(double)),     this, SLOT(onBandwidthChanged(double))) ;

	// update the value labels
	onVoiceHoldChanged(qsVoiceHold->value()) ;
	onNoiseChanged(qsNoise->value()) ;
	updateTransmitWidgets() ;

	echoClock.start() ;

	ticker = new QTimer(this) ;
	ticker->setSingleShot(false) ;
	connect(ticker, SIGNAL(timeout()), this, SLOT(onTicker())) ;
	ticker->start(20) ;

	// ---- navigation ----
	connect(qpbBack,   SIGNAL(clicked()), this, SLOT(goBack())) ;
	connect(qpbNext,   SIGNAL(clicked()), this, SLOT(goNext())) ;
	connect(qpbFinish, SIGNAL(clicked()), this, SLOT(accept())) ;
	connect(qpbCancel, SIGNAL(clicked()), this, SLOT(reject())) ;
	connect(qswPages,  SIGNAL(currentChanged(int)), this, SLOT(onPageChanged(int))) ;

	qswPages->setCurrentIndex(PageIntro) ;
	updateHeaderAndButtons() ;

	bInit = false ;
	VOIP_TRACE("wizard: constructor end");
}

AudioWizard::~AudioWizard()
{
	stopEverything() ;
}

void AudioWizard::stopEverything()
{
	if (ticker)
		ticker->stop() ;

	stopTestSound() ;
	stopAudio() ;
	stopVideo() ;
}

// =====================================================================================
//                                  Device lists
// =====================================================================================

void AudioWizard::fillAudioDeviceCombo(QComboBox *cb, bool input, const QString& selected)
{
	QSignalBlocker blocker(cb) ;
	cb->clear() ;

	RsAudioDevice def = input ? AudioDeviceHelper::inputDeviceInfo(QString()) : AudioDeviceHelper::outputDeviceInfo(QString()) ;
	cb->addItem(def.isNull() ? tr("System default") : tr("System default (%1)").arg(AudioDeviceHelper::deviceLabel(def)), QString()) ;

	const QList<RsAudioDevice> list = input ? AudioDeviceHelper::availableInputDevices() : AudioDeviceHelper::availableOutputDevices() ;

	for (const RsAudioDevice& dev : list)
	{
		cb->addItem(AudioDeviceHelper::deviceLabel(dev), AudioDeviceHelper::deviceKey(dev)) ;
		cb->setItemData(cb->count()-1, AudioDeviceHelper::deviceLabel(dev), Qt::ToolTipRole) ;
	}

	int idx = cb->findData(selected) ;
	if (idx < 0 && !selected.isEmpty())
	{
		// keep the saved choice visible even if the device is unplugged right now.
		cb->addItem(tr("%1 (not connected)").arg(selected), selected) ;
		idx = cb->count() - 1 ;
	}
	cb->setCurrentIndex(idx < 0 ? 0 : idx) ;
}

QString AudioWizard::selectedInputDevice() const  { return mAudioDevicesFilled ? qcbInput->currentData().toString()  : QString::fromStdString(mSaved.audioIn) ; }
QString AudioWizard::selectedOutputDevice() const { return mAudioDevicesFilled ? qcbOutput->currentData().toString() : QString::fromStdString(mSaved.audioOut) ; }
QString AudioWizard::selectedCamera() const       { return mCamerasFilled ? qcbCamera->currentData().toString() : QString::fromStdString(mSaved.videoIn) ; }

void AudioWizard::refreshAudioDevices()
{
	VOIP_TRACE("wizard: refreshAudioDevices");
	const QString in  = selectedInputDevice() ;
	const QString out = selectedOutputDevice() ;

	fillAudioDeviceCombo(qcbInput,  true,  in) ;
	fillAudioDeviceCombo(qcbOutput, false, out) ;
	mAudioDevicesFilled = true ;

	if (inputProcessor)
		startAudio() ;
}

void AudioWizard::refreshCameras()
{
	VOIP_TRACE("wizard: refreshCameras");
	const QString current = selectedCamera() ;

	QSignalBlocker blocker(qcbCamera) ;
	qcbCamera->clear() ;
	mCamerasFilled = true ;

	QList<QPair<QString,QString> > cams ;
	QVideoInputDevice::getAvailableDevices(cams) ;

	if (cams.isEmpty())
	{
		qcbCamera->addItem(tr("No camera found"), QString()) ;
		qcbCamera->setEnabled(false) ;
		qlVideoStatus->setText(tr("<b>No camera was found.</b> Plug in a webcam and click <i>Refresh</i>. "
		                          "On Linux, make sure the Qt multimedia plugins are installed.")) ;
		stopVideo() ;
		return ;
	}

	qcbCamera->setEnabled(true) ;

	const QString def = QVideoInputDevice::defaultCameraLabel() ;
	qcbCamera->addItem(def.isEmpty() ? tr("System default camera") : tr("System default camera (%1)").arg(def), QString()) ;

	for (const QPair<QString,QString>& c : cams)
	{
		qcbCamera->addItem(c.second, c.first) ;
		qcbCamera->setItemData(qcbCamera->count()-1, c.second, Qt::ToolTipRole) ;
	}

	int idx = qcbCamera->findData(current) ;
	if (idx < 0 && !current.isEmpty())
	{
		qcbCamera->addItem(tr("%1 (not connected)").arg(current), current) ;
		idx = qcbCamera->count() - 1 ;
	}
	qcbCamera->setCurrentIndex(idx < 0 ? 0 : idx) ;

	if (currentId() == PageVideo)
		startVideo() ;	// restart with the (possibly) new device
}

// =====================================================================================
//                                    Audio engine
// =====================================================================================

void AudioWizard::startAudio()
{
	stopAudio() ;
	VOIP_TRACE("wizard: startAudio in=\"" + selectedInputDevice() + "\" out=\"" + selectedOutputDevice() + "\"");

	const QAudioFormat fmt = AudioDeviceHelper::voipAudioFormat() ;
	QStringList warnings ;

	RsAudioDevice inInfo  = AudioDeviceHelper::inputDeviceInfo(selectedInputDevice()) ;
	RsAudioDevice outInfo = AudioDeviceHelper::outputDeviceInfo(selectedOutputDevice()) ;

	if (inInfo.isNull())
		warnings << tr("No microphone found.") ;
	else if (!inInfo.isFormatSupported(fmt))
		warnings << tr("The microphone \"%1\" does not report support for 16 kHz mono; it may not work.").arg(AudioDeviceHelper::deviceLabel(inInfo)) ;

	if (outInfo.isNull())
		warnings << tr("No speakers / headphones found.") ;
	else if (!outInfo.isFormatSupported(fmt))
		warnings << tr("The output device \"%1\" does not report support for 16 kHz mono; it may not work.").arg(AudioDeviceHelper::deviceLabel(outInfo)) ;

	qlAudioStatus->setText(warnings.isEmpty() ? QString() : errorText(warnings.join("<br/>"))) ;

	inputProcessor = new QtSpeex::SpeexInputProcessor() ;
	inputProcessor->open(QIODevice::WriteOnly | QIODevice::Unbuffered) ;
	connect(inputProcessor, SIGNAL(networkPacketReady()), this, SLOT(loopAudio())) ;

	outputProcessor = new QtSpeex::SpeexOutputProcessor() ;
	outputProcessor->open(QIODevice::ReadOnly | QIODevice::Unbuffered) ;
	connect(outputProcessor, SIGNAL(playingFrame(QByteArray*)), inputProcessor, SLOT(addEchoFrame(QByteArray*))) ;

	inputDevice = AudioDeviceHelper::createInputDevice(selectedInputDevice(), this) ;
	inputDevice->start(inputProcessor) ;

	outputDevice = AudioDeviceHelper::createOutputDevice(selectedOutputDevice(), this) ;
	outputDevice->start(outputProcessor) ;

	iMaxPeak = 0 ;
	iTicks = 0 ;
	VOIP_TRACE("wizard: startAudio done");
}

void AudioWizard::stopAudio()
{
	packetQueue.clear() ;

	if (!inputDevice && !outputDevice && !inputProcessor && !outputProcessor)
		return ;

	VOIP_TRACE("wizard: stopAudio");

	if (inputDevice) {
		inputDevice->stop() ;
		delete inputDevice ;
		inputDevice = NULL ;
	}
	if (outputDevice) {
		outputDevice->stop() ;
		delete outputDevice ;
		outputDevice = NULL ;
	}
	if (outputProcessor) {
		outputProcessor->disconnect() ;
		outputProcessor->close() ;
		delete outputProcessor ;
		outputProcessor = NULL ;
	}
	if (inputProcessor) {
		inputProcessor->disconnect() ;
		inputProcessor->close() ;
		delete inputProcessor ;
		inputProcessor = NULL ;
	}
	VOIP_TRACE("wizard: stopAudio done");
}

void AudioWizard::loopAudio()
{
	while (inputProcessor && inputProcessor->hasPendingPackets())
	{
		QByteArray packet = inputProcessor->getNetworkPacket() ;

		// local echo only on the tuning pages, and if the user wants it.
		if ((currentId() == PageVolume || currentId() == PageTransmit) && !qcbStopEcho->isChecked())
			packetQueue.enqueue(qMakePair(echoClock.elapsed(), packet)) ;
	}
}

void AudioWizard::onInputDeviceChanged(int)
{
	if (!bInit && inputProcessor)
		startAudio() ;
}

void AudioWizard::onOutputDeviceChanged(int)
{
	stopTestSound() ;
	if (!bInit && inputProcessor)
		startAudio() ;
}

void AudioWizard::onEchoCancelToggled(bool b)
{
	rsVOIP->setVoipEchoCancel(b) ;
}

void AudioWizard::playTestSound()
{
	stopTestSound() ;

	const QAudioFormat fmt = AudioDeviceHelper::voipAudioFormat() ;

	// Two short tones (440 Hz then 660 Hz) with a soft attack/release.
	if (testTone.isEmpty())
	{
		const int rate = fmt.sampleRate() ;
		const int noteLen = rate * 4 / 10 ;
		const int fade = rate / 50 ;
		testTone.resize(int(2 * noteLen * sizeof(qint16))) ;
		qint16 *s = reinterpret_cast<qint16*>(testTone.data()) ;

		for (int n = 0; n < 2; ++n)
		{
			const double freq = (n == 0) ? 440.0 : 660.0 ;
			for (int i = 0; i < noteLen; ++i)
			{
				double env = 1.0 ;
				if (i < fade) env = double(i) / fade ;
				else if (i > noteLen - fade) env = double(noteLen - i) / fade ;

				s[n*noteLen + i] = static_cast<qint16>(0.35 * 32767.0 * env * std::sin(2.0 * 3.14159265358979323846 * freq * i / rate)) ;
			}
		}
	}

	testBuffer = new QBuffer(&testTone, this) ;
	testBuffer->open(QIODevice::ReadOnly) ;

	RsAudioDevice info = AudioDeviceHelper::outputDeviceInfo(selectedOutputDevice()) ;
	testOutput = new RsAudioOutput(info, fmt, this) ;
	connect(testOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(onTestSoundStateChanged(QAudio::State))) ;
	testOutput->start(testBuffer) ;

	if (testOutput->error() != QAudio::NoError)
		qlAudioStatus->setText(errorText(tr("Cannot play sound on \"%1\".").arg(AudioDeviceHelper::deviceLabel(info)))) ;
}

void AudioWizard::onTestSoundStateChanged(QAudio::State state)
{
	if (state == QAudio::IdleState || state == QAudio::StoppedState)
		QTimer::singleShot(0, this, SLOT(onTestSoundFinished())) ;
}

void AudioWizard::stopTestSound()
{
	if (testOutput) {
		testOutput->disconnect(this) ;
		testOutput->stop() ;
		testOutput->deleteLater() ;
		testOutput = NULL ;
	}
	if (testBuffer) {
		testBuffer->close() ;
		testBuffer->deleteLater() ;
		testBuffer = NULL ;
	}
}

// =====================================================================================
//                                    Video engine
// =====================================================================================

void AudioWizard::startVideo()
{
	stopVideo() ;

	if (!qcbCamera->isEnabled())
		return ;	// no camera

	VOIP_TRACE("wizard: startVideo \"" + selectedCamera() + "\"");

	videoProcessor = new VideoProcessor() ;
	videoProcessor->setMaximumBandwidth(static_cast<uint32_t>(qsbBandwidth->value() * 1024)) ;

	videoInput = new QVideoInputDevice(this) ;
	videoInput->setVideoProcessor(videoProcessor) ;
	connect(videoInput, SIGNAL(cameraErrorString(QString)), this, SLOT(onCameraError(QString))) ;

	onShowEncodedToggled(qcbShowEncoded->isChecked()) ;

	qlVideoStatus->setText(tr("Starting camera...")) ;
	videoStartTime.start() ;
	videoNoFrameWarned = false ;
	const QString cam = selectedCamera() ;
	videoInput->start(cam.isEmpty() ? QString() : cam) ;	// null string = default camera
	VOIP_TRACE("wizard: startVideo done");
}

void AudioWizard::stopVideo()
{
	if (videoInput) {
		VOIP_TRACE("wizard: stopVideo");
		videoInput->disconnect(this) ;
		videoInput->stop() ;
		delete videoInput ;
		videoInput = NULL ;
		VOIP_TRACE("wizard: stopVideo done");
	}
	if (videoProcessor) {
		delete videoProcessor ;
		videoProcessor = NULL ;
	}
	videoPreview->showFrameOff() ;
}

void AudioWizard::onCameraChanged(int)
{
	if (currentId() == PageVideo)
		startVideo() ;
}

void AudioWizard::onShowEncodedToggled(bool b)
{
	if (!videoInput || !videoProcessor)
		return ;

	if (b) {
		videoInput->setEchoVideoTarget(NULL) ;
		videoProcessor->setDisplayTarget(videoPreview) ;
	} else {
		videoProcessor->setDisplayTarget(NULL) ;
		videoInput->setEchoVideoTarget(videoPreview) ;
	}
}

void AudioWizard::onBandwidthChanged(double kbps)
{
	if (videoProcessor)
		videoProcessor->setMaximumBandwidth(static_cast<uint32_t>(kbps * 1024)) ;
}

void AudioWizard::onCameraError(QString s)
{
	qlVideoStatus->setText(errorText("<b>" + s.toHtmlEscaped() + "</b>")) ;
}

// =====================================================================================
//                                  Page management
// =====================================================================================

int AudioWizard::currentId() const
{
	return qswPages->currentIndex() ;
}

void AudioWizard::goBack()
{
	if (currentId() > 0)
		qswPages->setCurrentIndex(currentId() - 1) ;
}

void AudioWizard::goNext()
{
	if (currentId() < qswPages->count() - 1)
		qswPages->setCurrentIndex(currentId() + 1) ;
}

void AudioWizard::updateHeaderAndButtons()
{
	QWidget *page = qswPages->currentWidget() ;
	if (page) {
		qlPageTitle->setText(page->property("pageTitle").toString()) ;
		qlPageSubTitle->setText(page->property("pageSubTitle").toString()) ;
	}

	const bool last = (currentId() == qswPages->count() - 1) ;

	qpbBack->setEnabled(currentId() > 0) ;
	qpbNext->setVisible(!last) ;
	qpbFinish->setVisible(last) ;
	(last ? qpbFinish : qpbNext)->setDefault(true) ;
}

int AudioWizard::chosenTransmitMode() const
{
	return qrContinuous->isChecked() ? RsVOIP::AudioTransmitContinous : RsVOIP::AudioTransmitVAD ;
}

void AudioWizard::onPageChanged(int id)
{
	VOIP_TRACE(QString("wizard: page %1").arg(id));

	if (id < 0)
		return ;

	updateHeaderAndButtons() ;
	iHeartbeats = 0 ;
	packetQueue.clear() ;

	if (id != PageAudioDevices)
		stopTestSound() ;

	// fill the device lists the first time the pages are shown
	if (id == PageAudioDevices && !mAudioDevicesFilled)
		refreshAudioDevices() ;
	if (id == PageVideo && !mCamerasFilled)
		refreshCameras() ;

	// While tuning the volume we want to hear everything, so transmit continuously.
	// On the transmission page, the selected mode is used so that you hear its effect.
	if (id == PageAudioDevices || id == PageVolume)
		rsVOIP->setVoipATransmit(RsVOIP::AudioTransmitContinous) ;
	else if (id == PageTransmit)
		rsVOIP->setVoipATransmit(chosenTransmitMode()) ;

	const bool needAudio = (id == PageAudioDevices || id == PageVolume || id == PageTransmit) ;
	const bool needVideo = (id == PageVideo) ;

	if (needAudio && !inputProcessor)
		startAudio() ;
	else if (!needAudio)
		stopAudio() ;

	if (needVideo && !videoInput)
		startVideo() ;
	else if (!needVideo)
		stopVideo() ;

	if (id == PageDone)
		updateSummary() ;

	VOIP_TRACE(QString("wizard: page %1 ready").arg(id));
}

void AudioWizard::onTicker()
{
	if (++iTicks >= 50) {
		iTicks = 0 ;
		iMaxPeak = 0 ;

		// debug: shows that the event loop is running (first seconds of each page only)
		if (iHeartbeats < 5)
			VOIP_TRACE(QString("wizard: alive, page %1").arg(currentId())) ;
		++iHeartbeats ;
	}

	// ---- video ----
	if (videoInput && currentId() == PageVideo)
	{
		// feed encoded frames back to the decoder for the "encoded" preview.
		RsVOIPDataChunk chunk ;
		while (!videoInput->stopped() && videoInput->getNextEncodedPacket(chunk))
		{
			if (qcbShowEncoded->isChecked() && videoProcessor)
				videoProcessor->receiveEncodedData(chunk) ;
			chunk.clear() ;
		}

		if (!videoNoFrameWarned && videoInput->framesReceived() == 0 && videoStartTime.isValid() && videoStartTime.elapsed() > 5000)
		{
			videoNoFrameWarned = true ;
			onCameraError(tr("The camera does not deliver any image. It may be used by another application, or access to the camera was not granted.")) ;
		}

		if ((iTicks % 25) == 0 && !videoInput->stopped() && videoInput->framesReceived() > 0)
			qlVideoStatus->setText(tr("Camera <b>%1</b> is working. Video bandwidth: %2 KB/s")
			                       .arg(videoInput->currentCameraHumanName().toHtmlEscaped())
			                       .arg(videoInput->currentBandwidth() / 1024.0, 0, 'f', 1)) ;
	}

	if (!inputProcessor)
		return ;

	// ---- device errors ----
	if (inputDevice && inputDevice->error() == QAudio::OpenError)
		qlAudioStatus->setText(errorText(tr("Cannot open the microphone. It may be used by another application or unplugged."))) ;
	if (outputDevice && outputDevice->error() == QAudio::OpenError)
		qlAudioStatus->setText(errorText(tr("Cannot open the output device. It may be used by another application or unplugged."))) ;

	// ---- delayed local echo ----
	const qint64 now = echoClock.elapsed() ;
	while (!packetQueue.isEmpty() && now - packetQueue.head().first >= LOCAL_ECHO_DELAY_MS)
	{
		QByteArray packet = packetQueue.dequeue().second ;
		if (outputProcessor)
			outputProcessor->putNetworkPacket("myself_loop", packet) ;
	}

	// ---- meters ----
	const int iPeak = iroundf(inputProcessor->dMaxMic) ;
	if (iPeak > iMaxPeak)
		iMaxPeak = iPeak ;

	switch (currentId())
	{
	case PageAudioDevices:
		abDeviceLevel->iValue = iPeak ;
		abDeviceLevel->iPeak  = iMaxPeak ;
		abDeviceLevel->update() ;
		break ;

	case PageVolume:
		abAmplify->iBelow = qsMaxAmp->value() ;
		abAmplify->iValue = iPeak ;
		abAmplify->iPeak  = iMaxPeak ;
		abAmplify->update() ;
		break ;

	case PageTransmit:
	{
		abVAD->iBelow = qsTransmitMin->value() ;
		abVAD->iAbove = qsTransmitMax->value() ;
		abVAD->iValue = iroundf(inputProcessor->dVoiceAcivityLevel * 32767.0f + 0.5f) ;
		abVAD->update() ;

		const bool active = inputProcessor->bPreviousVoice ;
		if (active != bLastActive) {
			bLastActive = active ;
			qlTalkIcon->setPixmap(active ? qpTalkingOn : qpTalkingOff) ;
		}
		break ;
	}
	default:
		break ;
	}
}

// =====================================================================================
//                                 Settings callbacks
// =====================================================================================

void AudioWizard::onMaxAmpChanged(int v)
{
	rsVOIP->setVoipiMinLoudness(qBound(1000, v, 30000)) ;
}

void AudioWizard::onHighContrastToggled(bool on)
{
	abDeviceLevel->highContrast = on ;
	abAmplify->highContrast = on ;
	abVAD->highContrast = on ;
}

void AudioWizard::onTransmitModeChanged()
{
	updateTransmitWidgets() ;
	if (currentId() == PageTransmit)
		rsVOIP->setVoipATransmit(chosenTransmitMode()) ;
}

void AudioWizard::updateTransmitWidgets()
{
	qwVAD->setEnabled(qrVAD->isChecked()) ;
}

void AudioWizard::onVADMinChanged(int v)
{
	if (v > qsTransmitMax->value())
		qsTransmitMax->setValue(v) ;
	rsVOIP->setVoipfVADmin(v) ;
}

void AudioWizard::onVADMaxChanged(int v)
{
	if (v < qsTransmitMin->value())
		qsTransmitMin->setValue(v) ;
	rsVOIP->setVoipfVADmax(v) ;
}

void AudioWizard::onVoiceHoldChanged(int v)
{
	const float secs = static_cast<float>(v * FRAME_SIZE) / SAMPLING_RATE ;
	qlVoiceHold->setText(tr("%1 s").arg(secs, 0, 'f', 2)) ;
	rsVOIP->setVoipVoiceHold(v) ;
}

void AudioWizard::onNoiseChanged(int v)
{
	QPalette pal = qlNoise->palette() ;

	if (v <= 14) {
		qlNoise->setText(tr("Off")) ;
		pal.setColor(QPalette::WindowText, Qt::red) ;
		rsVOIP->setVoipiNoiseSuppress(0) ;
	} else {
		qlNoise->setText(tr("-%1 dB").arg(v)) ;
		pal.setColor(QPalette::WindowText, palette().color(QPalette::WindowText)) ;
		rsVOIP->setVoipiNoiseSuppress(-v) ;
	}
	qlNoise->setPalette(pal) ;
}

void AudioWizard::updateSummary()
{
	const QString mode = qrContinuous->isChecked() ? tr("Continuous") : tr("Voice activity detection") ;

	auto row = [](const QString& k, const QString& v) {
		return QString("<tr><td><b>%1</b></td><td>%2</td></tr>").arg(k, v.toHtmlEscaped()) ;
	} ;

	QString html = "<table cellspacing=\"6\">" ;
	html += row(tr("Microphone:"),        qcbInput->currentText()) ;
	html += row(tr("Speakers:"),          qcbOutput->currentText()) ;
	html += row(tr("Echo cancellation:"), qcbEcho->isChecked() ? tr("On") : tr("Off")) ;
	html += row(tr("Transmission:"),      mode) ;
	html += row(tr("Camera:"),            qcbCamera->currentText()) ;
	html += row(tr("Video bandwidth:"),   tr("%1 KB/s").arg(qsbBandwidth->value(), 0, 'f', 0)) ;
	html += "</table>" ;

	qlSummary->setText(html) ;
}

// =====================================================================================
//                                   Accept / reject
// =====================================================================================

void AudioWizard::accept()
{
	VOIP_TRACE("wizard: accept");
	stopEverything() ;

	rsVOIP->setVoipATransmit(chosenTransmitMode()) ;
	rsVOIP->setVoipEchoCancel(qcbEcho->isChecked()) ;
	rsVOIP->setVoipAudioInputDevice(selectedInputDevice().toStdString()) ;
	rsVOIP->setVoipAudioOutputDevice(selectedOutputDevice().toStdString()) ;
	if (mCamerasFilled && qcbCamera->isEnabled())
		rsVOIP->setVoipVideoInputDevice(selectedCamera().toStdString()) ;
	rsVOIP->setVoipVideoMaxBandwidth(static_cast<int>(qsbBandwidth->value() * 1024)) ;

	QDialog::accept() ;
}

void AudioWizard::reject()
{
	VOIP_TRACE("wizard: reject");
	stopEverything() ;
	restoreSettings() ;

	QDialog::reject() ;
}

void AudioWizard::restoreSettings()
{
	rsVOIP->setVoipATransmit(mSaved.atransmit) ;
	rsVOIP->setVoipVoiceHold(mSaved.voiceHold) ;
	rsVOIP->setVoipfVADmin(mSaved.vadMin) ;
	rsVOIP->setVoipfVADmax(mSaved.vadMax) ;
	rsVOIP->setVoipiNoiseSuppress(mSaved.noiseSuppress) ;
	rsVOIP->setVoipiMinLoudness(mSaved.minLoudness) ;
	rsVOIP->setVoipEchoCancel(mSaved.echoCancel) ;
	rsVOIP->setVoipAudioInputDevice(mSaved.audioIn) ;
	rsVOIP->setVoipAudioOutputDevice(mSaved.audioOut) ;
	rsVOIP->setVoipVideoInputDevice(mSaved.videoIn) ;
	rsVOIP->setVoipVideoMaxBandwidth(mSaved.videoBandwidth) ;
}
