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

#pragma once

#include <QAudioInput>
#include <QAudioOutput>
#include <QQueue>

#include "AudioStats.h"
#include "SpeexProcessor.h"

#include "ui_AudioWizard.h"

class QGraphicsScene;
class QGraphicsItem;

class AudioWizard: public QWizard, public Ui::AudioWizard {
	private:
		Q_OBJECT
		Q_DISABLE_COPY(AudioWizard)
                AudioBar* abAmplify;
                AudioBar* abVAD;
                QAudioInput* inputDevice;
                QAudioOutput* outputDevice;
                QtSpeex::SpeexInputProcessor* inputProcessor;
                QtSpeex::SpeexOutputProcessor* outputProcessor;
                QQueue<QByteArray>packetQueue;

        protected:
                bool bTransmitChanged;

		QGraphicsScene *qgsScene;
		QGraphicsItem *qgiSource;
                //AudioOutputSample *aosSource;
		float fAngle;
		float fX, fY;

                //Settings sOldSettings;

		QTimer *ticker;

		bool bInit;
		bool bDelay;
		bool bLastActive;

		QPixmap qpTalkingOn, qpTalkingOff;

		int iMaxPeak;
                int iTicks;
        public slots:
                void on_playEcho_timeout();
                void on_Ticker_timeout();
                void on_qsMaxAmp_valueChanged(int);
                void on_qrPTT_clicked(bool);
                void on_qrVAD_clicked(bool);
                void on_qrContinuous_clicked(bool);
                void on_qsTransmitMin_valueChanged(int);
                void on_qsTransmitMax_valueChanged(int);
                void on_qcbHighContrast_clicked(bool);
                void updateTriggerWidgets(bool);
	public:
                explicit AudioWizard(QWidget *parent);
                ~AudioWizard();

        private slots :
                void loopAudio();

};
