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
                AudioWizard(QWidget *parent);
                ~AudioWizard();

        private slots :
                void loopAudio();

};
